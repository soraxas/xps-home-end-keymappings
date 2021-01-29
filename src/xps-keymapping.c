// compile:
//     gcc caps2esc.c -o caps2esc -I/usr/include/libevdev-1.0 -levdev -ludev
// run:
//     sudo nice -n -20 ./caps2esc >caps2esc.log 2>caps2esc.err &

#include <stdio.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>

#include <fcntl.h>
#include <signal.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>

#include <libudev.h>
#include <sys/select.h>

#include <libevdev/libevdev.h>
#include <libevdev/libevdev-uinput.h>

// clang-format off
const int STATE_KEY_UP = 0;
const int STATE_KEY_DOWN = 1;
const int STATE_KEY_REPEAT = 2;
const struct input_event
meta_up         = {.type = EV_KEY, .code = KEY_LEFTMETA, .value = STATE_KEY_UP},
meta_down       = {.type = EV_KEY, .code = KEY_LEFTMETA, .value = STATE_KEY_DOWN},
meta_repeat     = {.type = EV_KEY, .code = KEY_LEFTMETA, .value = STATE_KEY_REPEAT},
left_up         = {.type = EV_KEY, .code = KEY_LEFT,     .value = STATE_KEY_UP},
left_down       = {.type = EV_KEY, .code = KEY_LEFT,     .value = STATE_KEY_DOWN},
left_repeat     = {.type = EV_KEY, .code = KEY_LEFT,     .value = STATE_KEY_REPEAT},
right_up        = {.type = EV_KEY, .code = KEY_RIGHT,    .value = STATE_KEY_UP},
right_down      = {.type = EV_KEY, .code = KEY_RIGHT,    .value = STATE_KEY_DOWN},
right_repeat    = {.type = EV_KEY, .code = KEY_RIGHT,    .value = STATE_KEY_REPEAT},
up_up           = {.type = EV_KEY, .code = KEY_UP,       .value = STATE_KEY_UP},
up_down         = {.type = EV_KEY, .code = KEY_UP,       .value = STATE_KEY_DOWN},
up_repeat       = {.type = EV_KEY, .code = KEY_UP,       .value = STATE_KEY_REPEAT},
down_up         = {.type = EV_KEY, .code = KEY_DOWN,     .value = STATE_KEY_UP},
down_down       = {.type = EV_KEY, .code = KEY_DOWN,     .value = STATE_KEY_DOWN},
down_repeat     = {.type = EV_KEY, .code = KEY_DOWN,     .value = STATE_KEY_REPEAT},

home_up         = {.type = EV_KEY, .code = KEY_HOME,     .value = STATE_KEY_UP},
home_down       = {.type = EV_KEY, .code = KEY_HOME,     .value = STATE_KEY_DOWN},
end_up          = {.type = EV_KEY, .code = KEY_END,      .value = STATE_KEY_UP},
end_down        = {.type = EV_KEY, .code = KEY_END,      .value = STATE_KEY_DOWN},
pgup_up         = {.type = EV_KEY, .code = KEY_PAGEUP,   .value = STATE_KEY_UP},
pgup_down       = {.type = EV_KEY, .code = KEY_PAGEUP,   .value = STATE_KEY_DOWN},
pgdown_up       = {.type = EV_KEY, .code = KEY_PAGEDOWN, .value = STATE_KEY_UP},
pgdown_down     = {.type = EV_KEY, .code = KEY_PAGEDOWN, .value = STATE_KEY_DOWN},

esc_up          = {.type = EV_KEY, .code = KEY_ESC,      .value = STATE_KEY_UP},
ctrl_up         = {.type = EV_KEY, .code = KEY_LEFTCTRL, .value = STATE_KEY_UP},
capslock_up     = {.type = EV_KEY, .code = KEY_CAPSLOCK, .value = STATE_KEY_UP},
esc_down        = {.type = EV_KEY, .code = KEY_ESC,      .value = STATE_KEY_DOWN},
ctrl_down       = {.type = EV_KEY, .code = KEY_LEFTCTRL, .value = STATE_KEY_DOWN},
capslock_down   = {.type = EV_KEY, .code = KEY_CAPSLOCK, .value = STATE_KEY_DOWN},
esc_repeat      = {.type = EV_KEY, .code = KEY_ESC,      .value = STATE_KEY_REPEAT},
ctrl_repeat     = {.type = EV_KEY, .code = KEY_LEFTCTRL, .value = STATE_KEY_REPEAT},
capslock_repeat = {.type = EV_KEY, .code = KEY_CAPSLOCK, .value = STATE_KEY_REPEAT};
// clang-format on

// 0=disable blocking, 1=block super-key, 2=allow super-key but attempt to pass-through
int blocking_mode = 1;


#define eq_keyup(input_event, keycode) (((input_event)->code == (keycode)) && ((input_event)->value == (STATE_KEY_UP)))
#define eq_keydown(input_event, keycode) (((input_event)->code == (keycode)) && ((input_event)->value == (STATE_KEY_DOWN)))
#define eq_keyrepeat(input_event, keycode) (((input_event)->code == (keycode)) && ((input_event)->value == (STATE_KEY_REPEAT)))

int eventmap(const struct input_event *input, struct input_event output[]) {
    static int meta_is_down = 0, meta_give_up = 0, blocking_mode_2_keycombo = 0;

#ifdef CAP2ESC
    static int capslock_is_down = 0, esc_give_up = 0;
#endif

    if (input->type == EV_MSC && input->code == MSC_SCAN)
        return 0;

    else if (input->type != EV_KEY) {
        output[0] = *input;
        return 1;
    }

#ifdef CAP2ESC
    // turn capslock to esc
    else if (capslock_is_down)
    {
        if (input->code == KEY_CAPSLOCK)
        {
            // see whether esc had been given up (by treating it as ctrl)
            // if not, then return a esc down&up sequence.
            if (input->value == STATE_KEY_UP)
            {
                capslock_is_down = 0;
                if (esc_give_up)
                {
                    esc_give_up = 0;
                    output[0] = ctrl_up;
                    return 1;
                }
                output[0] = esc_down;
                output[1] = esc_up;
                return 2;
            }
            // ignore KEY_DOWN and KEY_REPEAT event as flag is already set to down
            return 0;
        }
        else if (input->code == KEY_LEFTCTRL)
            // ignore this as CAPS held will triggers leftctrl key event
            return 0;
        else if (input->code == KEY_ESC)
        {
            // convert CAPSLOCK + ESC to actual CAPSLOCK signal
            output[0] = *input;
            output[0].code = KEY_CAPSLOCK;
            esc_give_up = 1;
            return 1;
        }

        int k = 0;

        if (!esc_give_up && input->value)
        {
            // treat this as helding ctrl
            esc_give_up = 1;
            output[k++] = ctrl_down;
        }

        output[k++] = *input;
        return k;
    }

    else if (input->code == KEY_CAPSLOCK && input->value == STATE_KEY_DOWN)
    {
        capslock_is_down = 1;
        return 0;
    }
#endif

    else if (meta_is_down) {
        if (eq_keyup(input, KEY_LEFTMETA))
        {
            meta_is_down = 0;
            switch (blocking_mode)
            {
                case 0:
                    output[0] = meta_up;
                    return 1;

                case 1:
                    if (!meta_give_up) {
                        output[0] = meta_down;
                        output[1] = meta_up;
                        return 2;
                    }
                    return 0;

                case 2:
                    if (blocking_mode_2_keycombo) {
                        output[0] = meta_up;
                        return 1;
                    }
                    else if (!meta_give_up) {
                        output[0] = meta_down;
                        output[1] = meta_up;
                        return 2;
                    } 
                    // fall to default

                default:
                    return 0;
            }
        }

        // block key up (these had been emitted during key down)
        else if (eq_keyup(input, KEY_LEFT) || eq_keyup(input, KEY_RIGHT) || eq_keyup(input, KEY_UP) || eq_keyup(input, KEY_DOWN))
            return 0;

        else if (eq_keydown(input, KEY_LEFT) || eq_keyrepeat(input, KEY_LEFT))
        {
            output[0] = home_down;
            output[1] = home_up;
            meta_give_up = 1;
            return 2;
        }

        else if (eq_keydown(input, KEY_RIGHT) || eq_keyrepeat(input, KEY_RIGHT))
        {
            output[0] = end_down;
            output[1] = end_up;
            meta_give_up = 1;
            return 2;
        }

        else if (eq_keydown(input, KEY_UP) || eq_keyrepeat(input, KEY_UP))
        {
            output[0] = pgup_down;
            output[1] = pgup_up;
            meta_give_up = 1;
            return 2;
        }

        else if (eq_keydown(input, KEY_DOWN) || eq_keyrepeat(input, KEY_DOWN))
        {
            output[0] = pgdown_down;
            output[1] = pgdown_up;
            meta_give_up = 1;
            return 2;
        }

        if (blocking_mode == 2 && input->code != KEY_LEFTMETA) {
            // if modifier key is pressed, not inject meta yet (only react to non-modifier key)
            switch (input->code)
            {
                case KEY_LEFTSHIFT:
                case KEY_RIGHTSHIFT:
                case KEY_LEFTALT:
                case KEY_RIGHTALT:
                case KEY_LEFTMETA:
                case KEY_RIGHTMETA:
                    break;
                default:
                    // pass through by injecting a super_L press
                    output[0] = meta_down;
                    output[1] = *input;
                    blocking_mode_2_keycombo = 1;
                    return 2;
            }
        }
    }
    
    else if (eq_keydown(input, KEY_LEFTMETA)) {
        meta_is_down = 1;
        meta_give_up = 0;
        blocking_mode_2_keycombo = 0;
        if (blocking_mode > 0)
            return 0;
    }

    output[0] = *input;
    return 1;
}

int eventmap_loop(const char *devnode) {
    int result = 0;
    int fd     = open(devnode, O_RDONLY);
    if (fd < 0)
        return 0;

    struct libevdev *dev;
    if (libevdev_new_from_fd(fd, &dev) < 0)
        goto teardown_fd;

    sleep(1);

    if (libevdev_grab(dev, LIBEVDEV_GRAB) < 0)
        goto teardown_dev;
    if (libevdev_enable_event_type(dev, EV_KEY) < 0)
        goto teardown_grab;
    if (libevdev_enable_event_code(dev, EV_KEY, KEY_ESC, NULL) < 0)
        goto teardown_grab;
    if (libevdev_enable_event_code(dev, EV_KEY, KEY_CAPSLOCK, NULL) < 0)
        goto teardown_grab;
    if (libevdev_enable_event_code(dev, EV_KEY, KEY_LEFTCTRL, NULL) < 0)
        goto teardown_grab;
    if (libevdev_enable_event_code(dev, EV_KEY, KEY_LEFTMETA, NULL) < 0)
        goto teardown_grab;
    if (libevdev_disable_event_code(dev, EV_KEY, KEY_WLAN) < 0)
        goto teardown_grab;

    struct libevdev_uinput *udev;
    if (libevdev_uinput_create_from_device(dev, LIBEVDEV_UINPUT_OPEN_MANAGED,
                                           &udev) < 0)
        goto teardown_grab;

    for (;;) {
        struct input_event input;
        int rc = libevdev_next_event(
            dev, LIBEVDEV_READ_FLAG_NORMAL | LIBEVDEV_READ_FLAG_BLOCKING,
            &input);

        while (rc == LIBEVDEV_READ_STATUS_SYNC)
            rc = libevdev_next_event(dev, LIBEVDEV_READ_FLAG_SYNC, &input);

        if (rc == -EAGAIN)
            continue;

        if (rc != LIBEVDEV_READ_STATUS_SUCCESS)
            break;

        struct input_event output[2];
        for (int i = 0, k = eventmap(&input, output); i != k; ++i) {
            if (libevdev_uinput_write_event(
                    udev, output[i].type, output[i].code, output[i].value) < 0)
                goto teardown_udev;
            if (i + 1 != k) {
                if (libevdev_uinput_write_event(udev, EV_SYN, SYN_REPORT, 0) <
                    0)
                    goto teardown_udev;
                usleep(20000);
            }
        }
    }

    result = 1;

teardown_udev:
    libevdev_uinput_destroy(udev);
teardown_grab:
    libevdev_grab(dev, LIBEVDEV_UNGRAB);
teardown_dev:
    libevdev_free(dev);
teardown_fd:
    close(fd);

    return result;
}

// void eventmap_exec(const char *self_path, const char *devnode) {
void eventmap_exec(const int argc, const char *argv[], const char *devnode) {
    switch (fork()) {
        case -1:
            fprintf(stderr, "Fork failed on %s %s (%s)\n", argv[0], devnode,
                    strerror(errno));
            break;
        case 0: {
            char *command[argc + 2];
            command[0] = (char *) argv[0];  // self path
            command[1] = (char *) devnode;
            for (size_t i = 1; i < argc; i++)
                command[i + 1] = (char *) argv[i];

            command[argc + 1] = NULL;

            // char *command[] = {(char *)argv[0], (char *)devnode, NULL};
            execvp(command[0], command);
            fprintf(stderr, "Exec failed on %s %s (%s)\n", argv[0], devnode,
                    strerror(errno));
        } break;
    }
}

int should_grab(struct udev_device *device, int initial_scan) {
    if (device == NULL)
        return 0;

    const char virtual_devices_directory[] = "/sys/devices/virtual/input/";
    if (strncmp(udev_device_get_syspath(device), virtual_devices_directory,
                sizeof(virtual_devices_directory) - 1) == 0)
        return 0;

    if (!initial_scan) {
        const char *action = udev_device_get_action(device);
        if (!action || strcmp(action, "add"))
            return 0;
    }

    const char input_prefix[] = "/dev/input/event";
    const char *devnode       = udev_device_get_devnode(device);
    if (!devnode || strncmp(devnode, input_prefix, sizeof(input_prefix) - 1))
        return 0;

    int fd = open(devnode, O_RDONLY);
    if (fd < 0) {
        fprintf(stderr, "Failed to open %s (%s)\n", devnode, strerror(errno));
        return 0;
    }

    struct libevdev *dev;
    if (libevdev_new_from_fd(fd, &dev) < 0) {
        fprintf(stderr, "Failed to create evdev device from %s (%s)\n", devnode,
                strerror(errno));
        close(fd);
        return 0;
    }

    int should_be_grabbed =
        libevdev_has_event_type(dev, EV_KEY) &&
        (libevdev_has_event_code(dev, EV_KEY, KEY_ESC) ||
         libevdev_has_event_code(dev, EV_KEY, KEY_CAPSLOCK));

    libevdev_free(dev);
    close(fd);

    return should_be_grabbed;
}

void kill_zombies(__attribute__((unused)) int signum) {
    int status;
    while (waitpid(-1, &status, WNOHANG) > 0)
        ;
}

int main(int argc, const char *argv[]) {
    int initial_scan, i = 1;
    blocking_mode = 0;
    // // check for flags:
    // printf("> %i\n", argc);
    // for (i = 0; i < argc; ++i)
    //     printf("%s ", argv[i]);
    // printf("\n");

    i = 1;
    int argc_actual = argc;  // argc without the flags
    while (i < argc) {
        if (argv[i][0] == '-') {
            switch (argv[i][1]) {
                case '0':
                    blocking_mode = 0;
                    break;
                case '1':
                    blocking_mode = 1;
                    break;
                case '2':
                    blocking_mode = 2;
                    break;
                default:
                    printf("Unknown flags!\n");
                    return 1;
            }
            argc_actual--;
        }
        i++;
    }

    if (argc_actual > 2) {
        fprintf(stderr, "usage: caps2esc [device-path]\n");
        return EXIT_FAILURE;
    }

    if (argc_actual == 2) {
        // printf("Blocking mode: %i\n", blocking_mode);
        return !eventmap_loop(argv[1]);
    }

    struct sigaction sa;
    memset(&sa, 0, sizeof sa);
    sa.sa_flags   = SA_NOCLDSTOP;
    sa.sa_handler = &kill_zombies;
    if (sigaction(SIGCHLD, &sa, NULL) == -1) {
        perror("Couldn't summon zombie killer");
        return EXIT_FAILURE;
    }

    struct udev *udev = udev_new();
    if (!udev) {
        perror("Can't create udev");
        return EXIT_FAILURE;
    }

    struct udev_enumerate *enumerate = udev_enumerate_new(udev);
    udev_enumerate_add_match_subsystem(enumerate, "input");
    udev_enumerate_scan_devices(enumerate);
    struct udev_list_entry *devices = udev_enumerate_get_list_entry(enumerate);
    struct udev_list_entry *dev_list_entry;
    udev_list_entry_foreach(dev_list_entry, devices) {
        struct udev_device *device = udev_device_new_from_syspath(
            udev, udev_list_entry_get_name(dev_list_entry));
        if (device) {
            if (should_grab(device, initial_scan = 1))
                eventmap_exec(argc, argv, udev_device_get_devnode(device));
            udev_device_unref(device);
        }
    }
    udev_enumerate_unref(enumerate);

    struct udev_monitor *monitor = udev_monitor_new_from_netlink(udev, "udev");
    if (!monitor) {
        perror("Can't create monitor");
        return EXIT_FAILURE;
    }

    udev_monitor_filter_add_match_subsystem_devtype(monitor, "input", NULL);
    udev_monitor_enable_receiving(monitor);
    int fd = udev_monitor_get_fd(monitor);
    printf("Usage: ./xps-keymapping [-(0|1|2)]"
           "Keymapping:\n"
           "  [Super_L] + LEFT  = [Home]\n"
           "  [Super_L] + RIGHT = [End]\n"
           "  [Super_L] + UP    = [PgUp]\n"
           "  [Super_L] + DOWN  = [PgDown]\n"
           "\n"
           "Note:\n"
           "  1. Super_L's effect will be cancelled if a keymapping is successfully executed\n"
           "  2. Key-repeating feature is enabled for the keymapping\n"
           "\n"
           "Blocking mode for Super_L:\n"
           "  -0: Disable blocking (default)\n"
           "  -1: Robustly intercepts super-key but allows single key-press (other \n"
           "      superkey keycombos will not work)\n"
           "  -2: Simulates super-key passthrough by listening to other keypress \n"
           "      and inject a superkey key-press event when keys other than \n"
           "      LEFT/RIGHT is pressed\n"
           "   e.g. Run it as ./xps-keymapping -0 for blockingmode 0, etc.\n"
           "\n"
           ">> Current blocking mode: %i\n",
           blocking_mode);
    for (;;) {
        fd_set fds;
        FD_ZERO(&fds);
        FD_SET(fd, &fds);

        if (select(fd + 1, &fds, NULL, NULL, NULL) > 0 && FD_ISSET(fd, &fds)) {
            struct udev_device *device = udev_monitor_receive_device(monitor);
            if (device) {
                if (should_grab(device, initial_scan = 0))
                    eventmap_exec(argc, argv, udev_device_get_devnode(device));
                udev_device_unref(device);
            }
        }
    }

    udev_monitor_unref(monitor);
    udev_unref(udev);
}
