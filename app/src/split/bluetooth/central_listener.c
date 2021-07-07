#include <logging/log.h>
LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#include <bluetooth/bluetooth.h>
#include <bluetooth/conn.h>

#include <zmk/event_manager.h>
#include <zmk/events/activity_state_changed.h>

static void set_sleep_params(struct bt_conn *conn, void *data) {
    struct bt_conn_info info;

    bt_conn_get_info(conn, &info);

    if (info.role == BT_CONN_ROLE_MASTER) {
        int err = bt_conn_le_param_update(conn, BT_LE_CONN_PARAM(0x0012, 0x0012, 10, 400));

        if (err) {
            LOG_DBG("Failed to sleep split connection: %d", err);
        }
    }
}

static void set_wake_params(struct bt_conn *conn, void *data) {
    struct bt_conn_info info;

    bt_conn_get_info(conn, &info);

    if (info.role == BT_CONN_ROLE_MASTER) {
        int err = bt_conn_le_param_update(conn, BT_LE_CONN_PARAM(0x0006, 0x0006, 30, 400));

        if (err) {
            LOG_DBG("Failed to wake up split connection: %d", err);
        }
    }
}

static void sleep_all() {
    LOG_DBG("Sleeping!");

    bt_conn_foreach(BT_CONN_TYPE_LE, set_sleep_params, NULL);
}

static void wake_all() {
    LOG_DBG("Waking!");

    bt_conn_foreach(BT_CONN_TYPE_LE, set_wake_params, NULL);
}

int central_event_handler(const zmk_event_t *eh) {
    struct zmk_activity_state_changed *ev = as_zmk_activity_state_changed(eh);
    if (ev == NULL) {
        return -ENOTSUP;
    }

    switch (ev->state) {
    case ZMK_ACTIVITY_ACTIVE:
        wake_all();
        break;
    case ZMK_ACTIVITY_IDLE:
        sleep_all();
        break;
    case ZMK_ACTIVITY_SLEEP:
        break;
    default:
        LOG_WRN("Unhandled activity state: %d", ev->state);
        return -EINVAL;
    }
    return 0;
}

ZMK_LISTENER(central, central_event_handler);
ZMK_SUBSCRIPTION(central, zmk_activity_state_changed);
