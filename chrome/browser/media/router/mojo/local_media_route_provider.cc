// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/media/router/mojo/local_media_route_provider.h"

#include <string>
#include <utility>
#include <vector>

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

namespace {

bool IsPresentationSource(const std::string& media_source) {
  const GURL source_url(media_source);
  return source_url.is_valid() &&
         (source_url.SchemeIs("http") || source_url.SchemeIs("https"));
}

}  // namespace

namespace media_router {

// static
const char LocalMediaRouteProvider::kProviderName[] = "local";

LocalMediaRouteProvider::LocalMediaRouteProvider(
    mojom::MediaRouteProviderRequest request,
    mojom::MediaRouterPtr media_router,
    Profile* profile)
    : binding_(this, std::move(request)),
      media_router_(std::move(media_router)),
      profile_(profile) {}

LocalMediaRouteProvider::~LocalMediaRouteProvider() = default;

void LocalMediaRouteProvider::CreateRoute(const std::string& media_source,
                                          const std::string& sink_id,
                                          const std::string& presentation_id,
                                          const url::Origin& origin,
                                          int32_t tab_id,
                                          base::TimeDelta timeout,
                                          bool incognito,
                                          CreateRouteCallback callback) {
  LOG(ERROR) << "______ create route " << media_source;
  CreatePresentation(media_source, sink_id, presentation_id);
  const MediaRoute::Id route_id =
      presentation_id + "_" + std::to_string(route_index_++);
  routes_[route_id] = MediaRoute(route_id, MediaSource(media_source), sink_id,
                                 "description", true, "", true);
  routes_[route_id].set_offscreen_presentation(true);
  routes_to_presentations_[route_id] = presentation_id;

  std::move(callback).Run(routes_[route_id], base::nullopt,
                          RouteRequestResult::OK);
  NotifyRouteObservers();
}

void LocalMediaRouteProvider::JoinRoute(const std::string& media_source,
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

void LocalMediaRouteProvider::ConnectRouteByRouteId(
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

void LocalMediaRouteProvider::TerminateRoute(const std::string& route_id,
                                             TerminateRouteCallback callback) {
  LOG(ERROR) << "___________ TerminateRoute()";
  const std::string& presentation_id = routes_to_presentations_[route_id];
  DCHECK(!presentation_id.empty());
  presentation_web_contents_.at(presentation_id)->ClosePage();
  // TODO: erase from maps when done closing the window
  // TODO: may need to terminate all the routes associated with the presentation
  // ID
  routes_to_presentations_.erase(route_id);
  routes_.erase(route_id);
  NotifyRouteObservers();
  std::move(callback).Run(base::nullopt, RouteRequestResult::OK);
}

void LocalMediaRouteProvider::SendRouteMessage(
    const std::string& media_route_id,
    const std::string& message,
    SendRouteMessageCallback callback) {
  // Messages should be handled by LocalPresentationManager.
  std::move(callback).Run(false);
}

void LocalMediaRouteProvider::SendRouteBinaryMessage(
    const std::string& media_route_id,
    const std::vector<uint8_t>& data,
    SendRouteBinaryMessageCallback callback) {
  // Messages should be handled by LocalPresentationManager.
  std::move(callback).Run(false);
}

void LocalMediaRouteProvider::StartObservingMediaSinks(
    const std::string& media_source) {
  if (IsPresentationSource(media_source))
    sink_queries_.insert(media_source);
  UpdateMediaSinks(media_source);
}

void LocalMediaRouteProvider::StopObservingMediaSinks(
    const std::string& media_source) {
  sink_queries_.erase(media_source);
}

void LocalMediaRouteProvider::StartObservingMediaRoutes(
    const std::string& media_source) {
  route_queries_.insert(media_source);
  // TODO: populate joinable routes?
  std::vector<MediaRoute> route_list;
  for (const auto& route : routes_)
    route_list.push_back(route.second);
  media_router_->OnRoutesUpdated(kProviderName, route_list, media_source, {});
}

void LocalMediaRouteProvider::StopObservingMediaRoutes(
    const std::string& media_source) {
  route_queries_.erase(media_source);
}

void LocalMediaRouteProvider::StartListeningForRouteMessages(
    const std::string& route_id) {}

void LocalMediaRouteProvider::StopListeningForRouteMessages(
    const std::string& route_id) {}

void LocalMediaRouteProvider::DetachRoute(const std::string& route_id) {
  // Detaching should be handled by LocalPresentationManager.
  NOTREACHED();
}

void LocalMediaRouteProvider::EnableMdnsDiscovery() {}

void LocalMediaRouteProvider::UpdateMediaSinks(
    const std::string& media_source) {
  if (IsPresentationSource(media_source)) {
    // TODO: return actual sinks
    LOG(ERROR) << "source: " << media_source;

    MediaSink sink;
    MediaSinkInternal sink_internal;
    sink.set_sink_id("local1");
    sink.set_name("Display 1");
    sink_internal.set_sink_id("local1");
    sink_internal.set_name("Display 1");
    sink_internal.set_sink(sink);

    MediaSink sink2;
    MediaSinkInternal sink_internal2;
    sink2.set_sink_id("local2");
    sink2.set_name("Display 2");
    sink_internal2.set_sink_id("local2");
    sink_internal2.set_name("Display 2");
    sink_internal2.set_sink(sink2);

    media_router_->OnSinksReceived(kProviderName, media_source,
                                   {sink_internal, sink_internal2}, {});
  } else {
    media_router_->OnSinksReceived(kProviderName, media_source, {}, {});
  }
}

void LocalMediaRouteProvider::SearchSinks(
    const std::string& sink_id,
    const std::string& media_source,
    mojom::SinkSearchCriteriaPtr search_criteria,
    SearchSinksCallback callback) {
  LOG(ERROR) << "SearchSinks()";
  std::move(callback).Run(IsPresentationSource(media_source) &&
                                  true /* sink_id is in the list of sinks */
                              ? sink_id
                              : "");
}

void LocalMediaRouteProvider::ProvideSinks(
    const std::string& provider_name,
    const std::vector<media_router::MediaSinkInternal>& sinks) {
  NOTREACHED();
}

void LocalMediaRouteProvider::CreateMediaRouteController(
    const std::string& route_id,
    mojom::MediaControllerRequest media_controller,
    mojom::MediaStatusObserverPtr observer,
    CreateMediaRouteControllerCallback callback) {
  // Local screens do not support media controls.
  std::move(callback).Run(false);
}

void LocalMediaRouteProvider::OnDisplayAdded(const Display& new_display) {
  for (const std::string& sink_query : sink_queries_) {
    //
  }
}

void LocalMediaRouteProvider::OnDisplayRemoved(const Display& old_display) {
  for (const std::string& sink_query : sink_queries_) {
    //
  }
}

void LocalMediaRouteProvider::CreatePresentation(
    const std::string& media_source,
    const std::string& sink_id,
    const std::string& presentation_id) {
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
  browser_params.initial_show_state = ui::SHOW_STATE_FULLSCREEN;
  browser_params.type = Browser::TYPE_POPUP;
  browser_params.trusted_source = true;
  presentations_[presentation_id] = base::MakeUnique<Browser>(browser_params);
  presentations_.at(presentation_id)
      ->tab_strip_model()
      ->AddWebContents(web_contents, -1, ui::PAGE_TRANSITION_AUTO_TOPLEVEL,
                       TabStripModel::ADD_ACTIVE);
  presentations_.at(presentation_id)->window()->Show();

  // const std::vector<display::Display>& displays =
  //     display::Screen::GetScreen()->GetAllDisplays();

  // for (const auto& display : displays) {
  //   LOG(ERROR) << display.ToString();
  // }
}

void LocalMediaRouteProvider::NotifyRouteObservers() const {
  std::vector<MediaRoute> route_list;
  for (const auto& route : routes_)
    route_list.push_back(route.second);
  for (const std::string& route_query : route_queries_)
    media_router_->OnRoutesUpdated(kProviderName, route_list, route_query, {});
}

}  // namespace media_router
