# XPS keymappings on Home and End keys

This brings the wonderful shortcut keys of Home and End button on the left and right arrow keys, to other keyboard. 

The results is:

- <kbd>LSuper</kbd> + <kbd>Left</kbd> becomes the <kbd>Home</kbd> key
- <kbd>LSuper</kbd> + <kbd>Right</kbd> becomes the <kbd>End</kbd> key
## Dependencies

- libevdev

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

## Notes:

- <kbd>LSuper</kbd>'s effect will be nullify if a keymapping is successfully executed
- Key-repeating feature is enabled for the keymapping

## Blocking mode `[0|1|2]` for <kbd>LSuper</kbd>

<kbd>LSuper</kbd>'s effect will needs to be conditionally nullify depending on whether the desire keymapping (i.e. Super+Left/Right) had been successfully executed or not. There are 3 blocking modes implemented:

- -0: Disable blocking (default)
- -1: Robustly intercepts all <kbd>LSuper</kbd> key-presses, but allows single key-press when <kbd>Left</kbd>/<kbd>Right</kbd> is not being pressed. However, other <kbd>LSuper</kbd> keycombos will not work because the key-press down event will be intercepted by this program.
- -2: Simulates <kbd>LSuper</kbd> passthrough (it's not an actual passthrough) by first blocking the <kbd>LSuper</kbd>
 key-press down event, then start listening to other keypress and immediately inject a <kbd>LSuper</kbd> key-press event when keys other than <kbd>Left</kbd>/<kbd>Right</kbd> is pressed.


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

