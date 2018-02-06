// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_MEDIA_ROUTER_PROVIDERS_CAST_CAST_APP_DISCOVERY_SERVICE_H_
#define CHROME_BROWSER_MEDIA_ROUTER_PROVIDERS_CAST_CAST_APP_DISCOVERY_SERVICE_H_

#include <string>
#include <vector>

#include "base/callback.h"
#include "base/callback_list.h"
#include "base/containers/flat_map.h"
#include "base/containers/flat_set.h"
#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "base/sequenced_task_runner.h"
#include "chrome/browser/media/router/discovery/mdns/cast_media_sink_service_impl.h"
#include "chrome/browser/media/router/providers/cast/cast_app_availability_tracker.h"
#include "chrome/common/media_router/media_sink.h"
#include "chrome/common/media_router/media_source.h"
#include "chrome/common/media_router/providers/cast/cast_media_source.h"
#include "components/cast_channel/cast_message_util.h"

namespace cast_channel {
class CastMessageHandler;
class CastSocket;
}  // namespace cast_channel

namespace media_router {

// Keeps track of sink queries and listens to CastMediaSinkServiceImpl for sink
// updates, and issues app availability requests based on these signals. All
// methods must run on the same sequence.
class CastAppDiscoveryService {
 public:
  using SinkQueryFunc = void(const MediaSource::Id& source_id,
                             const std::vector<MediaSinkInternal>& sinks,
                             const std::vector<url::Origin>& origins);
  using SinkQueryCallback = base::RepeatingCallback<SinkQueryFunc>;
  using SinkQueryCallbackList = base::CallbackList<SinkQueryFunc>;
  using Subscription = std::unique_ptr<SinkQueryCallbackList::Subscription>;

  CastAppDiscoveryService(cast_channel::CastMessageHandler* message_handler,
                          cast_channel::CastSocketService* socket_service);
  ~CastAppDiscoveryService();

  // Adds a sink query for |source|. Results will be continuously returned via
  // |callback| until the returned Subscription is destroyed by the caller.
  Subscription StartObservingMediaSinks(const CastMediaSource& source,
                                        const SinkQueryCallback& callback);

  // Returns the CastMediaSinkServiceImpl::Observer instance associated with
  // |this|.
  CastMediaSinkServiceImpl::Observer* GetSinkDiscoveryObserver();

 private:
  // Inner part of CastAppDiscoveryService that runs on Cast socket / sink
  // discovery sequence. Also observes directly with CastMediaSinkServiceImpl
  // for sink updates. This class may be created on any sequence, but all other
  // methods (including the destructors) must run on the Cast socket sequence.
  class Core : public CastMediaSinkServiceImpl::Observer {
   public:
    explicit Core(cast_channel::CastMessageHandler* message_handler,
                  cast_channel::CastSocketService* socket_service,
                  const scoped_refptr<base::SequencedTaskRunner>& task_runner,
                  const base::WeakPtr<CastAppDiscoveryService>& outer);
    ~Core() override;

    // Adds a sink query given by |source|, issuing app availability requests
    // to known sinks as necessary. If there are previously cached results,
    // they will be returned to |outer_|.
    void AddSinkQuery(const CastMediaSource& source);

    // Removes the sink query given by |source_id|.
    void RemoveSinkQuery(const MediaSource::Id& source_id);

   private:
    // CastMediaSinkServiceImpl::Observer
    void OnSinkAddedOrUpdated(const MediaSinkInternal& sink,
                              cast_channel::CastSocket* socket) override;
    void OnSinkRemoved(const MediaSink::Id& sink_id) override;

    // Issues an app availability request for |app_id| to the sink given by
    // |sink_id| via |socket|.
    void RequestAppAvailability(cast_channel::CastSocket* socket,
                                const std::string& app_id,
                                const MediaSink::Id& sink_id);

    // Updates the availability result for |sink_id| and |app_id| with |result|,
    // and notifies |outer_| with updated sink query results.
    void UpdateAppAvailability(const MediaSink::Id& sink_id,
                               const std::string& app_id,
                               cast_channel::GetAppAvailabilityResult result);

    // Notifies |outer_| of sink query results for |sources|.
    void UpdateSinkQueries(const std::vector<CastMediaSource>& sources);

    // Posts task to |outer_| with sink query results given by |source_id| and
    // |sinks|.
    void PostTaskUpdateSinkQueryResults(
        const MediaSource::Id& source_id,
        const std::vector<MediaSinkInternal>& sinks);

    // Gets a list of sinks corresponding to |sink_ids|.
    std::vector<MediaSinkInternal> GetSinksByIds(
        const base::flat_set<MediaSink::Id>& sink_ids) const;

    cast_channel::CastMessageHandler* const message_handler_;
    cast_channel::CastSocketService* const socket_service_;

    // Task runner for |outer_|.
    scoped_refptr<base::SequencedTaskRunner> outer_task_runner_;

    // WeakPtr to the owning CastAppDiscoveryService.
    base::WeakPtr<CastAppDiscoveryService> outer_;

    CastAppAvailabilityTracker availability_tracker_;
    base::flat_map<MediaSink::Id, MediaSinkInternal> sinks_;

    base::WeakPtrFactory<Core> weak_ptr_factory_;
    DISALLOW_COPY_AND_ASSIGN(Core);
  };

  // Notifies sink query callbacks registered with |source| with results given
  // by |sinks| and |origins|.
  void UpdateSinkQueryResults(const MediaSource::Id& source_id,
                              const std::vector<MediaSinkInternal>& sinks,
                              const std::vector<url::Origin>& origins);

  // Removes the entry from |sink_queries_| if there are no more callbacks
  // associated with |source_id|.
  void MaybeRemoveSinkQueryEntry(const MediaSource::Id& source_id);

  // Task runner for |core_|.
  scoped_refptr<base::SequencedTaskRunner> core_task_runner_;

  // Registered sink queries and their associated callbacks.
  base::flat_map<MediaSource::Id, std::unique_ptr<SinkQueryCallbackList>>
      sink_queries_;

  std::unique_ptr<Core, base::OnTaskRunnerDeleter> core_;

  base::WeakPtrFactory<CastAppDiscoveryService> weak_ptr_factory_;
  DISALLOW_COPY_AND_ASSIGN(CastAppDiscoveryService);
};

}  // namespace media_router

#endif  // CHROME_BROWSER_MEDIA_ROUTER_PROVIDERS_CAST_CAST_APP_DISCOVERY_SERVICE_H_
