# XPS keymappings for Home and End shortcuts

This brings back the wonderful shortcut keys of Home and End button on the left and right arrow keys. The results is `Super+Left` becomes the `Home` key, and `Super+Right` becomes the `End` key.
## Dependencies

- [libevdev][]

## Building

```sh
cd src
make
```

## Execution

The following daemonized sample execution increases the application priority
(since it'll be responsible for a vital input device, just to make sure it stays
responsible):

```sh
sudo nice -n -20 ./xps-keymapping
```

If you want to use other blocking mode (mode 2 seems to works pretty well), run it as the fllowing instead:

```sh
sudo nice -n -20 ./xps-keymapping -2
```

## How it works

Executing `xps-keymapping` without parameters (with the necessary privileges to access
input devices) will make it monitor any devices connected (or that gets
connected) that produces SUPER_L events.

Upon detection it will fork and exec itself now passing the path of the detected
device as its first parameter. This child instance is then responsible for
producing an uinput clone of such device and doing the programmatic keymapping
of such device until it disconnects, at which time it ends its execution.

## Caveats

As always, there's always a caveat:

- It will "grab" the detected devices for itself.
- If you tweak your key repeat settings, check whether they get reset.  

