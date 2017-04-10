Service Worker basic setup
==========================

`WebFrameClient::createServiceWorkerProvider()` makes a
`WebServiceWorkerProvider`.

`RenderFrameImpl::createServiceWorkerProvider()` implements this. I think this
is the normal one that Chrome uses. It makes a `WebServiceWorkerProviderImpl`.

`ServiceWorkerContextClient::createServiceWorkerProvider()` also implements
this. It does `new WebServiceWorkerProviderImpl`. TODO: When is
`ServiceWorkerContextClient` used?

(R) and (B) are used below to indicate whether the class lives in the (R)enderer
or the (B)rowser.

ServiceWorkerContainer (R)
--------------------------
`registerServiceWorker()` -> `registerServiceWorkerImpl()` -> does a bunch of
checks about origins and schemes and normalizing urls or something and then
calls `m_provider->registerServiceWorker()`, where `m_provider` is a
`WebServiceWorkerProvider` which will go up to a `WebServiceWorkerProviderImpl`
in chrome.

TODO: Not sure how this actually gets called, maybe straight from JS.


WebServiceWorkerProviderImpl (R)
--------------------------------
When created in `RenderFrameImpl` gets a `ServiceWorkerNetworkProvider` as a
context object.

`WebServiceWorkerProviderImpl` is documented as "correspond[ing] to one
ServiceWorkerContainer interface in JS context (i.e. navigator.serviceWorker)".

This object has primarily:
- `registerServiceWorker()`

`GetDispatcher()->RegisterServiceWorker()` where `GetDispatcher()` is a
`ServiceWorkerDispatcher`.

- `getRegistration()`
- `getRegistrations()`
- `getRegistrationForReady()`
- `validateScopeAndScriptURL()`

ServiceWorkerDispatcher (R)
---------------------------
A ServiceWorkerDispatcher per thread (uses TLS). TODO: How many threads are
there?

This thing has `RegisterServiceWorker()` which gets called above (corresponding
`to navigator.serviceWorker.register()`) which saves the pending registration
and then sends back to the browser `ServiceWorkerHostMsg_RegisterServiceWorker`.

That gets back to the browser in `ServiceWorkerDispatcherHost`.

ServiceWorkerDispatcherHost (B)
-------------------------------
`ServiceWorkerDispatcherHost::OnRegisterServiceWorker()` does some validation
and either sends back a failure reason, or calls
`GetContext()->RegisterServiceWorker()` binding a completion to
`&RegistrationComplete`. `GetContext()` is a `ServiceWorkerContextCore`.

When `RegistrationComplete` is called, gets the
`ServiceWorkerProviderHost` for the render process id and provider id (TODO:
What is a provider id?) and if everything is still OK, replies to the renderer
with `ServiceWorkerMsg_ServiceWorkerRegistered`.

ServiceWorkerContextCore (B)
----------------------------
Manages data associated with service worker. Single threaded, IO thread only.

One instance per `StoragePartition` (TODO: What is a StoragePartition?). This is
the root of the hierarchy of service worker data for a particular partition.

It implements `ServiceWorkerVersion::Listener`.

`RegisterServiceWorker` calls `job_coordinator_->Register()` (on a
`ServiceWorkerJobCoordinator`) into `&RegistrationComplete` which calls back
into the callback that goes back to `ServiceWorkerDispatcherHost`.

ServiceWorkerJobCoordinator (B)
-------------------------------
"manages all in-flight registration or unregistration jobs". Don't know why it
needs to have a queue but it does `new ServiceWorkerRegisterJob` appends that
into a queue for the `pattern` (TODO: This is the script-provided pattern?) and
then sets the callback on that queued job which will call back into
`ServiceWorkerContextCore::RegistrationComplete`.

ServiceWorkerRegisterJob (B)
----------------------------
- Creates a `ServiceWorkerRegistration` if there isn't one
- Creates a `ServiceWorkerVersion` for the new version
- Starts a worker for the `ServiceWorkerVersion` *** TODO: Where? How?
- Fires 'install' for `ServiceWorkerVersion`
- Fires 'activate' for `ServiceWorkerVersion`
- Waits for old `ServiceWorkerVersion`s to deactivate
- Set new version as the active one
- Updates storage

Basically manages the state machine transitions between all the states.

Follows spec
`Start()` -> `StartImpl()` -> `ContinueWithRegistration()` ->
`RegisterAndContinue()` -> `UpdateAndContinue()`.

`RegisterAndContinue()` creates a `ServiceWorkerRegistration`.

`UpdateAndContinue` makes a `ServiceWorkerVersion` and then calls
`new_version()->StartWorker()` (callback back to &OnStartWorkerFinished).

ServiceWorkerVersion (B)
------------------------
Corresponds to a specific version of a service worker script for a given
pattern. During upgrade there's more than one `ServiceWorkerVersion` running,
but only one is activated (i.e. serving) at a time. `ServiceWorkerVersion`
connects the script with a running worker.

On creation it makes `embedded_worker_` from
`context_->embedded_worker_registry()->CreateWorker()`, which is on
`EmbeddedWorkerRegistry`.

`StartWorker()` called from IO thread, does some checks then calls
`context_->storage()->FindRegistrationForId()`. `context_` is a
`ServiceWorkerContextCore`. `storage()` on that is a `ServiceWorkerStorage`.

`DidEnsureLiveRegistrationForStartWorker()` -> `StartWorkerInternal()`

`StartWorkerInternal()` calls `embedded_worker_->Start()` which calls back to
`&OnStartSentAndScriptEvaluated`.

For fetch routing, `StartRequest()` is mostly an "add ref" on the script so that
it keeps while a request is pending. The callback from the renderer process
calls `FinishRequest()` once a fetch, etc. has been dispatched.

ServiceWorkerStorage (B)
------------------------
This is a subchunk of data that is conceptually part of
`ServiceWorkerContextCore`. It holds registration data (?) TODO: And maybe the
cache of data that the service worker stores?

`FindRegistrationForId()` handles some no-op cases or dispatches to
`database_task_manager_` which is a `ServiceWorkerDatabaseTaskManager` which
uses that task runner to run `&FindForIdInDB()` and then back to
`DidFindRegistrationForId()`.

`FindForIdInDB()` does `database->ReadRegistration()`. `database` is a
`ServiceWorkerDatabase`. Gets a `ServiceWorkerDatabase::RegistrationData` back
which is id, scope, script url, update times, `has_fetch_handler`, etc.

`DidFindRegistrationForId()` -> `ReturnFoundRegistration()` ->
`CompleteFindNow()` -> `callback`. I think this callback eventually gets back to
`ServiceWorkerVersion::DidEnsureLiveRegistrationForStartWorker()`

ServiceWorkerDatabase (B)
-------------------------
Persists service worker registration data in a database. Not on IO thread
because it writes files. Owned by `ServiceWorkerStorage` (which is in turn owned
by `ServiceWorkerContextCore`) and that is responsible for only calling it on a
worker pool thread. Uses level db.

EmbeddedWorkerRegistry (B)
--------------------------
Stub between `MessageFilter` and `EmbeddedWorkerInstance`. Owned by
`ServiceWorkerContextCore`. Lives on IO thread only. I'm not sure what this
class does exactly.

`CreateWorker()` creates an `EmbeddedWorkerInstance` which does most of the
work.

EmbeddedWorkerInstance (B)
--------------------------
Makes a `StartTask` and then calls `Start()` on that. That mostly does
`instance_->context_->process_manager()->AllocateWorkerProcess()`. `instance_`
is the parent `EmbeddedWorkerInstance`. `context_` is a
`ServiceWorkerContextCore`, `process_manager()` is a
`ServiceWorkerProcessManager` which I think might actually do work (!).

ServiceWorkerProcessManager (B)
-------------------------------
Interacts with UI thread to keep RenderProcessHosts alive for service workers
and tracks candidate processes for each pattern.

`AllocateWorkerProcess()` calls a callback with a running process that can run a
script. Posts back to self on UI. First, tries to find an existing process that
could be used if allowed. I think this happens when the same pattern is already
in use for another tab, so e.g. two docs tabs would use the same service worker
process. Otherwise, calls `SiteInstance::CreateForURL()`, which makes a
`RenderProcessHost` and passes that to the callback.

StoragePartition (B)
--------------------
A `StoragePartition` is the view each child process has of the persistent state
inside the BrowserContext. This is used to implement isolated storage where a
renderer with isolated storage cannot see the cookies, localStorage, etc. that
normal web renderers have access to.

Interaction with Network Stack
==============================
Now, given all the above which sets up the service worker process, and lets a
renderer register for handling of a scope/pattern, how does the set up service
worker actually capture and delegate network requests, in particular for a
fetch?

The hookups start in `ResourceDispatcherHostImpl`.
`ResourceDispatcherHostImpl::ContinuePendingBeginRequest()` calls
`ServiceWorkerRequestHandler::InitializeHandler()`, and
`ResourceDispatcherHostImpl::BeginNavigationRequest()` calls
`ServiceWorkerRequestHandler::InitializeForNavigation()`. (They also hook
`ForeignFetchRequestHandler` in a similar way, I think.)

Additionally, `StoragePartitionImplMap::Get()` calls
`ServiceWorkerRequestHandler::CreateInterceptor` and
`ForeignFetchRequestHandler::CreateInterceptor`. (I don't exactly know what this
function is, but it's generally adding semi-global interceptors and protocol
handlers and so on.)

ServiceWorkerRequestHandler (B)
-------------------------------
In `InitializeHandler` for a given `net::URLRequest`, checks to see if a service
worker makes sense, and if it does makes a `ServiceWorkerRequestHandler` via
`ServiceWorkerProviderHost::CreateRequestHandler()`, and attaches it to the
`URLRequest` via `SetUserData`.

`CreateInterceptor`, creates a `ServiceWorkerRequestInterceptor`, and returns it
to `StoragePartitionImplMap` where it gets included into the list of
`request_interceptors` for the storage partition.

ServiceWorkerRequestInterceptor (B)
-----------------------------------
This is a simple forwarder overriding `MaybeInterceptRequest()` and if
`InitializeHandler` has been called (attaching the `ServiceWorkerRequestHandler`
via user data to the request) then it thunks back into
`ServiceWorkerRequestHandler::MaybeCreateJob`. This is only implemented in
classes derived from `ServiceWorkerRequestHandler` of which there seem to be
three, see `ServiceWorkerProviderHost::CreateRequestHandler()` below.

ServiceWorkerProviderHost (B)
-----------------------------
This class is the browser-side representation of a "service worker provider"
(TODO: I don't know what that means yet). There are two types of providers:

1. for a client (windows, dedicated workers, shared workers);

2. for hosting a running service worker.

For client providers, there's a provider per document/worker and the provider
has the same lifetime as that document/worker in the renderer process. The
provider hosts state that is scoped to an individual document/worker.

For running service worker providers, the provider observes resource loads made
by the service worker.

`CreateRequestHandler` creates one of three concrete subclasses of
`ServiceWorkerRequestHandler`:
- `ServiceWorkerURLTrackingRequestHandler`. This one seems to be some sort of
  edge case. TODO: Figure this out.
- `ServiceWorkerContextRequestHandler`. This one handles "requests from service
  workers". I think that means requests from the process that's running the
  service worker script code.
- `ServiceWorkerControlleeRequestHandler`. This one handles "requests from
  controlled documents". I think that means the document that's having its
  requests redirected to a service worker.

`ServiceWorkerControlleeRequestHandler::MaybeCreateJob()` seems like the core
one. It mainly creates a `ServiceWorkerURLRequestJob` (into `job_`) and then
continues to either `PrepareForMainResource()` or `PrepareForSubResource()`.

`PrepareForSubResource()` calls `MaybeForwardToServiceWorker()`, which checks if
the `ServiceWorkerVersion` has a fetch handler, and if it does calls
`ServiceWorkerURLRequestJob::ForwardToServiceWorker()`.

ServiceWorkerURLRequestJob (B)
------------------------------
`ForwardToServiceWorker()` calls `MaybeStartRequest()`, which calls
`StartRequest()`, which in the `FORWARD_TO_SERVICE_WORKER` case calls
`RequestBodyFileSizesResolved()`. This gets the `ServiceWorkerVersion` (the
worker), creates a `ServiceWorkerFetchDispatcher` for it, and `Run()`s it.

ServiceWorkerFetchDispatcher (B)
--------------------------------
`Run()` makes sure the worker is set up and then calls `StartWorker()`. This
calls `DispatchFetchEvent()` which generally calls
`ServiceWorkerVersion::StartRequest()`. It then users
`ServiceWorkerVersion::RegisterRequestCallback()` to be notified when a
`ServiceWorkerHostMsg_FetchEventResponse` message is received (into
`ServiceWorkerFetchDispatcher::ResponseCallback::Run()`).

Finally, it calls `mojom::ServiceWorkerEventDispatcher::DispatchFetchEvent`
which will call back into `OnFetchEventFinished()`.

`OnFetchEventFinished()` calls `ServiceWorkerVersion::FinishRequest()`

ServiceWorkerContextClient (R)
------------------------------
This is a renderer-side implementation of `mojom::ServiceWorkerEventDispatcher`.
`DispatchFetchEvent()` calls into
`blink::WebServiceWorkerContextProxy::dispatchFetchEvent()` which does ... some
blink stuff, and then I think eventually makes its way back to
`ServiceWorkerContextClient::didHandleFetchEvent()`. That in turn calls the
callback registered in `DispatchFetchEvent()` which calls back to the browser
in `ServiceWorkerFetchDispatcher::OnFetchEventFinished()`.


Possible approaches for Service Worker with a network process
=============================================================
TODO
