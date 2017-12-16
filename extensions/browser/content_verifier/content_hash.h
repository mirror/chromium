// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EXTENSIONS_BROWSER_CONTENT_VERIFIER_CONTENT_HASH_H_
#define EXTENSIONS_BROWSER_CONTENT_VERIFIER_CONTENT_HASH_H_

#include <set>

#include "base/files/file_path.h"
#include "base/version.h"
#include "extensions/browser/computed_hashes.h"
#include "extensions/browser/content_verifier_delegate.h"
#include "extensions/browser/verified_contents.h"
#include "extensions/common/extension_id.h"

namespace extensions {

class CancellationStatus;

// Represents content verification hashes for an extension.
//
// Instances can be created using Create() factory methods on sequences with
// blocking IO access. If hash retrieval succeeds then ContentHash::succeeded()
// will return true and
// a. ContentHash::verified_contents() will return structured representation of
//    verified_contents.json
// b. ContentHash::computed_hashes() will return structured representation of
//    computed_hashes.json.
//
// Additionally if computed_hashes.json was written to disk successfully, then
// ContentHash::hash_mismatch_unix_paths() will return FilePaths whose content
// verification didn't match.
class ContentHash {
 public:
  // Key to identify an extension.
  struct ExtensionKey {
    ExtensionId extension_id;
    base::FilePath extension_root;
    base::Version extension_version;
    // The key used to validate verified_contents.json.
    ContentVerifierKey verifier_key;

    ExtensionKey();
    ExtensionKey(const ExtensionId& extension_id,
                 const base::FilePath& extension_root,
                 const base::Version& extension_version,
                 const ContentVerifierKey& verifier_key);

    ExtensionKey(const ExtensionKey& other);
    ExtensionKey& operator=(const ExtensionKey& other);
  };

  // Factories:
  // Reads existing hashes from disk.
  static ContentHash Create(const ExtensionKey& key);
  // Reads existing hashes from disk, but with the exceptions:
  // - verified_contents.json: Deletes the file if the file is not valid and
  //   only_read_verified_contents is false.
  // - computed_hashes.json: If |force_recreate_existing_computed_hashes_file|
  //   is true, always creates computed_hashes.json. Otherwise only creates it
  //   when the file is non-existent.
  static ContentHash Create(
      const ExtensionKey& key,
      bool only_read_verified_contents,
      bool force_recreate_existing_computed_hashes_file,
      scoped_refptr<CancellationStatus> cancellation_status);

  // Constructs an empty instance to std::move() in PostTask.*AndReply.
  ContentHash();

  ContentHash(ContentHash&& other);
  ContentHash& operator=(ContentHash&& other);
  ~ContentHash();

  const VerifiedContents& verified_contents() const;
  // TODO(lazyboy): Make it return const ComputedHashes::Reader&.
  ComputedHashes::Reader& computed_hashes() const;

  bool has_verified_contents() const {
    return status_ >= Status::kHasVerifiedContents;
  }
  bool succeeded() const { return status_ >= Status::kSucceeded; }

  // If ContentHash creation writes computed_hashes.json, then this returns the
  // FilePaths whose content hash didn't match expected hashes.
  const std::set<base::FilePath>& hash_mismatch_unix_paths() {
    return hash_mismatch_unix_paths_;
  }

 private:
  enum class Status {
    // Retrieving hashes failed.
    kInvalid,
    // Retrieved valid verified_contents.json, but there was no
    // computed_hashes.json.
    kHasVerifiedContents,
    // Both verified_contents.json and computed_hashes.json was read
    // correctly.
    kSucceeded,
  };

  static ContentHash CreateImpl(
      const ExtensionKey& key,
      bool delete_invalid_verified_contents_file,
      bool create_computed_hashes_file,
      bool force_recreate_existing_computed_hashes_file,
      scoped_refptr<CancellationStatus> cancellation_status);

  ContentHash(const ExtensionKey& key, Status status);
  ContentHash(const ExtensionKey& key,
              std::unique_ptr<VerifiedContents> verified_contents,
              std::unique_ptr<ComputedHashes::Reader> computed_hashes);

  // Computes hashes for all files in |key_.extension_root|, and uses
  // a ComputedHashes::Writer to write that information into |hashes_file|.
  // Returns true on success.
  // The verified contents file from the webstore only contains the treehash
  // root hash, but for performance we want to cache the individual block level
  // hashes. This function will create that cache with block-level hashes for
  // each file in the extension if needed (the treehash root hash for each of
  // these should equal what is in the verified contents file from the
  // webstore).
  bool CreateHashes(const base::FilePath& hashes_file,
                    scoped_refptr<CancellationStatus> cancellation_status);

  ExtensionKey key_;

  Status status_;

  // TODO(lazyboy): Avoid dynamic allocations here, |this| already supports
  // move.
  std::unique_ptr<VerifiedContents> verified_contents_;
  std::unique_ptr<ComputedHashes::Reader> computed_hashes_;

  // Paths that were found to have a mismatching hash.
  std::set<base::FilePath> hash_mismatch_unix_paths_;

  // The block size to use for hashing.
  // TODO(asargent) - use the value from verified_contents.json for each
  // file, instead of using a constant.
  int block_size_ = 4096;

  DISALLOW_COPY_AND_ASSIGN(ContentHash);
};

}  // namespace extensions

#endif  // EXTENSIONS_BROWSER_CONTENT_VERIFIER_CONTENT_HASH_H_
