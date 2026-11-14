# Split central position-state regression

Run through the existing test runner from `app/`:

```sh
west test tests/split-central
```

Or build the standalone fixture from the repository root:

```sh
west build -s app/tests/split-central/position-state -b native_sim/native/64 -d build/split-central-position-state
build/split-central-position-state/zephyr/zephyr.exe
```

The fixture compiles the production Bluetooth central directly. It replaces
radio initialization, transport registration and connection unref so it can use
synthetic connections. The final transport event handler observes each accepted
position transition. Message queues, system work, spinlocks and the notification,
disconnect and drain paths are the real production/kernel implementations.

A semaphore holds the system worker while notifications fill the five-entry
queue. Tests verify releases with no further notification, identical snapshots,
unrelated changes, disconnect cleanup, immediate slot reuse, normal short taps,
callbacks during dispatch and two peripherals sharing the queue. The observer
rejects duplicate transitions.

This checks position-event convergence and ordering. It does not simulate radio
loss, host HID delivery or physical sleep/wake. Under saturation, transitions
that never entered the queue may coalesce to the latest state.
