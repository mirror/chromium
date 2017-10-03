// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MEDIA_MOJO_SERVICES_MEDIA_CAPABILITIES_DATABASE_IMPL_H_
#define MEDIA_MOJO_SERVICES_MEDIA_CAPABILITIES_DATABASE_IMPL_H_

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

// LevelDB implementation of MediaCapabilitiesDatabase. This class is not
// thread safe. All API calls should happen on the same sequence used for
// construction. API callbacks will also occur on this sequence.
class MEDIA_MOJO_EXPORT MediaCapabilitiesDatabaseImpl
    : public MediaCapabilitiesDatabase {
 public:
  MediaCapabilitiesDatabaseImpl();
  ~MediaCapabilitiesDatabaseImpl() override;

  // |dir| specifies where to store LevelDB files to disk. LevelDB generates a
  // handful of files, so its recommended to provide a dedicated directory to
  // keep them isolated.
  // |init_cb| is called with the results of initialization. This is mainly for
  // testing, as init failures are handled internally (db operations all no-op).
  void Initialize(const base::FilePath& path,
                  base::OnceCallback<void(bool)> init_cb);

  // See MediaCapabilitiesDatabase.
  void AppendInfoToEntry(const Entry& entry, const Info& info) override;
  void GetDecodingInfo(const Entry& entry, GetDecodingInfoCb callback) override;

 private:
  friend class MediaCapabilitiesDatabaseTest;

  // Comparator for using Entries as keys in cache map.
  struct EntryComparator {
    bool operator()(const Entry& lhs, const Entry& rhs) const;
  };

  // Called when the database has been initialised.
  void OnInit(base::OnceCallback<void(bool)> init_cb, bool success);

  // Returns true if the DB is initialized and ready for use. The optional
  // |operation| is used to issue log warnings when the DB is not ready.
  bool IsDatabaseReady(const std::string& operation = "");

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

  // Implementation of `GetDecodingInfo`.
  void GetDecodingInfoFromDatabase(const std::string& serialized_entry,
                                   GetDecodingInfoCb callback);

  // Called when the database has been modified after a call to
  // `UpdateDatabase`. If `success` is false, it will set the `db_` in a failed
  // state.
  void OnDatabaseUpdated(bool success);

  // Called when GetDecodingInfoFromDatabase operation was performed. `callback`
  // will be run with `success` and an `Info` created based on
  // `capabilities_info_proto` or nullptr.
  void OnGetDecodingInfoFromDatabase(
      GetDecodingInfoCb callback,
      bool success,
      std::unique_ptr<CapabilitiesInfo> capabilities_info_proto);

  // Indicates whether initialization is completed. Does not indicate whether it
  // was successful. Failed initialization is signaled by setting |db_| to null.
  bool db_init_ = false;

  // ProtoDatabase instance. Set to null if fatal database error is encountered.
  std::unique_ptr<leveldb_proto::ProtoDatabase<CapabilitiesInfo>> db_;

  std::map<Entry, Info, EntryComparator> cache_;

  // Ensures all access to class members come on the same sequence. API calls
  // and callbacks should occur on the same sequence used during construction.
  // LevelDB operations happen on a separate task runner, but all LevelDB
  // callbacks to this happen on the checked sequence.
  SEQUENCE_CHECKER(sequence_checker_);

  base::WeakPtrFactory<MediaCapabilitiesDatabaseImpl> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(MediaCapabilitiesDatabaseImpl);
};

}  // namespace media

#endif  // MEDIA_MOJO_SERVICES_MEDIA_CAPABILITIES_DATABASE_IMPL_H_
