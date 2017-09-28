# Web Locks

Proposal: https://github.com/inexorabletash/web-locks

## TODO

* Lock options (timeout)

## General Lifecycle

1. Script calls `requestLock()`
2. Renderer calls `LocksService::RequestLock(...)` (Renderer &rarr; Browser)
3. In parallel, Renderer mints a Promise to return to script
4. Browser puts request in queue, processes queue
5. Request granted!
6. Browser mints a strongly-bound `LockHandle`, responds via callback
7. Renderer resolves Promise with new `Lock` instance (which holds the handle)
9. Renderer waits until Lock's _waiting promises_ resolves
10. Renderer drops the handle
11. Browser sees this c/o a strong binding
12. Browser releases lock, processes queue

## Releasing Locks

Apart from the degenerate `waitUntil(new Promise(r=>{}))` case (bad user code),
it is critical that locks are released when the holder terminates or an origin
using the feature can become wedged.

Lock _requests_ are cancelled when the source terminates - process
crash, frame detached, worker terminated. Each source is handled as
binding in the service's BindingSet and outstanding requests are
tracked in an associated context. When the pipe is broken the requests
are dequeued.

_Held_ locks are represented by a opaque `LockHandle`; dropping this on
the renderer - either due to the release conditions obtaining, or due to
the source terminating - results in the lock being dropped.
