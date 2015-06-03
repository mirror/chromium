// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/media/router/media_router_mojo_impl.h"

#include "base/bind.h"
#include "base/guid.h"
#include "base/logging.h"
#include "base/observer_list.h"
#include "chrome/browser/media/router/media_router_mojo_impl_factory.h"
#include "chrome/browser/media/router/media_router_type_converters.h"
#include "chrome/browser/media/router/media_routes_observer.h"
#include "chrome/browser/media/router/media_sinks_observer.h"
#include "extensions/browser/process_manager.h"

#define DVLOG_WITH_INSTANCE(level) \
  DVLOG(level) << "MR #" << instance_id_ << ": "

#define DLOG_WITH_INSTANCE(level) DLOG(level) << "MR #" << instance_id_ << ": "

namespace media_router {
namespace {

// Converts the callback result of calling Mojo CreateRoute() into a local
// callback.
void CreateRouteFinished(const MediaSink::Id& sink_id,
                         const MediaRouteResponseCallback& callback,
                         interfaces::MediaRoutePtr media_route,
                         const mojo::String& error_text) {
  if (media_route.is_null()) {
    // An error occurred.
    DCHECK(!error_text.is_null());
    callback.Run(nullptr, !error_text.get().empty() ? error_text.get()
                                                    : "Unknown error.");
    return;
  }
  callback.Run(media_route.To<scoped_ptr<MediaRoute>>(), "");
}

// TODO(imcheng): We should handle failure in this case. One way is to invoke
// all pending requests with failure. (crbug.com/490787)
void EventPageWakeComplete(bool success) {
  if (!success)
    LOG(ERROR) << "An error encountered while waking the event page.";
}

}  // namespace

MediaRouterMojoImpl::MediaRouterMojoImpl(
    extensions::EventPageTracker* event_page_tracker)
    : event_page_tracker_(event_page_tracker),
      instance_id_(base::GenerateGUID()) {
  DCHECK(event_page_tracker_);
}

MediaRouterMojoImpl::~MediaRouterMojoImpl() {
  DCHECK(thread_checker_.CalledOnValidThread());
}

// static
void MediaRouterMojoImpl::BindToRequest(
    const std::string& extension_id,
    content::BrowserContext* context,
    mojo::InterfaceRequest<interfaces::MediaRouterObserver> request) {
  MediaRouterMojoImpl* impl =
      MediaRouterMojoImplFactory::GetApiForBrowserContext(context);
  DCHECK(impl);

  impl->BindToMojoRequest(request.Pass(), extension_id);
}

void MediaRouterMojoImpl::BindToMojoRequest(
    mojo::InterfaceRequest<interfaces::MediaRouterObserver> request,
    const std::string& extension_id) {
  DCHECK(thread_checker_.CalledOnValidThread());

  binding_.reset(
      new mojo::Binding<interfaces::MediaRouterObserver>(this, request.Pass()));
  binding_->set_error_handler(this);

  mojo_media_router_extension_id_ = extension_id;
}

// TODO(imcheng): If this occurs while there are pending requests, we should
// probably invoke them with failure. (crbug.com/490787)
void MediaRouterMojoImpl::OnConnectionError() {
  DCHECK(thread_checker_.CalledOnValidThread());

  mojo_media_router_.reset();
  binding_.reset();
  mojo_media_router_extension_id_.clear();
}

void MediaRouterMojoImpl::ProvideMediaRouter(
    interfaces::MediaRouterPtr media_router_ptr,
    const interfaces::MediaRouterObserver::ProvideMediaRouterCallback&
        callback) {
  DCHECK(thread_checker_.CalledOnValidThread());

  mojo_media_router_ = media_router_ptr.Pass();
  mojo_media_router_.set_error_handler(this);
  callback.Run(instance_id_);
  ExecutePendingRequests();
}

void MediaRouterMojoImpl::OnMessage(const mojo::String& route_id,
                                    const mojo::String& message) {
  // TODO(imcheng): Implement. (crbug.com/461815)
  DCHECK(thread_checker_.CalledOnValidThread());
  NOTIMPLEMENTED();
}

void MediaRouterMojoImpl::OnIssue(const interfaces::IssuePtr issue) {
  // TODO(imcheng): Implement. (crbug.com/461815)
  DCHECK(thread_checker_.CalledOnValidThread());
  NOTIMPLEMENTED();
}

void MediaRouterMojoImpl::OnSinksReceived(
    const mojo::String& media_source,
    mojo::Array<interfaces::MediaSinkPtr> sinks) {
  DCHECK(thread_checker_.CalledOnValidThread());

  DVLOG_WITH_INSTANCE(1) << "OnSinksReceived";
  std::vector<MediaSink> sinks_converted;
  sinks_converted.reserve(sinks.size());

  for (size_t i = 0; i < sinks.size(); ++i) {
    sinks_converted.push_back(sinks[i].To<MediaSink>());
  }

  auto it = sinks_observers_.find(media_source);
  if (it == sinks_observers_.end()) {
    DVLOG_WITH_INSTANCE(1)
        << "Received sink list without any active observers: " << media_source;
  } else {
    FOR_EACH_OBSERVER(MediaSinksObserver, *it->second,
                      OnSinksReceived(sinks_converted));
  }
}

void MediaRouterMojoImpl::OnRoutesUpdated(
    mojo::Array<interfaces::MediaRoutePtr> routes) {
  DCHECK(thread_checker_.CalledOnValidThread());

  DVLOG_WITH_INSTANCE(1) << "OnRoutesUpdated";

  std::vector<MediaRoute> routes_converted;
  routes_converted.reserve(routes.size());

  for (size_t i = 0; i < routes.size(); ++i) {
    routes_converted.push_back(routes[i].To<MediaRoute>());
  }

  FOR_EACH_OBSERVER(MediaRoutesObserver, routes_observers_,
                    OnRoutesUpdated(routes_converted));
}

void MediaRouterMojoImpl::CreateRoute(
    const MediaSource::Id& source_id,
    const MediaSink::Id& sink_id,
    const MediaRouteResponseCallback& callback) {
  DCHECK(thread_checker_.CalledOnValidThread());

  RunOrDefer(base::Bind(&MediaRouterMojoImpl::DoCreateRoute,
                        base::Unretained(this), source_id, sink_id, callback));
}

void MediaRouterMojoImpl::CloseRoute(const MediaRoute::Id& route_id) {
  DCHECK(thread_checker_.CalledOnValidThread());

  RunOrDefer(base::Bind(&MediaRouterMojoImpl::DoCloseRoute,
                        base::Unretained(this), route_id));
}

void MediaRouterMojoImpl::PostMessage(const MediaRoute::Id& route_id,
                                      const std::string& message) {
  DCHECK(thread_checker_.CalledOnValidThread());

  RunOrDefer(base::Bind(&MediaRouterMojoImpl::DoPostMessage,
                        base::Unretained(this), route_id, message));
}

void MediaRouterMojoImpl::ClearIssue(const Issue::Id& issue_id) {
  DCHECK(thread_checker_.CalledOnValidThread());

  RunOrDefer(base::Bind(&MediaRouterMojoImpl::DoClearIssue,
                        base::Unretained(this), issue_id));
}

void MediaRouterMojoImpl::RegisterMediaSinksObserver(
    MediaSinksObserver* observer) {
  DCHECK(thread_checker_.CalledOnValidThread());

  // Create an observer list for the media source and add |observer|
  // to it. Fail if |observer| is already registered.
  const std::string& source_id = observer->source().id();
  base::ObserverList<MediaSinksObserver>* observer_list =
      sinks_observers_.get(source_id);
  if (!observer_list) {
    observer_list = new base::ObserverList<MediaSinksObserver>;
    sinks_observers_.add(source_id, make_scoped_ptr(observer_list));
  } else {
    DCHECK(!observer_list->HasObserver(observer));
  }

  // We need to call DoStartObservingMediaSinks every time an observer is
  // added to ensure the observer will be notified with a fresh set of results.
  // TODO(imcheng): Implement caching. (crbug.com/492451)
  observer_list->AddObserver(observer);
  RunOrDefer(base::Bind(&MediaRouterMojoImpl::DoStartObservingMediaSinks,
                        base::Unretained(this), source_id));
}

void MediaRouterMojoImpl::UnregisterMediaSinksObserver(
    MediaSinksObserver* observer) {
  DCHECK(thread_checker_.CalledOnValidThread());

  const std::string& source_id = observer->source().id();
  auto* observer_list = sinks_observers_.get(source_id);
  if (!observer_list || !observer_list->HasObserver(observer)) {
    return;
  }

  // If we are removing the final observer for the source, then stop
  // observing sinks for it.
  // might_have_observers() is reliable here on the assumption that this call
  // is not inside the ObserverList iteration.
  observer_list->RemoveObserver(observer);
  if (!observer_list->might_have_observers()) {
    RunOrDefer(base::Bind(&MediaRouterMojoImpl::DoStopObservingMediaSinks,
                          base::Unretained(this), source_id));
    sinks_observers_.erase(source_id);
  }
}

void MediaRouterMojoImpl::RegisterMediaRoutesObserver(
    MediaRoutesObserver* observer) {
  DCHECK(thread_checker_.CalledOnValidThread());
  DCHECK(!routes_observers_.HasObserver(observer));

  RunOrDefer(base::Bind(&MediaRouterMojoImpl::DoStartObservingMediaRoutes,
                        base::Unretained(this)));
  routes_observers_.AddObserver(observer);
}

void MediaRouterMojoImpl::UnregisterMediaRoutesObserver(
    MediaRoutesObserver* observer) {
  if (!routes_observers_.HasObserver(observer))
    return;

  routes_observers_.RemoveObserver(observer);
  if (!routes_observers_.might_have_observers()) {
    RunOrDefer(base::Bind(&MediaRouterMojoImpl::DoStopObservingMediaRoutes,
                          base::Unretained(this)));
  }
}

void MediaRouterMojoImpl::AddIssuesObserver(IssuesObserver* observer) {
  // TODO(imcheng): Implement. (crbug.com/461815)
  NOTIMPLEMENTED();
}

void MediaRouterMojoImpl::RemoveIssuesObserver(IssuesObserver* observer) {
  // TODO(imcheng): Implement. (crbug.com/461815)
  NOTIMPLEMENTED();
}

void MediaRouterMojoImpl::DoCreateRoute(
    const MediaSource::Id& source_id,
    const MediaSink::Id& sink_id,
    const MediaRouteResponseCallback& callback) {
  DVLOG_WITH_INSTANCE(1) << "CreateRoute " << source_id << "=>" << sink_id;
  mojo_media_router_->CreateRoute(
      source_id, sink_id, base::Bind(&CreateRouteFinished, sink_id, callback));
}

void MediaRouterMojoImpl::DoCloseRoute(const MediaRoute::Id& route_id) {
  DVLOG_WITH_INSTANCE(1) << "CloseRoute " << route_id;
  mojo_media_router_->CloseRoute(route_id);
}

void MediaRouterMojoImpl::DoPostMessage(const MediaRoute::Id& route_id,
                                        const std::string& message) {
  DVLOG_WITH_INSTANCE(1) << "PostMessage " << route_id;
  mojo_media_router_->PostMessage(route_id, message);
}

void MediaRouterMojoImpl::DoClearIssue(const Issue::Id& issue_id) {
  DVLOG_WITH_INSTANCE(1) << "ClearIssue " << issue_id;
  mojo_media_router_->ClearIssue(issue_id);
}

void MediaRouterMojoImpl::DoStartObservingMediaSinks(
    const std::string& source_id) {
  DVLOG_WITH_INSTANCE(1) << "StartObservingMediaSinks: " << source_id;
  mojo_media_router_->StartObservingMediaSinks(source_id);
}

void MediaRouterMojoImpl::DoStopObservingMediaSinks(
    const std::string& source_id) {
  DVLOG_WITH_INSTANCE(1) << "StopObservingMediaSinks: " << source_id;
  mojo_media_router_->StopObservingMediaSinks(source_id);
}

void MediaRouterMojoImpl::DoStartObservingMediaRoutes() {
  DVLOG_WITH_INSTANCE(1) << "StartObservingMediaRoutes";
  mojo_media_router_->StartObservingMediaRoutes();
}

void MediaRouterMojoImpl::DoStopObservingMediaRoutes() {
  DVLOG_WITH_INSTANCE(1) << "StopObservingMediaRoutes";
  mojo_media_router_->StopObservingMediaRoutes();
}

void MediaRouterMojoImpl::EnqueueTask(const base::Closure& closure) {
  pending_requests_.push_back(closure);
  DVLOG_WITH_INSTANCE(2) << "EnqueueTask (queue-length="
                         << pending_requests_.size() << ")";
}

void MediaRouterMojoImpl::RunOrDefer(const base::Closure& request) {
  DCHECK(event_page_tracker_);

  if (mojo_media_router_extension_id_.empty()) {
    DVLOG_WITH_INSTANCE(1) << "Extension ID not known yet.";
    EnqueueTask(request);
  } else if (event_page_tracker_->IsEventPageSuspended(
                 mojo_media_router_extension_id_)) {
    DVLOG_WITH_INSTANCE(1) << "Waking event page.";
    EnqueueTask(request);
    if (!event_page_tracker_->WakeEventPage(
            mojo_media_router_extension_id_,
            base::Bind(&EventPageWakeComplete))) {
      LOG(ERROR) << "An error encountered while waking the event page.";
    }
    mojo_media_router_.reset();
  } else if (!mojo_media_router_) {
    DVLOG_WITH_INSTANCE(1) << "Extension is awake, awaiting ProvideMediaRouter "
                              " to be called.";
    EnqueueTask(request);
  } else {
    request.Run();
  }
}

void MediaRouterMojoImpl::ExecutePendingRequests() {
  DCHECK(thread_checker_.CalledOnValidThread());
  DCHECK(mojo_media_router_);
  DCHECK(event_page_tracker_);
  DCHECK(!mojo_media_router_extension_id_.empty());

  if (event_page_tracker_->IsEventPageSuspended(
          mojo_media_router_extension_id_)) {
    DVLOG_WITH_INSTANCE(1)
        << "ExecutePendingRequests was called while extension is suspended.";
    return;
  }

  for (const auto& next_request : pending_requests_)
    next_request.Run();

  pending_requests_.clear();
}

}  // namespace media_router
