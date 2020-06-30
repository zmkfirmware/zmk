
#include <zephyr.h>
#include <logging/log.h>

LOG_MODULE_DECLARE(zmk, CONFIG_ZMK_LOG_LEVEL);

#include <zmk/event-manager.h>

extern struct zmk_event_type* __event_type_start[];
extern struct zmk_event_type* __event_type_end[];

extern struct zmk_event_subscription __event_subscriptions_start[];
extern struct zmk_event_subscription __event_subscriptions_end[];

int zmk_event_manager_raise(struct zmk_event_header *event)
{
    int ret;
    struct zmk_event_subscription *ev_sub;
    for (ev_sub = __event_subscriptions_start; ev_sub != __event_subscriptions_end; ev_sub++) {
        if (ev_sub->event_type == event->event) {
            ret = ev_sub->listener->callback(event);
            if (ret) {
                LOG_DBG("Listener returned an error: %d", ret);
                goto release;
            }
        }
    }

release:
    k_free(event);
    return ret;
}