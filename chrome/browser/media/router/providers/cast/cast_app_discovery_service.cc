// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/media/router/providers/cast/cast_app_discovery_service.h"

#include "components/cast_channel/cast_message_handler.h"
#include "components/cast_channel/cast_socket.h"
#include "components/cast_channel/cast_socket_service.h"

namespace media_router {

CastAppDiscoveryService::CastAppDiscoveryService(
    cast_channel::CastMessageHandler* message_handler,
    cast_channel::CastSocketService* socket_service)
    : task_runner_(socket_service->task_runner()),
      core_(nullptr, base::OnTaskRunnerDeleter(task_runner_)),
      weak_ptr_factory_(this) {
  core_.reset(new Core(message_handler, socket_service,
                       base::SequencedTaskRunnerHandle::Get(),
                       weak_ptr_factory_.GetWeakPtr()));
}
CastAppDiscoveryService::~CastAppDiscoveryService() = default;

CastAppDiscoveryService::Subscription
CastAppDiscoveryService::StartObservingMediaSinks(
    const CastMediaSource& source,
    const SinkQueryCallback& callback) {
  const MediaSource::Id& source_id = source.source_id();
  VLOG(0) << "XXX: " << __func__ << ", " << source_id;
  auto& callback_list = sink_queries_[source_id];
  if (!callback_list) {
    callback_list = std::make_unique<SinkQueryCallbackList>();
    callback_list->set_removal_callback(
        base::BindRepeating(&CastAppDiscoveryService::MaybeRemoveSinkQueryEntry,
                            base::Unretained(this), source_id));
  }

  task_runner_->PostTask(
      FROM_HERE, base::BindOnce(&CastAppDiscoveryService::Core::AddSinkQuery,
                                base::Unretained(core_.get()), source));
  return callback_list->Add(callback);
}

void CastAppDiscoveryService::UpdateSinkQueryResults(
    const MediaSource::Id& source_id,
    const std::vector<MediaSinkInternal>& sinks,
    const std::vector<url::Origin>& origins) {
  VLOG(0) << "XXX: " << __func__;
  auto it = sink_queries_.find(source_id);
  if (it == sink_queries_.end())
    return;

  it->second->Notify(source_id, sinks, origins);
}

CastMediaSinkServiceImpl::Observer*
CastAppDiscoveryService::GetSinkDiscoveryObserver() {
  return core_.get();
}

void CastAppDiscoveryService::MaybeRemoveSinkQueryEntry(
    const MediaSource::Id& source_id) {
  auto it = sink_queries_.find(source_id);
  DCHECK(it != sink_queries_.end());

  if (it->second->empty()) {
    sink_queries_.erase(it);
    task_runner_->PostTask(
        FROM_HERE,
        base::BindOnce(&CastAppDiscoveryService::Core::RemoveSinkQuery,
                       base::Unretained(core_.get()), source_id));
  }
}

CastAppDiscoveryService::Core::Core(
    cast_channel::CastMessageHandler* message_handler,
    cast_channel::CastSocketService* socket_service,
    const scoped_refptr<base::SequencedTaskRunner>& task_runner,
    const base::WeakPtr<CastAppDiscoveryService>& outer)
    : message_handler_(message_handler),
      socket_service_(socket_service),
      task_runner_(task_runner),
      outer_(outer),
      weak_ptr_factory_(this) {
  DCHECK(message_handler_);
  DCHECK(socket_service_);
}

CastAppDiscoveryService::Core::~Core() = default;

void CastAppDiscoveryService::Core::AddSinkQuery(
    const CastMediaSource& source) {
  base::flat_set<std::string> new_app_ids =
      availability_tracker_.RegisterSource(source);
  base::flat_set<MediaSink::Id> cached_sink_ids =
      availability_tracker_.GetAvailableSinks(source);

  // Returned cached results immediately, if available.
  std::vector<MediaSinkInternal> cached_sinks = GetSinksByIds(cached_sink_ids);
  if (!cached_sinks.empty())
    PostTaskUpdateSinkQueryResults(source.source_id(), cached_sinks);

  for (const auto& app_id : new_app_ids) {
    for (const auto& sink : sinks_) {
      int channel_id = sink.second.cast_data().cast_channel_id;
      cast_channel::CastSocket* socket = socket_service_->GetSocket(channel_id);
      if (!socket) {
        DVLOG(1) << "Socket not found for id " << channel_id;
        continue;
      }

      RequestAppAvailability(socket, app_id, sink.first);
    }
  }
}

void CastAppDiscoveryService::Core::RemoveSinkQuery(
    const MediaSource::Id& source_id) {
  availability_tracker_.UnregisterSource(source_id);
}

void CastAppDiscoveryService::Core::OnSinkAddedOrUpdated(
    const MediaSinkInternal& sink,
    cast_channel::CastSocket* socket) {
  const MediaSink::Id& sink_id = sink.sink().id();
  sinks_.insert_or_assign(sink_id, sink);
  for (const std::string& app_id : availability_tracker_.GetRegisteredApps()) {
    if (availability_tracker_.IsAvailabilityKnown(sink_id, app_id))
      continue;

    RequestAppAvailability(socket, app_id, sink_id);
  }
}

void CastAppDiscoveryService::Core::OnSinkRemoved(
    const MediaSink::Id& sink_id) {
  sinks_.erase(sink_id);
  UpdateSinkQueries(availability_tracker_.RemoveResultsForSink(sink_id));
}

void CastAppDiscoveryService::Core::RequestAppAvailability(
    cast_channel::CastSocket* socket,
    const std::string& app_id,
    const MediaSink::Id& sink_id) {
  VLOG(0) << __func__ << "XXX: requesting app avail: " << sink_id << ", "
          << app_id;
  message_handler_->RequestAppAvailability(
      socket, app_id,
      base::BindOnce(&CastAppDiscoveryService::Core::UpdateAppAvailability,
                     weak_ptr_factory_.GetWeakPtr(), sink_id));
}

void CastAppDiscoveryService::Core::UpdateAppAvailability(
    const MediaSink::Id& sink_id,
    const std::string& app_id,
    cast_channel::GetAppAvailabilityResult result) {
  VLOG(0) << "XXX: " << __func__ << " sink_id: " << sink_id
          << ", app_id: " << app_id << ", result: " << static_cast<int>(result);

  if (!base::ContainsKey(sinks_, sink_id))
    return;

  if (result == cast_channel::GetAppAvailabilityResult::kUnknown) {
    // TODO(crbug.com/779892): Implement retry on user gesture.
    DVLOG(2) << "App availability unknown for sink: " << sink_id
             << ", app: " << app_id;
    return;
  }

  AppAvailability availability =
      result == cast_channel::GetAppAvailabilityResult::kAvailable
          ? AppAvailability::kAvailable
          : AppAvailability::kUnavailable;

  DVLOG(1) << "App " << app_id << " on sink " << sink_id << " is "
           << (availability == AppAvailability::kAvailable);
  UpdateSinkQueries(availability_tracker_.UpdateAppAvailability(sink_id, app_id,
                                                                availability));
}

void CastAppDiscoveryService::Core::UpdateSinkQueries(
    const std::vector<CastMediaSource>& sources) {
  for (const auto& source : sources) {
    base::flat_set<MediaSink::Id> sink_ids =
        availability_tracker_.GetAvailableSinks(source);
    PostTaskUpdateSinkQueryResults(source.source_id(), GetSinksByIds(sink_ids));
  }
}

void CastAppDiscoveryService::Core::PostTaskUpdateSinkQueryResults(
    const MediaSource::Id& source_id,
    const std::vector<MediaSinkInternal>& sinks) {
  // Cast devices are valid on all origins.
  // TODO(crbug.com/779892): There is an exception to the above for the
  // mirroring app ID, which is used by Slides.
  task_runner_->PostTask(
      FROM_HERE,
      base::BindOnce(&CastAppDiscoveryService::UpdateSinkQueryResults, outer_,
                     source_id, sinks, std::vector<url::Origin>()));
}

std::vector<MediaSinkInternal> CastAppDiscoveryService::Core::GetSinksByIds(
    const base::flat_set<MediaSink::Id>& sink_ids) const {
  std::vector<MediaSinkInternal> sinks;
  for (const auto& sink_id : sink_ids) {
    auto it = sinks_.find(sink_id);
    if (it != sinks_.end())
      sinks.push_back(it->second);
  }
  return sinks;
}

}  // namespace media_router
