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

namespace cast_channel {
class CastMessageHandler;
}

namespace media_router {

// TODO(imcheng): Description
class CastAppDiscoveryService {
 public:
  using SinkQueryFunc = void(const MediaSource::Id& source_id,
                             const std::vector<MediaSinkInternal>& sinks,
                             const std::vector<url::Origin>& origins);
  using SinkQueryCallback = base::RepeatingCallback<SinkQueryFunc>;
  using SinkQueryCallbackList = base::CallbackList<SinkQueryFunc>;
  using Subscription = std::unique_ptr<SinkQueryCallbackList::Subscription>;

  CastAppDiscoveryService(
      const scoped_refptr<base::SequencedTaskRunner>& task_runner,
      cast_channel::CastMessageHandler* message_handler);
  ~CastAppDiscoveryService();

  Subscription StartObservingMediaSinks(const CastMediaSource& source,
                                        const SinkQueryCallback& callback);
  void OnSinkQueryUpdated(const MediaSource::Id& source_id,
                          const std::vector<MediaSinkInternal>& sinks,
                          const std::vector<url::Origin>& origins);

  CastMediaSinkServiceImpl::Observer* GetSinkDiscoveryObserver();

 private:
  // Runs on IO thread.
  class Core : public CastMediaSinkServiceImpl::Observer {
   public:
    explicit Core(cast_channel::CastMessageHandler* message_handler,
                  const scoped_refptr<base::SequencedTaskRunner>& task_runner,
                  const base::WeakPtr<CastAppDiscoveryService>& outer);
    ~Core() override;

    void AddSinkQuery(const CastMediaSource& source);
    void RemoveSinkQuery(const MediaSource::Id& source_id);

   private:
    void OnSinkAdded(const MediaSinkInternal& sink) override;
    void OnSinkUpdated(const MediaSinkInternal& sink) override;
    void OnSinkRemoved(const MediaSink::Id& sink_id) override;

    std::vector<MediaSinkInternal> GetSinksByIds(
        const base::flat_set<MediaSink::Id>& sink_ids) const;

    cast_channel::CastMessageHandler* const message_handler_;

    // Task runner for |outer_|.
    scoped_refptr<base::SequencedTaskRunner> task_runner_;

    // WeakPtr to the owning CastAppDiscoveryService.
    base::WeakPtr<CastAppDiscoveryService> outer_;

    CastAppAvailabilityTracker availability_tracker_;

    DISALLOW_COPY_AND_ASSIGN(Core);
  };

  void MaybeRemoveSinkQueryEntry(const MediaSource::Id& source_id);

  // Task runner for |core_|.
  scoped_refptr<base::SequencedTaskRunner> task_runner_;

  base::flat_map<MediaSource::Id, std::unique_ptr<SinkQueryCallbackList>>
      sink_queries_;
  std::unique_ptr<Core, base::OnTaskRunnerDeleter> core_;

  base::WeakPtrFactory<CastAppDiscoveryService> weak_ptr_factory_;
  DISALLOW_COPY_AND_ASSIGN(CastAppDiscoveryService);
};

}  // namespace media_router

#endif  // CHROME_BROWSER_MEDIA_ROUTER_PROVIDERS_CAST_CAST_APP_DISCOVERY_SERVICE_H_
