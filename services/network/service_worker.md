Service Worker basic tour
=========================

`WebFrameClient::createServiceWorkerProvider()` makes a
`WebServiceWorkerProvider`.

`RenderFrameImpl::createServiceWorkerProvider()` implements this. I think this
is the normal one that Chrome uses. It makes a `WebServiceWorkerProviderImpl`.

`ServiceWorkerContextClient::createServiceWorkerProvider()` also implements
this. It does `new WebServiceWorkerProviderImpl`. TODO: When is
`ServiceWorkerContextClient` used?

ServiceWorkerContainer
----------------------
`registerServiceWorker()` -> `registerServiceWorkerImpl()` -> does a bunch of
checks about origins and schemes and normalizing urls or something and then
calls `m_provider->registerServiceWorker()`, where `m_provider` is a
`WebServiceWorkerProvider` which will go up to a `WebServiceWorkerProviderImpl`
in chrome.

TODO: Not sure how this actually gets called, maybe straight from JS.


WebServiceWorkerProviderImpl
----------------------------
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

ServiceWorkerDispatcher
-----------------------
A ServiceWorkerDispatcher per thread (uses TLS). TODO: How many threads are
there?

This thing has `RegisterServiceWorker()` which gets called above (corresponding
`to navigator.serviceWorker.register()`) which saves the pending registration
and then sends back to the browser `ServiceWorkerHostMsg_RegisterServiceWorker`.

That gets back to the browser in `ServiceWorkerDispatcherHost`.

ServiceWorkerDispatcherHost
---------------------------
`ServiceWorkerDispatcherHost::OnRegisterServiceWorker()` does some validation
and either sends back a failure reason, or calls
`GetContext()->RegisterServiceWorker()` binding a completion to
`&RegistrationComplete`. `GetContext()` is a `ServiceWorkerContextCore`.

When `RegistrationComplete` is called, gets the
`ServiceWorkerProviderHost` for the render process id and provider id (TODO:
What is a provider id?) and if everything is still OK, replies to the renderer
with `ServiceWorkerMsg_ServiceWorkerRegistered`.

ServiceWorkerContextCore
------------------------
Manages data associated with services worker. Single threaded, IO thread only.

One instance per StoragePartition (TODO: What is a StoragePartition?). This is
the root of the hierarchy of service worker data for a particular partition.

It implements `ServiceWorkerVersion::Listener`.

`RegisterServiceWorker` calls `job_coordinator_->Register()` (on a
`ServiceWorkerJobCoordinator`) into `&RegistrationComplete` which calls back
into the callback that goes back to `ServiceWorkerDispatcherHost`.

ServiceWorkerJobCoordinator
---------------------------
"manages all in-flight registration or unregistration jobs". Don't know why it
needs to have a queue but it does `new ServiceWorkerRegisterJob` appends that
into a queue for the `pattern` (TODO: This is the script-provided pattern?) and
then sets the callback on that queued job which will call back into
`ServiceWorkerContextCore::RegistrationComplete`.

ServiceWorkerRegisterJob
------------------------
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

ServiceWorkerVersion
--------------------

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

ServiceWorkerStorage
--------------------
This is a subchunk of data that is conceptually part of
`ServiceWorkerContextCore`. It holds registration data (?) TODO: And maybe the
cache of data that the service worker stores?

`FindRegistrationForId()` handles some no-op cases or dispatches to
`database_task_manager_` which is a `ServiceWorkerDatabaseTaskManager` which
uses that task runner to run `&FindForIdInDB()` and then back to
`DidFindRegistrationForId()`.

`FindForIdInDB()` does `database->ReadRegistration()`. `database` is a
`ServiceWorkerDatabase`. Gets a `ServiceWorkerDatabase::RegistrationData` back
which is id, scope, script url, update times, has_fetch_handler, etc.

`DidFindRegistrationForId()` -> `ReturnFoundRegistration()` ->
`CompleteFindNow()` -> `callback`. I think this callback eventually gets back to
`ServiceWorkerVersion::DidEnsureLiveRegistrationForStartWorker()`

ServiceWorkerDatabase
---------------------
Persists service worker registration data in a database. Not on IO thread
because it writes files. Owned by `ServiceWorkerStorage` (which is in turn owned
by `ServiceWorkerContextCore`) and that is responsible for only calling it on a
worker pool thread. Uses level db.

EmbeddedWorkerRegistry
----------------------
Stub between `MessageFilter` and `EmbeddedWorkerInstance`. Owned by
`ServiceWorkerContextCore`. Lives on IO thread only. I'm not sure what this
class does exactly.

`CreateWorker()` creates an `EmbeddedWorkerInstance` which does most of the
work.

EmbeddedWorkerInstance
----------------------
Makes a `StartTask` and then calls `Start()` on that. That mostly does
`instance_->context_->process_manager()->AllocateWorkerProcess()`. `instance_`
is the parent `EmbeddedWorkerInstance`. `context_` is a
`ServiceWorkerContextCore`, `process_manager()` is a
`ServiceWorkerProcessManager` which I think might actually do work (!).

ServiceWorkerProcessManager
---------------------------
Interacts with UI thread to keep RenderProcessHosts alive for service workers
and tracks candidate processes for each pattern. 

`AllocateWorkerProcess()` calls a callback with a running process that can run a
script. Posts back to self on UI. First, tries to find an existing process that
could be used if allowed. I think this happens when the same pattern is already
in use for another tab, so e.g. two docs tabs would use the same service worker
process. Otherwise, calls `SiteInstance::CreateForURL()`

StoragePartition
----------------
A `StoragePartition` is the view each child process has of the persistent state
inside the BrowserContext. This is used to implement isolated storage where a
renderer with isolated storage cannot see the cookies, localStorage, etc. that
normal web renderers have access to.
