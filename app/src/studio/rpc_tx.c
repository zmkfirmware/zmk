// SPDX-License-Identifier: MIT

#include <pb_encode.h>
#include <zephyr/kernel.h>
#include "msg_framing.h"
#include "rpc_tx.h"

BUILD_ASSERT(CONFIG_ZMK_STUDIO_RPC_TX_BUF_SIZE > 0);

// Writers serialize frames. Neither endpoint changes nor completion callbacks take this mutex.
static K_MUTEX_DEFINE(writer);
static K_SEM_DEFINE(progress, 0, 1);
static struct k_spinlock lock;
static const struct zmk_rpc_transport *selected;
static uint32_t session, ticket;
static const uint8_t *pending;
static size_t remaining;
static int error;

uint32_t studio_tx_session(void) {
    k_spinlock_key_t key = k_spin_lock(&lock);
    uint32_t current = session;
    k_spin_unlock(&lock, key);
    return current;
}

void studio_tx_select(const struct zmk_rpc_transport *transport) {
    k_spinlock_key_t key = k_spin_lock(&lock);
    selected = transport;
    session++;
    remaining = 0;
    k_spin_unlock(&lock, key);
    k_sem_give(&progress);
}

size_t studio_tx_peek(enum zmk_transport transport, uint8_t *buf, size_t size, uint32_t *id) {
    k_spinlock_key_t key = k_spin_lock(&lock);
    size_t len = selected && selected->transport == transport ? MIN(size, remaining) : 0;
    if (len) {
        memcpy(buf, pending, len);
        *id = ticket;
    }
    k_spin_unlock(&lock, key);
    return len;
}

void studio_tx_complete(uint32_t id, int sent) {
    k_spinlock_key_t key = k_spin_lock(&lock);
    if (id == ticket && remaining && sent != 0) {
        if (sent < 0 || (size_t)sent > remaining) {
            error = sent < 0 ? sent : -EIO;
            remaining = 0;
        } else {
            pending += sent;
            remaining -= sent;
        }
        ticket++;
        k_sem_give(&progress);
    }
    k_spin_unlock(&lock, key);
}

struct tx_frame {
    const struct zmk_rpc_transport *transport;
    uint32_t session;
    size_t used;
    uint8_t bytes[CONFIG_ZMK_STUDIO_RPC_TX_BUF_SIZE];
    int error;
};

static bool flush(struct tx_frame *frame) {
    k_spinlock_key_t key = k_spin_lock(&lock);
    if (session != frame->session) {
        k_spin_unlock(&lock, key);
        frame->error = -ENOTCONN;
        return false;
    }
    pending = frame->bytes;
    remaining = frame->used;
    error = 0;
    ticket++;
    k_spin_unlock(&lock, key);
    frame->transport->tx_start();

    // One deadline per bounded chunk, independent of spurious or late wakeups.
    k_timepoint_t deadline = sys_timepoint_calc(K_SECONDS(1));
    for (;;) {
        key = k_spin_lock(&lock);
        frame->error = session == frame->session ? error : -ENOTCONN;
        if (!remaining || frame->error || sys_timepoint_expired(deadline)) {
            if (remaining && !frame->error) {
                frame->error = -ETIMEDOUT;
            }
            remaining = 0;
            pending = NULL;
            k_spin_unlock(&lock, key);
            frame->used = 0;
            return !frame->error;
        }
        k_spin_unlock(&lock, key);
        k_sem_take(&progress, sys_timepoint_timeout(deadline));
    }
}

static bool put_byte(struct tx_frame *frame, uint8_t byte) {
    frame->bytes[frame->used++] = byte;
    return frame->used < sizeof(frame->bytes) || flush(frame);
}

static bool encode_bytes(pb_ostream_t *stream, const uint8_t *buf, size_t count) {
    struct tx_frame *frame = stream->state;
    for (size_t i = 0; i < count; i++) {
        uint8_t b = buf[i];
        if ((b == FRAMING_SOF || b == FRAMING_ESC || b == FRAMING_EOF) &&
            !put_byte(frame, FRAMING_ESC)) {
            return false;
        }
        if (!put_byte(frame, b)) {
            return false;
        }
    }
    return true;
}

int studio_tx_send(const zmk_studio_Response *response, uint32_t expected_session) {
    // Only the serialized writer accesses the staging buffer and resync flags.
    static struct tx_frame frame;
    static bool resync[ZMK_TRANSPORT_BLE + 1];
    k_mutex_lock(&writer, K_FOREVER);
    k_spinlock_key_t key = k_spin_lock(&lock);
    frame.transport = selected;
    frame.session = session;
    k_spin_unlock(&lock, key);
    int rc = -ENOTCONN;
    if (frame.transport && frame.session == expected_session) {
        frame.used = 0;
        frame.error = 0;
        bool *dirty = &resync[frame.transport->transport];
        pb_ostream_t stream = {.callback = encode_bytes, .state = &frame, .max_size = SIZE_MAX};
        // These delimiters discard an interrupted frame even after a trailing escape.
        bool ok = !*dirty || (put_byte(&frame, FRAMING_SOF) && put_byte(&frame, FRAMING_SOF) &&
                              put_byte(&frame, FRAMING_EOF));
        ok = ok && put_byte(&frame, FRAMING_SOF) &&
             pb_encode(&stream, &zmk_studio_Response_msg, response) &&
             put_byte(&frame, FRAMING_EOF) && (!frame.used || flush(&frame));
        *dirty = !ok;
        rc = ok ? 0 : (frame.error ? frame.error : -EINVAL);
    }
    k_mutex_unlock(&writer);
    return rc;
}
