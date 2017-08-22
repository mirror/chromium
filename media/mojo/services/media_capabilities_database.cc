// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/mojo/services/media_capabilities_database.h"

#include "base/files/file_path.h"
#include "base/task_scheduler/post_task.h"
#include "media/mojo/services/media_capabilities.pb.h"
#include "components/leveldb_proto/proto_database_impl.h"

namespace media {

namespace {

constexpr uint32_t kDatabaseVersion = 1;

};

MediaCapabilitiesDatabase::Entry::Entry(VideoCodecProfile codec_profile,
                                        const gfx::Size size, int frame_rate)
  : codec_profile_(codec_profile), size_(size), frame_rate_(frame_rate) {}

std::string MediaCapabilitiesDatabase::Entry::Serialize() const {
  std::string serialized_value = "";
  serialized_value += static_cast<int>(codec_profile_);
  serialized_value += "|";
  serialized_value += size_.ToString();
  serialized_value += "|";
  serialized_value += frame_rate_;
  return serialized_value;
}

bool MediaCapabilitiesDatabase::Entry::operator<(const Entry& other) const {
  if (codec_profile_ != other.codec_profile_)
    return codec_profile_ < other.codec_profile_;
  if (size_.width() != other.size_.width())
    return size_.width() < other.size_.width();
  if (size_.height() != other.size_.height())
    return size_.height() < other.size_.height();
  return frame_rate_ < other.frame_rate_;
}

MediaCapabilitiesDatabase::Info::Info(uint32_t frames_decoded,
                                      uint32_t frames_dropped)
  : frames_decoded(frames_decoded), frames_dropped(frames_dropped) {}

MediaCapabilitiesDatabase::MediaCapabilitiesDatabase(const base::FilePath& path)
    : db_(new leveldb_proto::ProtoDatabaseImpl<CapabilitiesInfo>(
          base::CreateSequencedTaskRunnerWithTraits(
              {base::MayBlock(), base::TaskPriority::BACKGROUND,
               base::TaskShutdownBehavior::CONTINUE_ON_SHUTDOWN}))),
      weak_ptr_factory_(this) {
  db_->Init(path.BaseName().value().c_str(), path.DirName(),
            base::BindOnce(&MediaCapabilitiesDatabase::OnInit,
                           weak_ptr_factory_.GetWeakPtr()));
}

MediaCapabilitiesDatabase::~MediaCapabilitiesDatabase() = default;

void MediaCapabilitiesDatabase::AppendInfoToEntry(const Entry& entry,
                                                  const Info& info) {
  auto iter = cache_.find(entry);

  // The value was cached, no need to read it. The cache needs to be updated and
  // the database updated with the new value.
  if (iter != cache_.end()) {
    iter->second.frames_decoded += info.frames_decoded;
    iter->second.frames_dropped += info.frames_dropped;

    std::unique_ptr<CapabilitiesInfo> capabilities_info_proto(new CapabilitiesInfo());
    capabilities_info_proto->set_version(kDatabaseVersion);
    capabilities_info_proto->set_frames_decoded(iter->second.frames_decoded);
    capabilities_info_proto->set_frames_dropped(iter->second.frames_dropped);
    UpdateDatabase(entry.Serialize(), std::move(capabilities_info_proto));
  }

  // The information wasn't cached, it needs to be read from the db, cached, and
  // the updated value needs to be saved again.
  ReadDatabaseAndUpdateWithInfo(entry, info);
}

void MediaCapabilitiesDatabase::GetInfo(const Entry& entry,
                                        GetInfoCallback callback) {
  auto iter = cache_.find(entry);

  if (iter != cache_.end()) {
    std::move(callback).Run(true, base::MakeUnique<Info>(iter->second));
    return;
  }

  GetInfoFromDatabase(entry.Serialize(), std::move(callback));
}

void MediaCapabilitiesDatabase::OnInit(bool success) {
  db_init_ = success;
  if (!success) {
    DVLOG(1) << "MediaCapabilitiesDatabase failed to initialize.";
    db_.reset();
  }
}

void MediaCapabilitiesDatabase::ReadDatabaseAndUpdateWithInfo(
    const Entry& entry, const Info& info) {
  if (!db_init_) {
    DVLOG(1) << "MediaCapabilitiesDatabase wasn't initialized when reading to "
                "update, ignoring call";
    // TODO: record event.
    return;
  }

  if (!db_) {
    DVLOG(1) << "MediaCapabilitiesDatabase is in a failed state, reading to "
                "update ignored";
    // TODO: record event.
    return;
  }

  db_->GetEntry(entry.Serialize(),
      base::BindOnce(&MediaCapabilitiesDatabase::WriteUpdatedEntry,
                     weak_ptr_factory_.GetWeakPtr(), entry, info));
}

void MediaCapabilitiesDatabase::WriteUpdatedEntry(
    const Entry& entry,
    const Info& info,
    bool success,
    std::unique_ptr<CapabilitiesInfo> capabilities_info_proto) {
  if (!success)
    return;

  if (!capabilities_info_proto) {
    capabilities_info_proto.reset(new CapabilitiesInfo());
    capabilities_info_proto->set_version(kDatabaseVersion);
    capabilities_info_proto->set_frames_decoded(0);
    capabilities_info_proto->set_frames_dropped(0);
  }

  bool frames_decoded =
      capabilities_info_proto->frames_decoded() + info.frames_decoded;
  bool frames_dropped =
      capabilities_info_proto->frames_dropped() + info.frames_dropped;

  capabilities_info_proto->set_frames_decoded(frames_decoded);
  capabilities_info_proto->set_frames_dropped(frames_dropped);

  cache_.emplace(std::make_pair(entry, Info(frames_decoded, frames_dropped)));

  UpdateDatabase(entry.Serialize(), std::move(capabilities_info_proto));
}

void MediaCapabilitiesDatabase::UpdateDatabase(
    const std::string& serialized_entry,
    std::unique_ptr<CapabilitiesInfo> capabilities_info_proto) {
  if (!db_init_) {
    DVLOG(1) << "MediaCapabilitiesDatabase wasn't initialized when update "
                "requested, ignoring.";
    // TODO: record event.
    return;
  }

  if (!db_) {
    DVLOG(1) << "MediaCapabilitiesDatabase is in a failed state, update "
                "ignored.";
    // TODO: record event.
    return;
  }

  std::unique_ptr<leveldb_proto::ProtoDatabase<CapabilitiesInfo>::KeyEntryVector> entries(new leveldb_proto::ProtoDatabase<CapabilitiesInfo>::KeyEntryVector());
  entries->push_back(std::make_pair(serialized_entry, *capabilities_info_proto));

  db_->UpdateEntries(
    std::move(entries), base::MakeUnique<leveldb_proto::KeyVector>(),
    base::BindOnce(&MediaCapabilitiesDatabase::OnDatabaseUpdated,
        weak_ptr_factory_.GetWeakPtr()));
}

void MediaCapabilitiesDatabase::GetInfoFromDatabase(
    const std::string& serialized_entry, GetInfoCallback callback) {
  if (!db_init_) {
    DVLOG(1) << "MediaCapabilitiesDatabase wasn't initialized when reading "
                "database, ignoring";
    // TODO: record event.
    return;
  }

  if (!db_) {
    DVLOG(1) << "MediaCapabilitiesDatabase is in a failed state, reading "
                "ignored.";
    // TODO: record event.
    return;
  }

  db_->GetEntry(serialized_entry,
      base::BindOnce(&MediaCapabilitiesDatabase::OnGetInfoFromDatabase,
                     weak_ptr_factory_.GetWeakPtr(), std::move(callback)));
}

void MediaCapabilitiesDatabase::OnDatabaseUpdated(bool success) {
  if (!success) {
    DVLOG(1) << "MediaCapabilitiesDatabase failed to update one entry.";
    db_.reset();
  }
}

void MediaCapabilitiesDatabase::OnGetInfoFromDatabase(
    GetInfoCallback callback,
    bool success,
    std::unique_ptr<CapabilitiesInfo> capabilities_info_proto) {
  if (!success) {
    DVLOG(1) << "MediaCapabilitiesDatabase failed to read entry.";
    db_.reset();
  }

  if (!capabilities_info_proto || !success) {
    std::move(callback).Run(success, nullptr);
    return;
  }

  std::move(callback).Run(true,
      base::MakeUnique<Info>(capabilities_info_proto->frames_decoded(),
                             capabilities_info_proto->frames_dropped()));
}

}  // media
