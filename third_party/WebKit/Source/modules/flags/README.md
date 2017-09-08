# Origin Flags

Rough Spec: https://gist.github.com/inexorabletash/a53c6add9fbc8b9b1191

## TODO

* Flag options (timeout)

* Handle terminated requests by iframes
 * frame1 requests and holds [r1] - should be granted immediately
 * frame2 requests [r1, r2] - should be blocked
 * frame3 requests [r2] - should be blocked
 * frame2 is terminated
 * frame3's request should be granted immediately
 * This doesn't happen because (1) the channel itself doesn't close and
   (2) the frame has no handle to signal a terminated request. The
   cleanest fix would a different channel per iframe.


## General Lifecycle

1. Script calls `requestFlag()`, mints Promise
2. ... which turns into `FlagsService::RequestFlag(...)` Mojo call (Renderer &rarr; Browser)
3. Browser puts request in queue, processes queue
4. Requested granted, passes `flag_id` back Renderer &rarr; Browser
5. Resolves Promise with `Flag` instance (which wraps `flag_id`)
6. Wait until _waiting promises_ resolve
7. Calls into `FlagsService::ReleaseFlag(flag_id)` Mojo call (Renderer &rarr; Browser)
8. Browser releases flag, processes queue

## Releasing Flags

Apart from the degenerate `waitUntil(new Promise(r=>{}))` case (bad user code),
it is critical that flags are released when the holder terminates or an origin
using the feature can become wedged.

Flag _requests_ are cancelled when:
 * Source terminates (via `FlagsServiceImpl::OnConnectionError`)
 * When granted, if script execution was terminated (via `FlagRequestCallbacks::onSuccess`)
 * When granted, if worker thread was terminated (via `FlagsServiceImpl::OnConnectionError`)

_Held_ flags are released when:
 * Waiting promises all resolve (via `Flag::decrementPending`)
 * Source terminates (via `FlagsServiceImpl::OnConnectionError`)
 * On execution termination (via `ActiveDOMObject`)
 * On GC (via `USING_PRE_FINALIZER`)

See TODO (above) for an edge case; the overall accounting is still
accurate but a potentially grantable flag is blocked.

### Request Flow
* script calls `self.requestFlag(scope, mode)`
* --> `GlobalFlagEnvironment::requestFlag(scope, mode)`
  * looks up current security origin
  * mints a FlagRequestCallbacks object, returns bound Promise
* ----> `Platform::current()->flagProvider()->requestFlag(origin, scope, mode, callbacks)`
* ------> `FlagsProvider::requestFlag(origin, scope, mode, callbacks)`
  * wraps callback in a closure on `::RequestCallback`
* --------> `FlagsServicePtr::RequestFlag(origin, scope, mode, closure)`
  * *...Mojo...* (renderer &rarr; browser)
* --> `FlagsServiceImpl::RequestFlag(origin, scope, mode, callback)`
  * converts mojo types
* ----> `FlagContext::RequestFlag(origin, scope, mode, callback)`
  * adds request (scope, mode, id, closure) to origin's queue
  * returns unique flag_id
* <-- `FlagsServiceImpl::RequestFlag`
  * remembers flag_id, to release it in case channel terminates
  * pokes `FlagContext::ProcessRequests(origin)` just in case

### Granted Flow
* --> `FlagContext::ProcessRequests(origin)`
  * calls callback.Run(flag_id)
  * *...Mojo...* (browser &rarr; renderer)
* --> `::RequestCallback(flag_id)`
* ----> `FlagRequestCallbacks::onSuccess(flag_id)`
  * resolves Promise with new Flag

### Timeout Flow - TODO
* ???
  * ???
  * *...Mojo...* (browser &rarr; renderer)
* --> `::RequestCallback(flag_id)` (id is -1)
* ----> `FlagRequestCallbacks::onSuccess(flag_id)`
  * rejects Promise with TimeoutError

### Release Flow
* all waiting Promises resolve
* `Flag::decrementPending()`
* --> `Flag::releaseIfHeld()`
* ---> `Platform::current()->flagProvider()->releaseFlag(origin, flag_id)`
* -----> `FlagsProvider::releaseFlag(origin, flag_id)`
* --------> `FlagsServicePtr::ReleaseFlag(origin, flag_id)`
  * *...Mojo...* (renderer &rarr; browser)
* --> `FlagsServiceImpl::ReleaseFlag(origin, flag_id)`
* ----> `FlagContext::ReleaseFlag(origin, flag_id)`
  * pokes `FlagContext::ProcessRequests(origin)` just in case

### On GC
* `Flag::dispose` (via `USING_PRE_FINALIZER`)
* --> `Flag::releaseIfHeld()`
* (same as regular release flow...)

### On Stop (e.g. navigate)
* `Flag::stop` (via `ActiveDOMObject`)
* --> `Flag::releaseIfHeld()`
* (same as regular release flow...)
