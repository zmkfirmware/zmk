
#include <stdlib.h>
#include <zephyr/kernel.h>
#include <zephyr/shell/shell.h>
#include <zmk/event_manager.h>
#include <zmk/events/position_state_changed.h>

#define HELP_NONE "[key_position]"

static int parse_key_position(const struct shell *shell, char *pos_str) {
    int position;
    char *endptr;

    position = strtoul(pos_str, &endptr, 10);

    if (endptr == pos_str) {
        shell_error(shell, "Enter an integer key position");
        return -EINVAL;
    }

    return position;
}

static int parse_and_raise(const struct shell *shell, char *pos_str, bool pressed) {
    int position = parse_key_position(shell, pos_str);

    if (position < 0) {
        return position;
    }

    ZMK_EVENT_RAISE(new_zmk_position_state_changed(
        (struct zmk_position_state_changed){.source = ZMK_POSITION_STATE_CHANGE_SOURCE_LOCAL,
                                            .state = pressed,
                                            .position = position,
                                            .timestamp = k_uptime_get()}));

    return position;
}

static int cmd_key_tap(const struct shell *shell, size_t argc, char **argv) {
    if (argc != 2) {
        return -EINVAL;
    }

    parse_and_raise(shell, argv[1], true);

    k_sleep(K_MSEC(50));

    parse_and_raise(shell, argv[1], false);

    return 0;
};

static int cmd_key_press(const struct shell *shell, size_t argc, char **argv) {
    if (argc != 2) {
        return -EINVAL;
    }

    parse_and_raise(shell, argv[1], true);

    return 0;
};

static int cmd_key_release(const struct shell *shell, size_t argc, char **argv) {
    if (argc != 2) {
        return -EINVAL;
    }

    parse_and_raise(shell, argv[1], false);

    return 0;
};

SHELL_STATIC_SUBCMD_SET_CREATE(sub_key, SHELL_CMD(tap, NULL, HELP_NONE, cmd_key_tap),
                               SHELL_CMD(press, NULL, HELP_NONE, cmd_key_press),
                               SHELL_CMD(release, NULL, HELP_NONE, cmd_key_release),
                               SHELL_SUBCMD_SET_END /* Array terminated. */
);

SHELL_CMD_REGISTER(key, &sub_key, "Key commands", NULL);