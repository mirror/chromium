// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MEDIA_MOJO_SERVICES_LEVELdB_MEDIA_CAPAbILITIES_DATABASE_H_
#define MEDIA_MOJO_SERVICES_LEVELdB_MEDIA_CAPAbILITIES_DATABASE_H_

#include <memory>

#include "base/memory/weak_ptr.h"
#include "components/leveldb_proto/proto_database.h"
#include "media/base/video_codecs.h"
#include "media/mojo/services/media_capabilities_database.h"
#include "media/mojo/services/media_mojo_export.h"
#include "ui/gfx/geometry/size.h"

namespace base {
class FilePath;
}  // namespace base

namespace media {

class CapabilitiesInfo;

// LevelDB implementation of MediaCapabilitiesDatabase.
class MEDIA_MOJO_EXPORT MediaCapabilitiesLevelDB
    : public MediaCapabilitiesDatabase {
 public:
  // |dir| specifies where to store LevelDB files to disk. LevelDB generates a
  // handful of files, so its recommended to provide a dedicated directory to
  // keep them isolated.
  MediaCapabilitiesLevelDB(const base::FilePath& dir);
  ~MediaCapabilitiesLevelDB() override;

  // See MediaCapabilitiesDatabase.
  void AppendInfoToEntry(const Entry& entry, const Info& info) override;
  void GetInfo(const Entry& entry, GetInfoCallback callback) override;

 private:
  friend class MediaCapabilitiesLevelDBTest;

  // Comparator for using Entries as keys in cache map.
  struct EntryComparator {
    bool operator()(const Entry& lhs, const Entry& rhs) const;
  };

  // Returns true if the DB is initialized and ready for use. The optional
  // |operation| is used to issue log warnings when the DB is not ready.
  bool IsDatabaseReady(std::string operation = "");

  // Reads the database to get the data associated with `entry` to then write it
  // to cache, and write to the database the new value by adding `info`.
  void ReadDatabaseAndUpdateWithInfo(const Entry& entry, const Info& info);

  // Called by `ReadDatabaseAndUpdateWithInfo` when the database was read.
  void WriteUpdatedEntry(
      const Entry& entry,
      const Info& info,
      bool read_success,
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

  // Indicates whether initialization is completed. Does not indicate whether it
  // was successful. Failed initialization is signaled by setting |db_| to null.
  bool db_init_ = false;

  // ProtoDatabase instance. Set to null if fatal database error is encountered.
  std::unique_ptr<leveldb_proto::ProtoDatabase<CapabilitiesInfo>> db_;

  std::map<Entry, Info, EntryComparator> cache_;

  base::WeakPtrFactory<MediaCapabilitiesLevelDB> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(MediaCapabilitiesLevelDB);
};

}  // namespace media

#endif  // MEDIA_MOJO_SERVICES_LEVELdB_MEDIA_CAPAbILITIES_DATABASE_H_
