// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/media/router/mojo/wired_display_media_route_provider.h"

#include <string>
#include <utility>
#include <vector>

#include "chrome/browser/media/router/offscreen_presentation_manager.h"
#include "chrome/browser/media/router/offscreen_presentation_manager_factory.h"
#include "chrome/browser/media/router/receiver_presentation_service_delegate_impl.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/ui/browser.h"
#include "chrome/browser/ui/browser_window.h"
#include "chrome/browser/ui/tabs/tab_strip_model.h"
#include "chrome/common/media_router/route_request_result.h"
#include "content/public/browser/navigation_controller.h"
#include "content/public/browser/render_view_host.h"
#include "content/public/browser/web_contents.h"
#include "content/public/common/web_preferences.h"
#include "ui/display/display.h"
#include "ui/display/screen.h"

namespace media_router {

namespace {

bool IsPresentationSource(const std::string& media_source) {
  const GURL source_url(media_source);
  return source_url.is_valid() &&
         (source_url.SchemeIs("http") || source_url.SchemeIs("https"));
}

MediaSinkInternal CreateSinkForDisplay(const display::Display& display) {
  MediaSink sink;
  MediaSinkInternal sink_internal;
  sink.set_sink_id(std::to_string(display.id()));
  sink.set_name(std::to_string(display.id()));
  sink_internal.set_sink_id(std::to_string(display.id()));
  sink_internal.set_name(std::to_string(display.id()));
  sink_internal.set_sink(sink);
  return sink_internal;
}

}  // namespace

// static
const mojom::MediaRouteProvider::Id
    WiredDisplayMediaRouteProvider::kProviderId =
        mojom::MediaRouteProvider::Id::WIRED_DISPLAY;

WiredDisplayMediaRouteProvider::WiredDisplayMediaRouteProvider(
    mojom::MediaRouteProviderRequest request,
    mojom::MediaRouterPtr media_router,
    Profile* profile)
    : binding_(this, std::move(request)),
      media_router_(std::move(media_router)),
      profile_(profile) {
  OffscreenPresentationManagerFactory::GetOrCreateForBrowserContext(profile_)
      ->AddObserver(this);
}

WiredDisplayMediaRouteProvider::~WiredDisplayMediaRouteProvider() {
  OffscreenPresentationManagerFactory::GetOrCreateForBrowserContext(profile_)
      ->RemoveObserver(this);
}

void WiredDisplayMediaRouteProvider::CreateRoute(
    const std::string& media_source,
    const std::string& sink_id,
    const std::string& presentation_id,
    const url::Origin& origin,
    int32_t tab_id,
    base::TimeDelta timeout,
    bool incognito,
    CreateRouteCallback callback) {
  CreatePresentation(media_source, sink_id, presentation_id);
  const MediaRoute::Id route_id =
      presentation_id + "_" + std::to_string(route_index_++);
  routes_[route_id] = MediaRoute(route_id, MediaSource(media_source), sink_id,
                                 "", true, "", true);
  routes_[route_id].set_offscreen_presentation(true);
  routes_to_presentations_[route_id] = presentation_id;

  std::move(callback).Run(routes_[route_id], base::nullopt,
                          RouteRequestResult::OK);
  NotifyRouteObservers();
}

void WiredDisplayMediaRouteProvider::JoinRoute(
    const std::string& media_source,
    const std::string& presentation_id,
    const url::Origin& origin,
    int32_t tab_id,
    base::TimeDelta timeout,
    bool incognito,
    JoinRouteCallback callback) {
  std::move(callback).Run(
      base::nullopt,
      base::make_optional<std::string>(
          "Join should be handled by the presentation manager"),
      RouteRequestResult::UNKNOWN_ERROR);
}

void WiredDisplayMediaRouteProvider::ConnectRouteByRouteId(
    const std::string& media_source,
    const std::string& route_id,
    const std::string& presentation_id,
    const url::Origin& origin,
    int32_t tab_id,
    base::TimeDelta timeout,
    bool incognito,
    ConnectRouteByRouteIdCallback callback) {
  std::move(callback).Run(
      base::nullopt,
      base::make_optional<std::string>(
          "Connect should be handled by the presentation manager"),
      RouteRequestResult::UNKNOWN_ERROR);
}

void WiredDisplayMediaRouteProvider::TerminateRoute(
    const std::string& route_id,
    TerminateRouteCallback callback) {
  const std::string& presentation_id = routes_to_presentations_[route_id];
  DCHECK(!presentation_id.empty());
  presentation_web_contents_[presentation_id]->ClosePage();
  // TODO: erase from maps when done closing the window
  // TODO: may need to terminate all the routes associated with the presentation
  // ID
  routes_to_presentations_.erase(route_id);
  routes_.erase(route_id);
  NotifyRouteObservers();
  std::move(callback).Run(base::nullopt, RouteRequestResult::OK);
}

void WiredDisplayMediaRouteProvider::SendRouteMessage(
    const std::string& media_route_id,
    const std::string& message,
    SendRouteMessageCallback callback) {
  // Messages should be handled by LocalPresentationManager.
  std::move(callback).Run(false);
}

void WiredDisplayMediaRouteProvider::SendRouteBinaryMessage(
    const std::string& media_route_id,
    const std::vector<uint8_t>& data,
    SendRouteBinaryMessageCallback callback) {
  // Messages should be handled by LocalPresentationManager.
  std::move(callback).Run(false);
}

void WiredDisplayMediaRouteProvider::StartObservingMediaSinks(
    const std::string& media_source) {
  if (IsPresentationSource(media_source))
    sink_queries_.insert(media_source);
  UpdateMediaSinks(media_source);
}

void WiredDisplayMediaRouteProvider::StopObservingMediaSinks(
    const std::string& media_source) {
  sink_queries_.erase(media_source);
}

void WiredDisplayMediaRouteProvider::StartObservingMediaRoutes(
    const std::string& media_source) {
  route_queries_.insert(media_source);
  std::vector<MediaRoute> route_list;
  for (const auto& route : routes_)
    route_list.push_back(route.second);
  media_router_->OnRoutesUpdated(kProviderId, route_list, media_source, {});
}

void WiredDisplayMediaRouteProvider::StopObservingMediaRoutes(
    const std::string& media_source) {
  route_queries_.erase(media_source);
}

void WiredDisplayMediaRouteProvider::StartListeningForRouteMessages(
    const std::string& route_id) {
  // Messages should be handled by LocalPresentationManager.
}

void WiredDisplayMediaRouteProvider::StopListeningForRouteMessages(
    const std::string& route_id) {
  // Messages should be handled by LocalPresentationManager.
}

void WiredDisplayMediaRouteProvider::DetachRoute(const std::string& route_id) {
  // Detaching should be handled by LocalPresentationManager.
  NOTREACHED();
}

void WiredDisplayMediaRouteProvider::EnableMdnsDiscovery() {}

void WiredDisplayMediaRouteProvider::UpdateMediaSinks(
    const std::string& media_source) {
  if (IsPresentationSource(media_source)) {
    media_router_->OnSinksReceived(kProviderId, media_source, GetSinks(), {});
  } else {
    media_router_->OnSinksReceived(kProviderId, media_source, {}, {});
  }
}

void WiredDisplayMediaRouteProvider::SearchSinks(
    const std::string& sink_id,
    const std::string& media_source,
    mojom::SinkSearchCriteriaPtr search_criteria,
    SearchSinksCallback callback) {
  std::move(callback).Run(IsPresentationSource(media_source) &&
                                  true /* sink_id is in the list of sinks */
                              ? sink_id
                              : "");
}

void WiredDisplayMediaRouteProvider::ProvideSinks(
    const std::string& provider_name,
    const std::vector<media_router::MediaSinkInternal>& sinks) {
  NOTREACHED();
}

void WiredDisplayMediaRouteProvider::CreateMediaRouteController(
    const std::string& route_id,
    mojom::MediaControllerRequest media_controller,
    mojom::MediaStatusObserverPtr observer,
    CreateMediaRouteControllerCallback callback) {
  // Local screens do not support media controls.
  std::move(callback).Run(false);
}

void WiredDisplayMediaRouteProvider::OnDisplayAdded(
    const display::Display& new_display) {
  NotifySinkObservers();
}

void WiredDisplayMediaRouteProvider::OnDisplayRemoved(
    const display::Display& old_display) {
  NotifySinkObservers();
}

void WiredDisplayMediaRouteProvider::OnPendingControllersUpdated() {
  NotifySinkObservers();
}

void WiredDisplayMediaRouteProvider::CreatePresentation(
    const std::string& media_source,
    const std::string& sink_id,
    const std::string& presentation_id) {
  // TODO: all this should be handled by LocalPresentationWindow

  content::WebContents* web_contents = content::WebContents::Create(
      content::WebContents::CreateParams(profile_));
  presentation_web_contents_[presentation_id] = web_contents;

  ReceiverPresentationServiceDelegateImpl::CreateForWebContents(
      web_contents, presentation_id);
  if (auto* render_view_host = web_contents->GetRenderViewHost()) {
    auto web_prefs = render_view_host->GetWebkitPreferences();
    web_prefs.presentation_receiver = true;
    render_view_host->UpdateWebkitPreferences(web_prefs);
  }

  content::NavigationController::LoadURLParams load_params(
      (GURL(media_source)));
  load_params.should_replace_current_entry = true;
  load_params.should_clear_history_list = true;
  web_contents->GetController().LoadURLWithParams(load_params);

  Browser::CreateParams browser_params(profile_, true);
  browser_params.initial_bounds = gfx::Rect(500, 500);
  // browser_params.initial_show_state = ui::SHOW_STATE_FULLSCREEN;
  browser_params.type = Browser::TYPE_POPUP;
  browser_params.trusted_source = true;
  presentations_[presentation_id] = base::MakeUnique<Browser>(browser_params);
  presentations_[presentation_id]->tab_strip_model()->AddWebContents(
      web_contents, -1, ui::PAGE_TRANSITION_AUTO_TOPLEVEL,
      TabStripModel::ADD_ACTIVE);
  presentations_[presentation_id]->window()->Show();
}

void WiredDisplayMediaRouteProvider::NotifyRouteObservers() const {
  std::vector<MediaRoute> route_list;
  for (const auto& route : routes_)
    route_list.push_back(route.second);
  for (const std::string& route_query : route_queries_)
    media_router_->OnRoutesUpdated(kProviderId, route_list, route_query, {});
}

void WiredDisplayMediaRouteProvider::NotifySinkObservers() const {
  std::vector<MediaSinkInternal> sinks = GetSinks();
  for (const std::string& sink_query : sink_queries_)
    media_router_->OnSinksReceived(kProviderId, sink_query, sinks, {});
}

std::vector<MediaSinkInternal> WiredDisplayMediaRouteProvider::GetSinks()
    const {
  const std::vector<display::Display>& displays =
      display::Screen::GetScreen()->GetAllDisplays();
  std::vector<MediaSinkInternal> sinks;

  for (const display::Display& display : displays) {
    if (!display.IsInternal() &&
        !OffscreenPresentationManagerFactory::GetOrCreateForBrowserContext(
             profile_)
             ->HasPendingController(display.id())) {
      sinks.push_back(CreateSinkForDisplay(display));
    }
  }
  return sinks;
}

}  // namespace media_router
