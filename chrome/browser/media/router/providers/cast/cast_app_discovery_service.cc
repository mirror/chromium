// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/media/router/providers/cast/cast_app_discovery_service.h"

namespace media_router {

CastAppDiscoveryService::CastAppDiscoveryService(
    const scoped_refptr<base::SequencedTaskRunner>& task_runner,
    cast_channel::CastMessageHandler* message_handler)
    : task_runner_(task_runner),
      core_(nullptr, base::OnTaskRunnerDeleter(task_runner)),
      weak_ptr_factory_(this) {
  core_.reset(new Core(message_handler, base::SequencedTaskRunnerHandle::Get(),
                       weak_ptr_factory_.GetWeakPtr()));
}
CastAppDiscoveryService::~CastAppDiscoveryService() = default;

CastAppDiscoveryService::Subscription
CastAppDiscoveryService::StartObservingMediaSinks(
    const CastMediaSource& source,
    const SinkQueryCallback& callback) {
  const MediaSource::Id& source_id = source.source_id();
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

void CastAppDiscoveryService::OnSinkQueryUpdated(
    const MediaSource::Id& source_id,
    const std::vector<MediaSinkInternal>& sinks,
    const std::vector<url::Origin>& origins) {
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
    const scoped_refptr<base::SequencedTaskRunner>& task_runner,
    const base::WeakPtr<CastAppDiscoveryService>& outer)
    : message_handler_(message_handler),
      task_runner_(task_runner),
      outer_(outer) {
  VLOG(0) << message_handler_;
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
  if (!cached_sinks.empty()) {
    task_runner_->PostTask(
        FROM_HERE, base::BindOnce(&CastAppDiscoveryService::OnSinkQueryUpdated,
                                  outer_, source.source_id(), cached_sinks,
                                  std::vector<url::Origin>()));
  }
}

void CastAppDiscoveryService::Core::RemoveSinkQuery(
    const MediaSource::Id& source_id) {}

void CastAppDiscoveryService::Core::OnSinkAdded(const MediaSinkInternal& sink) {
  VLOG(0) << __func__ << ": " << sink.sink().id();
}

void CastAppDiscoveryService::Core::OnSinkUpdated(
    const MediaSinkInternal& sink) {
  VLOG(0) << __func__ << ": " << sink.sink().id();
}

void CastAppDiscoveryService::Core::OnSinkRemoved(
    const MediaSink::Id& sink_id) {
  VLOG(0) << __func__ << ": " << sink_id;
}

std::vector<MediaSinkInternal> CastAppDiscoveryService::Core::GetSinksByIds(
    const base::flat_set<MediaSink::Id>& sink_ids) const {
  // XXX: implement
  return std::vector<MediaSinkInternal>();
}

}  // namespace media_router
