// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MEDIA_MOJO_SERVICES_MEDIA_CAPABILITIES_DATABASE_H_
#define MEDIA_MOJO_SERVICES_MEDIA_CAPABILITIES_DATABASE_H_

#include <memory>

#include "base/memory/weak_ptr.h"
#include "components/leveldb_proto/proto_database.h"
#include "media/base/video_codecs.h"
#include "media/mojo/services/media_mojo_export.h"
#include "ui/gfx/geometry/size.h"

namespace base {
class FilePath;
}  // namespace base

namespace media {

class CapabilitiesInfo;

// TODO: class comment.
class MEDIA_MOJO_EXPORT MediaCapabilitiesDatabase final {
 public:
  // Representation of the information used to identify a type of media
  // playback.
  class Entry {
   public:
    Entry(VideoCodecProfile codec_profile, const gfx::Size size, int frame_rate);

    std::string Serialize() const;

    bool operator<(const Entry& other) const;

   private:
    VideoCodecProfile codec_profile_;
    gfx::Size size_;
    int frame_rate_;
  };

  // Information saves to identify the capabilities related to a given |Entry|.
  struct Info {
    Info(uint32_t frames_decoded, uint32_t frames_dropped);

    uint32_t frames_decoded;
    uint32_t frames_dropped;
  };

  MediaCapabilitiesDatabase(const base::FilePath& dir);
  ~MediaCapabilitiesDatabase();

  // Adds `info` data into the database records associated with `entry`. The
  // operation is asynchronous. The caller should be aware of potential race
  // conditions when calling this method for the same `entry` very close to
  // eachothers.
  void AppendInfoToEntry(const Entry& entry, const Info& info);

  // Returns the `info` associated with `entry`. The `callback` will received
  // the `info` in addition to a boolean signaling if the call was succesful.
  // `info` can be nullptr if there was no data associated with `entry`.
  using GetInfoCallback = base::OnceCallback<void(bool, std::unique_ptr<Info>)>;
  void GetInfo(const Entry& entry, GetInfoCallback callback);

 private:
  // Reads the database to get the data associated with `entry` to then write it
  // to cache, and write to the database the new value by adding `info`.
  void ReadDatabaseAndUpdateWithInfo(const Entry& entry, const Info& info);

  // Called by `ReadDatabaseAndUpdateWithInfo` when the database was read.
  void WriteUpdatedEntry(
      const Entry& entry,
      const Info& info,
      bool success,
      std::unique_ptr<CapabilitiesInfo> capabilities_info_proto);

  // Called by the method above and `AppendInfoToEntry` to update the database
  // and write `capabilities_info_proto` associated to `serialized_entry`.
  void UpdateDatabase(
      const std::string& serialized_entry,
      std::unique_ptr<CapabilitiesInfo> capabilities_info_proto);

  // Implementation of `GetInfo`.
  void GetInfoFromDatabase(const std::string& serialized_entry,
                           GetInfoCallback callback);

  // Called when the database has been initialised.
  void OnInit(bool success);

  // Called when the database has been modified after a call to
  // `UpdateDatabase`. If `success` is false, it will set the `db_` in a failed
  // state.
  void OnDatabaseUpdated(bool success);

  // Called when GetInfoFromDatabase operation was performed. `callback` will be
  // run with `success` and an `Info` created based on `capabilities_info_proto`
  // or nullptr.
  void OnGetInfoFromDatabase(
      GetInfoCallback callback,
      bool success,
      std::unique_ptr<CapabilitiesInfo> capabilities_info_proto);

  bool db_init_ = false;
  std::unique_ptr<leveldb_proto::ProtoDatabase<CapabilitiesInfo>> db_;

  std::map<Entry, Info> cache_;

  base::WeakPtrFactory<MediaCapabilitiesDatabase> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(MediaCapabilitiesDatabase);
};

}  // namespace media

#endif  // MEDIA_MOJO_SERVICES_MEDIA_CAPABILITIES_DATABASE_H_

