// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_MEDIA_ROUTER_MOJO_MOCK_MEDIA_ROUTE_PROVIDER_H_
#define CHROME_BROWSER_MEDIA_ROUTER_MOJO_MOCK_MEDIA_ROUTE_PROVIDER_H_

#include <string>

#include "base/macros.h"
#include "chrome/common/media_router/mojo/media_router.mojom.h"
#include "testing/gmock/include/gmock/gmock.h"

namespace media_router {

class MockMediaRouteProvider : public mojom::MediaRouteProvider {
 public:
  using RouteCallback =
      base::OnceCallback<void(const base::Optional<MediaRoute>&,
                              const base::Optional<std::string>&,
                              RouteRequestResult::ResultCode)>;

  MockMediaRouteProvider();
  ~MockMediaRouteProvider() override;

  void CreateRoute(const std::string& source_urn,
                   const std::string& sink_id,
                   const std::string& presentation_id,
                   const url::Origin& origin,
                   int tab_id,
                   base::TimeDelta timeout,
                   bool incognito,
                   CreateRouteCallback callback) {
    CreateRouteInternal(source_urn, sink_id, presentation_id, origin, tab_id,
                        timeout, incognito, callback);
  }
  MOCK_METHOD8(CreateRouteInternal,
               void(const std::string& source_urn,
                    const std::string& sink_id,
                    const std::string& presentation_id,
                    const url::Origin& origin,
                    int tab_id,
                    base::TimeDelta timeout,
                    bool incognito,
                    CreateRouteCallback& callback));
  void JoinRoute(const std::string& source_urn,
                 const std::string& presentation_id,
                 const url::Origin& origin,
                 int tab_id,
                 base::TimeDelta timeout,
                 bool incognito,
                 JoinRouteCallback callback) {
    JoinRouteInternal(source_urn, presentation_id, origin, tab_id, timeout,
                      incognito, callback);
  }
  MOCK_METHOD7(JoinRouteInternal,
               void(const std::string& source_urn,
                    const std::string& presentation_id,
                    const url::Origin& origin,
                    int tab_id,
                    base::TimeDelta timeout,
                    bool incognito,
                    JoinRouteCallback& callback));
  void ConnectRouteByRouteId(const std::string& source_urn,
                             const std::string& route_id,
                             const std::string& presentation_id,
                             const url::Origin& origin,
                             int tab_id,
                             base::TimeDelta timeout,
                             bool incognito,
                             JoinRouteCallback callback) {
    ConnectRouteByRouteIdInternal(source_urn, route_id, presentation_id, origin,
                                  tab_id, timeout, incognito, callback);
  }
  MOCK_METHOD8(ConnectRouteByRouteIdInternal,
               void(const std::string& source_urn,
                    const std::string& route_id,
                    const std::string& presentation_id,
                    const url::Origin& origin,
                    int tab_id,
                    base::TimeDelta timeout,
                    bool incognito,
                    JoinRouteCallback& callback));
  MOCK_METHOD1(DetachRoute, void(const std::string& route_id));
  void TerminateRoute(const std::string& route_id,
                      TerminateRouteCallback callback) {
    TerminateRouteInternal(route_id, callback);
  }
  MOCK_METHOD2(TerminateRouteInternal,
               void(const std::string& route_id,
                    TerminateRouteCallback& callback));
  MOCK_METHOD1(StartObservingMediaSinks, void(const std::string& source));
  MOCK_METHOD1(StopObservingMediaSinks, void(const std::string& source));
  void SendRouteMessage(const std::string& media_route_id,
                        const std::string& message,
                        SendRouteMessageCallback callback) {
    SendRouteMessageInternal(media_route_id, message, callback);
  }
  MOCK_METHOD3(SendRouteMessageInternal,
               void(const std::string& media_route_id,
                    const std::string& message,
                    SendRouteMessageCallback& callback));
  void SendRouteBinaryMessage(const std::string& media_route_id,
                              const std::vector<uint8_t>& data,
                              SendRouteMessageCallback callback) override {
    SendRouteBinaryMessageInternal(media_route_id, data, callback);
  }
  MOCK_METHOD3(SendRouteBinaryMessageInternal,
               void(const std::string& media_route_id,
                    const std::vector<uint8_t>& data,
                    SendRouteMessageCallback& callback));
  MOCK_METHOD1(StartListeningForRouteMessages,
               void(const std::string& route_id));
  MOCK_METHOD1(StopListeningForRouteMessages,
               void(const std::string& route_id));
  MOCK_METHOD1(OnPresentationSessionDetached,
               void(const std::string& route_id));
  MOCK_METHOD1(StartObservingMediaRoutes, void(const std::string& source));
  MOCK_METHOD1(StopObservingMediaRoutes, void(const std::string& source));
  MOCK_METHOD0(EnableMdnsDiscovery, void());
  MOCK_METHOD1(UpdateMediaSinks, void(const std::string& source));
  void SearchSinks(const std::string& sink_id,
                   const std::string& media_source,
                   mojom::SinkSearchCriteriaPtr search_criteria,
                   SearchSinksCallback callback) override {
    SearchSinksInternal(sink_id, media_source, search_criteria, callback);
  }
  MOCK_METHOD4(SearchSinksInternal,
               void(const std::string& sink_id,
                    const std::string& media_source,
                    mojom::SinkSearchCriteriaPtr& search_criteria,
                    SearchSinksCallback& callback));
  MOCK_METHOD2(ProvideSinks,
               void(const std::string&, const std::vector<MediaSinkInternal>&));
  void CreateMediaRouteController(
      const std::string& route_id,
      mojom::MediaControllerRequest media_controller,
      mojom::MediaStatusObserverPtr observer,
      CreateMediaRouteControllerCallback callback) override {
    CreateMediaRouteControllerInternal(route_id, media_controller, observer,
                                       callback);
  }
  MOCK_METHOD4(CreateMediaRouteControllerInternal,
               void(const std::string& route_id,
                    mojom::MediaControllerRequest& media_controller,
                    mojom::MediaStatusObserverPtr& observer,
                    CreateMediaRouteControllerCallback& callback));

  // These methods execute the callbacks with the success or timeout result
  // code. If the callback takes a route, the route set in SetRouteToReturn() is
  // used.
  void RouteRequestSuccess(RouteCallback& cb) const;
  void RouteRequestTimeout(RouteCallback& cb) const;
  void TerminateRouteSuccess(TerminateRouteCallback& cb) const;
  void SendRouteMessageSuccess(SendRouteMessageCallback& cb) const;
  void SendRouteBinaryMessageSuccess(SendRouteBinaryMessageCallback& cb) const;
  void SearchSinksSuccess(SearchSinksCallback& cb) const;
  void CreateMediaRouteControllerSuccess(
      CreateMediaRouteControllerCallback& cb) const;

  // Sets the route to pass into callbacks.
  void SetRouteToReturn(const MediaRoute& route);

 private:
  // The route that is passed into callbacks.
  base::Optional<MediaRoute> route_;

  DISALLOW_COPY_AND_ASSIGN(MockMediaRouteProvider);
};

}  // namespace media_router

#endif  // CHROME_BROWSER_MEDIA_ROUTER_MOJO_MOCK_MEDIA_ROUTE_PROVIDER_H_
