// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/media/router/mojo/wired_display_media_route_provider.h"

#include <string>
#include <utility>
#include <vector>

#include "base/i18n/number_formatting.h"
#include "chrome/browser/media/router/offscreen_presentation_manager.h"
#include "chrome/browser/media/router/offscreen_presentation_manager_factory.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/common/media_router/route_request_result.h"
#include "chrome/grit/generated_resources.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/display/display.h"
#include "ui/display/screen.h"

namespace media_router {

namespace {

bool IsPresentationSource(const std::string& media_source) {
  const GURL source_url(media_source);
  return source_url.is_valid() &&
         (source_url.SchemeIs("http") || source_url.SchemeIs("https"));
}

MediaSinkInternal CreateSinkForDisplay(const display::Display& display,
                                       int display_index) {
  const std::string sink_id = WiredDisplayMediaRouteProvider::kSinkPrefix +
                              std::to_string(display.id());
  const std::string sink_name =
      l10n_util::GetStringFUTF8(IDS_MEDIA_ROUTER_WIRED_DISPLAY_SINK_NAME,
                                base::FormatNumber(display_index));
  MediaSink sink(sink_id, sink_name, SinkIconType::GENERIC);
  MediaSinkInternal sink_internal;
  sink_internal.set_sink_id(sink_id);
  sink_internal.set_name(sink_name);
  sink_internal.set_sink(sink);
  return sink_internal;
}

// Returns true if the top-left corner of |display1| is above that of
// |display2|, or if they are at the same height and |display1| is to the left
// of |display2|.
bool CompareDisplayBounds(const display::Display& display1,
                          const display::Display& display2) {
  return display1.bounds().y() < display2.bounds().y() ||
         (display1.bounds().y() == display2.bounds().y() &&
          display1.bounds().x() < display2.bounds().x());
}

}  // namespace

// static
const mojom::MediaRouteProvider::Id
    WiredDisplayMediaRouteProvider::kProviderId =
        mojom::MediaRouteProvider::Id::WIRED_DISPLAY;

// static
const char WiredDisplayMediaRouteProvider::kSinkPrefix[] = "wired_display_";

WiredDisplayMediaRouteProvider::WiredDisplayMediaRouteProvider(
    mojom::MediaRouteProviderRequest request,
    mojom::MediaRouterPtr media_router,
    Profile* profile)
    : binding_(this, std::move(request)),
      media_router_(std::move(media_router)),
      profile_(profile),
      presentation_manager_(
          OffscreenPresentationManagerFactory::GetOrCreateForBrowserContext(
              profile_)) {
  presentation_manager_->AddObserver(this);
  display::Screen::GetScreen()->AddObserver(this);
}

WiredDisplayMediaRouteProvider::~WiredDisplayMediaRouteProvider() {
  presentation_manager_->RemoveObserver(this);
  display::Screen::GetScreen()->RemoveObserver(this);
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
  DCHECK(routes_.find(presentation_id) == routes_.end());
  // Use |presentation_id| as the route ID. This MRP creates only one route per
  // presentation ID.
  routes_[presentation_id] = MediaRoute(
      presentation_id, MediaSource(media_source), sink_id, "", true, "", true);
  routes_[presentation_id].set_offscreen_presentation(true);

  // TODO(crbug.com/777654): Create a presentation receiver window.
  std::move(callback).Run(routes_[presentation_id], base::nullopt,
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
  base::flat_map<size_t, display::Display> displays = GetAvailableDisplays();
  bool valid_id =
      std::find_if(
          displays.begin(), displays.end(), [&sink_id](const auto& display) {
            return kSinkPrefix + std::to_string(display.second.id()) == sink_id;
          }) != displays.end();
  std::move(callback).Run(
      IsPresentationSource(media_source) && valid_id ? sink_id : "");
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

void WiredDisplayMediaRouteProvider::OnDidProcessDisplayChanges() {
  NotifySinkObservers();
}

void WiredDisplayMediaRouteProvider::OnDisplayAdded(
    const display::Display& new_display) {
  NotifySinkObservers();
}

void WiredDisplayMediaRouteProvider::OnDisplayRemoved(
    const display::Display& old_display) {
  NotifySinkObservers();
}

void WiredDisplayMediaRouteProvider::OnDisplayMetricsChanged(
    const display::Display& display,
    uint32_t changed_metrics) {
  NotifySinkObservers();
}

void WiredDisplayMediaRouteProvider::OnPendingControllersUpdated() {
  NotifySinkObservers();
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
  std::vector<MediaSinkInternal> sinks;
  base::flat_map<size_t, display::Display> displays = GetAvailableDisplays();
  for (const auto& display : displays)
    sinks.push_back(CreateSinkForDisplay(display.second, display.first));
  return sinks;
}

base::flat_map<size_t, display::Display>
WiredDisplayMediaRouteProvider::GetAvailableDisplays() const {
  std::vector<display::Display> all_displays =
      display::Screen::GetScreen()->GetAllDisplays();
  std::sort(all_displays.begin(), all_displays.end(), CompareDisplayBounds);
  base::flat_set<gfx::Point> internal_display_origins;
  for (const display::Display& display : all_displays) {
    if (display.IsInternal())
      internal_display_origins.insert(display.bounds().origin());
  }
  // Get external displays that have no pending controller and do not mirror
  // an internal display.
  base::flat_map<size_t, display::Display> displays;
  for (size_t i = 0; i < all_displays.size(); i++) {
    const display::Display& display = all_displays[i];
    if (!display.IsInternal() &&
        !presentation_manager_->HasPendingController(display.id()) &&
        internal_display_origins.find(display.bounds().origin()) ==
            internal_display_origins.end()) {
      displays.insert({i + 1, display});
    }
  }
  return displays;
}

}  // namespace media_router
