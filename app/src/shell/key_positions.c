
#include <stdlib.h>
#include <zephyr/kernel.h>
#include <zephyr/shell/shell.h>
#include <zmk/event_manager.h>
#include <zmk/events/position_state_changed.h>

#define HELP_NONE "[key_position]"
#define TAP_EVENT_SPACING K_MSEC(50)

static int parse_positive_int(char *s) {
    char *endptr;
    unsigned long value = strtoul(s, &endptr, 10);

    if (endptr == s)
        return -EINVAL;
    if (value > INT_MAX)
        return -ERANGE;
    return value;
}

static int parse_and_raise(const struct shell *shell, char *pos_str, bool pressed) {
    long position = parse_positive_int(pos_str);

    if (position < 0) {
        shell_error(shell, "Enter an integer key position");
        return position;
    }

    ZMK_EVENT_RAISE(new_zmk_position_state_changed(
        (struct zmk_position_state_changed){.source = ZMK_POSITION_STATE_CHANGE_SOURCE_LOCAL,
                                            .state = pressed,
                                            .position = position,
                                            .timestamp = k_uptime_get()}));

    return 0;
}

static int cmd_key_tap(const struct shell *shell, size_t argc, char **argv) {
    if (argc != 2) {
        return -EINVAL;
    }

    parse_and_raise(shell, argv[1], true);

    k_sleep(TAP_EVENT_SPACING);

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

static int cmd_sleep(const struct shell *shell, size_t argc, char **argv) {
    if (argc != 2) {
        return -EINVAL;
    }

    int sleep_ms = parse_positive_int(argv[1]);

    if (sleep_ms < 0) {
        shell_error(shell, "Enter a positive number of milliseconds");
        return sleep_ms;
    }

    k_sleep(K_MSEC(sleep_ms));

    return 0;
};

static int cmd_exit(const struct shell *shell, size_t argc, char **argv) { exit(0); };

SHELL_STATIC_SUBCMD_SET_CREATE(sub_key, SHELL_CMD(tap, NULL, HELP_NONE, cmd_key_tap),
                               SHELL_CMD(press, NULL, HELP_NONE, cmd_key_press),
                               SHELL_CMD(release, NULL, HELP_NONE, cmd_key_release),
                               SHELL_SUBCMD_SET_END /* Array terminated. */
);

SHELL_CMD_REGISTER(key, &sub_key, "Key commands", NULL);
SHELL_CMD_REGISTER(sleep, NULL, "Sleep (milliseconds)", cmd_sleep);
SHELL_CMD_REGISTER(exit, NULL, "Exit", cmd_exit);