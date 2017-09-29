// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/mojo/services/media_capabilities_leveldb.h"

#include <inttypes.h>

#include "base/files/file_path.h"
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

MediaCapabilitiesLevelDB::MediaCapabilitiesLevelDB(const base::FilePath& path)
    : db_(new leveldb_proto::ProtoDatabaseImpl<CapabilitiesInfo>(
          base::CreateSequencedTaskRunnerWithTraits(
              {base::MayBlock(), base::TaskPriority::BACKGROUND,
               base::TaskShutdownBehavior::CONTINUE_ON_SHUTDOWN}))),
      weak_ptr_factory_(this) {
  db_->Init(kDatabaseClientName, path, leveldb_proto::CreateSimpleOptions(),
            base::BindOnce(&MediaCapabilitiesLevelDB::OnInit,
                           weak_ptr_factory_.GetWeakPtr()));
}

MediaCapabilitiesLevelDB::~MediaCapabilitiesLevelDB() = default;

bool MediaCapabilitiesLevelDB::EntryComparator::operator()(
    const Entry& lhs,
    const Entry& rhs) const {
  if (lhs.codec_profile() != rhs.codec_profile())
    return lhs.codec_profile() < rhs.codec_profile();
  if (lhs.size().width() != rhs.size().width())
    return lhs.size().width() < rhs.size().width();
  if (lhs.size().height() != rhs.size().height())
    return lhs.size().height() < rhs.size().height();
  return lhs.frame_rate() < rhs.frame_rate();
}

bool MediaCapabilitiesLevelDB::IsDatabaseReady(std::string operation) {
  if (!db_init_) {
    // TODO(chcunningham): record UMA.
    DVLOG_IF(2, !operation.empty()) << __func__ << " DB is not initialized; "
                                    << "ignoring operation: " << operation;
    return false;
  }

  if (!db_) {
    // TODO(chcunningham): record UMA.
    DVLOG_IF(2, !operation.empty()) << __func__ << " DB is not initialized; "
                                    << "ignoring operation: " << operation;
    return false;
  }

  return true;
}

void MediaCapabilitiesLevelDB::AppendInfoToEntry(const Entry& entry,
                                                 const Info& info) {
  auto iter = cache_.find(entry);

  // The value was cached, no need to read it. The cache needs to be updated and
  // the database updated with the new value.
  if (iter != cache_.end()) {
    iter->second.frames_decoded += info.frames_decoded;
    iter->second.frames_dropped += info.frames_dropped;
    DVLOG(3) << __func__ << " Appended to cache " << EntryToString(entry)
             << " aggregate: " << InfoToString(iter->second);

    std::unique_ptr<CapabilitiesInfo> capabilities_info_proto(
        new CapabilitiesInfo());
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

void MediaCapabilitiesLevelDB::GetInfo(const Entry& entry,
                                       GetInfoCallback callback) {
  auto iter = cache_.find(entry);
  if (iter != cache_.end()) {
    std::move(callback).Run(true, base::MakeUnique<Info>(iter->second));
    return;
  }

  GetInfoFromDatabase(SerializeEntry(entry), std::move(callback));
}

void MediaCapabilitiesLevelDB::OnInit(bool success) {
  DVLOG(2) << __func__ << (success ? " succeeded" : " FAILED!");

  db_init_ = true;

  // Can't use DB when initialization fails.
  // TODO(chcunningham): Record UMA.
  if (!success)
    db_.reset();
}

void MediaCapabilitiesLevelDB::ReadDatabaseAndUpdateWithInfo(const Entry& entry,
                                                             const Info& info) {
  if (!IsDatabaseReady("ReadAndUpdate"))
    return;

  DVLOG(3) << __func__ << " Reading entry " << EntryToString(entry)
           << " from DB with intent to update with " << InfoToString(info);

  db_->GetEntry(SerializeEntry(entry),
                base::BindOnce(&MediaCapabilitiesLevelDB::WriteUpdatedEntry,
                               weak_ptr_factory_.GetWeakPtr(), entry, info));
}

void MediaCapabilitiesLevelDB::WriteUpdatedEntry(
    const Entry& entry,
    const Info& info,
    bool read_success,
    std::unique_ptr<CapabilitiesInfo> capabilities_info_proto) {
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

  Info aggreagtd_info(frames_decoded, frames_dropped);

  DVLOG(3) << __func__ << " Updating " << EntryToString(entry) << " with "
           << InfoToString(info)
           << " aggregate:" << InfoToString(aggreagtd_info);

  cache_.emplace(std::make_pair(entry, aggreagtd_info));

  UpdateDatabase(SerializeEntry(entry), std::move(capabilities_info_proto));
}

void MediaCapabilitiesLevelDB::UpdateDatabase(
    const std::string& serialized_entry,
    std::unique_ptr<CapabilitiesInfo> capabilities_info_proto) {
  if (!IsDatabaseReady("Update"))
    return;

  std::unique_ptr<
      leveldb_proto::ProtoDatabase<CapabilitiesInfo>::KeyEntryVector>
      entries(
          new leveldb_proto::ProtoDatabase<CapabilitiesInfo>::KeyEntryVector());
  entries->push_back(
      std::make_pair(serialized_entry, *capabilities_info_proto));

  db_->UpdateEntries(
      std::move(entries), base::MakeUnique<leveldb_proto::KeyVector>(),
      base::BindOnce(&MediaCapabilitiesLevelDB::OnDatabaseUpdated,
                     weak_ptr_factory_.GetWeakPtr()));
}

void MediaCapabilitiesLevelDB::GetInfoFromDatabase(
    const std::string& serialized_entry,
    GetInfoCallback callback) {
  if (!IsDatabaseReady("GetInfo"))
    return;

  db_->GetEntry(
      serialized_entry,
      base::BindOnce(&MediaCapabilitiesLevelDB::OnGetInfoFromDatabase,
                     weak_ptr_factory_.GetWeakPtr(), std::move(callback)));
}

void MediaCapabilitiesLevelDB::OnDatabaseUpdated(bool success) {
  // TODO(chcunningham): record UMA.
  DVLOG(3) << __func__ << " update " << (success ? "succeeded" : "FAILED!");
}

void MediaCapabilitiesLevelDB::OnGetInfoFromDatabase(
    GetInfoCallback callback,
    bool success,
    std::unique_ptr<CapabilitiesInfo> capabilities_info_proto) {
  // TODO(chcunningham): record UMA.

  std::unique_ptr<Info> info;
  if (capabilities_info_proto) {
    info = base::MakeUnique<Info>(capabilities_info_proto->frames_decoded(),
                                  capabilities_info_proto->frames_dropped());
  }

  DVLOG(3) << __func__ << " read " << (success ? "succeeded" : "FAILED!")
           << " info: " << (info ? InfoToString(*info) : "nullptr");

  std::move(callback).Run(success, std::move(info));
}

}  // namespace media
