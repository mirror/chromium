// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/mojo/services/media_capabilities_database_impl.h"

#include <memory>
#include <tuple>

#include "base/files/file_path.h"
#include "base/format_macros.h"
#include "base/sequence_checker.h"
#include "base/strings/stringprintf.h"
#include "base/task_scheduler/post_task.h"
#include "components/leveldb_proto/proto_database_impl.h"
#include "media/mojo/services/media_capabilities.pb.h"

namespace media {

namespace {

// Avoid changing client name. Used in UMA.
// See comments in components/leveldb_proto/leveldb_database.h
const char kDatabaseClientName[] = "MediaCapabilities";

constexpr uint32_t kDatabaseVersion = 1;

// Serialize the |entry| for storing in the database.
std::string SerializeEntry(const MediaCapabilitiesDatabase::Entry& entry) {
  return base::StringPrintf("%d|%s|%d", static_cast<int>(entry.codec_profile()),
                            entry.size().ToString().c_str(),
                            entry.frame_rate());
}

// For debug logging.
std::string EntryToString(const MediaCapabilitiesDatabase::Entry& entry) {
  return "Entry {" + SerializeEntry(entry) + "}";
}

// For debug logging.
std::string InfoToString(const MediaCapabilitiesDatabase::Info& info) {
  return base::StringPrintf("Info {frames decoded:%" PRIu64
                            ", dropped: %" PRIu64 "}",
                            info.frames_decoded, info.frames_dropped);
}

};  // namespace

MediaCapabilitiesDatabaseImpl::MediaCapabilitiesDatabaseImpl()
    : weak_ptr_factory_(this) {}

MediaCapabilitiesDatabaseImpl::~MediaCapabilitiesDatabaseImpl() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
}

void MediaCapabilitiesDatabaseImpl::Initialize(
    const base::FilePath& path,
    base::OnceCallback<void(bool)> init_cb) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  db_ = std::make_unique<leveldb_proto::ProtoDatabaseImpl<CapabilitiesInfo>>(
      base::CreateSequencedTaskRunnerWithTraits(
          {base::MayBlock(), base::TaskPriority::BACKGROUND,
           base::TaskShutdownBehavior::CONTINUE_ON_SHUTDOWN}));

  db_->Init(kDatabaseClientName, path, leveldb_proto::CreateSimpleOptions(),
            base::BindOnce(&MediaCapabilitiesDatabaseImpl::OnInit,
                           weak_ptr_factory_.GetWeakPtr(), std::move(init_cb)));
}

void MediaCapabilitiesDatabaseImpl::OnInit(
    base::OnceCallback<void(bool)> init_cb,
    bool success) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  DVLOG(2) << __func__ << (success ? " succeeded" : " FAILED!");

  db_init_ = true;

  // Can't use DB when initialization fails.
  // TODO(chcunningham): Record UMA.
  if (!success)
    db_.reset();

  if (init_cb)
    std::move(init_cb).Run(success);
}

bool MediaCapabilitiesDatabaseImpl::EntryComparator::operator()(
    const Entry& lhs,
    const Entry& rhs) const {
  return std::make_tuple(lhs.codec_profile(), lhs.size().width(),
                         lhs.size().height(), lhs.frame_rate()) <
         std::make_tuple(rhs.codec_profile(), rhs.size().width(),
                         rhs.size().height(), rhs.frame_rate());
}

bool MediaCapabilitiesDatabaseImpl::IsDatabaseReady(
    const std::string& operation) {
  if (!db_init_) {
    DVLOG_IF(2, !operation.empty()) << __func__ << " DB is not initialized; "
                                    << "ignoring operation: " << operation;
    return false;
  }

  if (!db_) {
    DVLOG_IF(2, !operation.empty()) << __func__ << " DB is not initialized; "
                                    << "ignoring operation: " << operation;
    return false;
  }

  return true;
}

void MediaCapabilitiesDatabaseImpl::AppendInfoToEntry(const Entry& entry,
                                                      const Info& info) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  auto iter = cache_.find(entry);
  if (iter != cache_.end()) {
    // The value was cached, no need to read it. The cache needs to be updated
    // and the database updated with the new value.
    iter->second.frames_decoded += info.frames_decoded;
    iter->second.frames_dropped += info.frames_dropped;
    DVLOG(3) << __func__ << " Appended to cache " << EntryToString(entry)
             << " aggregate: " << InfoToString(iter->second);

    std::unique_ptr<CapabilitiesInfo> capabilities_info_proto =
        std::make_unique<CapabilitiesInfo>();
    capabilities_info_proto->set_version(kDatabaseVersion);
    capabilities_info_proto->set_frames_decoded(iter->second.frames_decoded);
    capabilities_info_proto->set_frames_dropped(iter->second.frames_dropped);
    UpdateDatabase(SerializeEntry(entry), std::move(capabilities_info_proto));
  } else {
    // The information wasn't cached, it needs to be read from the db, cached,
    // and the updated value needs to be saved again.
    ReadDatabaseAndUpdateWithInfo(entry, info);
  }
}

void MediaCapabilitiesDatabaseImpl::GetDecodingInfo(
    const Entry& entry,
    GetDecodingInfoCb callback) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  auto iter = cache_.find(entry);
  if (iter != cache_.end()) {
    std::move(callback).Run(true, std::make_unique<Info>(iter->second));
    return;
  }

  GetDecodingInfoFromDatabase(SerializeEntry(entry), std::move(callback));
}

void MediaCapabilitiesDatabaseImpl::ReadDatabaseAndUpdateWithInfo(
    const Entry& entry,
    const Info& info) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  if (!IsDatabaseReady("ReadAndUpdate"))
    return;

  DVLOG(3) << __func__ << " Reading entry " << EntryToString(entry)
           << " from DB with intent to update with " << InfoToString(info);

  db_->GetEntry(
      SerializeEntry(entry),
      base::BindOnce(&MediaCapabilitiesDatabaseImpl::WriteUpdatedEntry,
                     weak_ptr_factory_.GetWeakPtr(), entry, info));
}

void MediaCapabilitiesDatabaseImpl::WriteUpdatedEntry(
    const Entry& entry,
    const Info& info,
    bool read_success,
    std::unique_ptr<CapabilitiesInfo> capabilities_info_proto) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  // TODO(chcunningham): Record UMA.
  if (!read_success) {
    DVLOG(2) << __func__ << " FAILED DB read for " << EntryToString(entry)
             << "; ignoring update!";
    return;
  }

  if (!capabilities_info_proto) {
    capabilities_info_proto.reset(new CapabilitiesInfo());
    capabilities_info_proto->set_version(kDatabaseVersion);
    capabilities_info_proto->set_frames_decoded(0);
    capabilities_info_proto->set_frames_dropped(0);
  }

  uint64_t frames_decoded =
      capabilities_info_proto->frames_decoded() + info.frames_decoded;
  uint64_t frames_dropped =
      capabilities_info_proto->frames_dropped() + info.frames_dropped;

  capabilities_info_proto->set_frames_decoded(frames_decoded);
  capabilities_info_proto->set_frames_dropped(frames_dropped);

  Info aggregate_info(frames_decoded, frames_dropped);

  DVLOG(3) << __func__ << " Updating " << EntryToString(entry) << " with "
           << InfoToString(info)
           << " aggregate:" << InfoToString(aggregate_info);

  cache_.emplace(std::make_pair(entry, aggregate_info));

  UpdateDatabase(SerializeEntry(entry), std::move(capabilities_info_proto));
}

void MediaCapabilitiesDatabaseImpl::UpdateDatabase(
    const std::string& serialized_entry,
    std::unique_ptr<CapabilitiesInfo> capabilities_info_proto) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  if (!IsDatabaseReady("Update"))
    return;

  using ProtoInfo = leveldb_proto::ProtoDatabase<CapabilitiesInfo>;
  std::unique_ptr<ProtoInfo::KeyEntryVector> entries(
      new ProtoInfo::KeyEntryVector());

  entries->emplace_back(serialized_entry, *capabilities_info_proto);

  db_->UpdateEntries(
      std::move(entries), std::make_unique<leveldb_proto::KeyVector>(),
      base::BindOnce(&MediaCapabilitiesDatabaseImpl::OnDatabaseUpdated,
                     weak_ptr_factory_.GetWeakPtr()));
}

void MediaCapabilitiesDatabaseImpl::GetDecodingInfoFromDatabase(
    const std::string& serialized_entry,
    GetDecodingInfoCb callback) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  if (!IsDatabaseReady("GetDecodingInfo")) {
    std::move(callback).Run(false, nullptr);
    return;
  }

  db_->GetEntry(
      serialized_entry,
      base::BindOnce(
          &MediaCapabilitiesDatabaseImpl::OnGetDecodingInfoFromDatabase,
          weak_ptr_factory_.GetWeakPtr(), std::move(callback)));
}

void MediaCapabilitiesDatabaseImpl::OnDatabaseUpdated(bool success) {
  // TODO(chcunningham): record UMA.
  DVLOG(3) << __func__ << " update " << (success ? "succeeded" : "FAILED!");
}

void MediaCapabilitiesDatabaseImpl::OnGetDecodingInfoFromDatabase(
    GetDecodingInfoCb callback,
    bool success,
    std::unique_ptr<CapabilitiesInfo> capabilities_info_proto) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  // TODO(chcunningham): record UMA.

  std::unique_ptr<Info> info;
  if (capabilities_info_proto) {
    info = std::make_unique<Info>(capabilities_info_proto->frames_decoded(),
                                  capabilities_info_proto->frames_dropped());
  }

  DVLOG(3) << __func__ << " read " << (success ? "succeeded" : "FAILED!")
           << " info: " << (info ? InfoToString(*info) : "nullptr");

  std::move(callback).Run(success, std::move(info));
}

}  // namespace media
