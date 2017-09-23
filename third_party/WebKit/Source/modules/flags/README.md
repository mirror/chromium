# Origin Flags

Rough Spec: https://github.com/inexorabletash/origin-flags

## TODO

* Flag options (timeout)

## General Lifecycle

1. Script calls `requestFlag()`
2. Renderer calls `FlagsService::RequestFlag(...)` (Renderer &rarr; Browser)
3. In parallel, Renderer mints a Promise to return to script
4. Browser puts request in queue, processes queue
5. Request granted!
6. Browser mints a strongly-bound `FlagHandle`, responds via callback
7. Renderer resolves Promise with new `Flag` instance (which holds the handle)
9. Renderer waits until Flag's _waiting promises_ resolves
10. Renderer drops the handle
11. Browser sees this c/o a strong binding
12. Browser releases flag, processes queue

## Releasing Flags

Apart from the degenerate `waitUntil(new Promise(r=>{}))` case (bad user code),
it is critical that flags are released when the holder terminates or an origin
using the feature can become wedged.

Flag _requests_ are cancelled when the source terminates - process
crash, frame detached, worker terminated. Each source is handled as
binding in the service's BindingSet and outstanding requests are
tracked in an associated context. When the pipe is broken the requests
are dequeued.

_Held_ flags are represented by a opaque `FlagHandle`; dropping this on
the renderer - either due to the release conditions obtaining, or due to
the source terminating - results in the flag being dropped.
