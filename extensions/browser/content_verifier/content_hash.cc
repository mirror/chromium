// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "extensions/browser/content_verifier/content_hash.h"

#include "base/files/file_enumerator.h"
#include "base/files/file_util.h"
#include "base/metrics/histogram_macros.h"
#include "base/threading/thread_restrictions.h"
#include "base/timer/elapsed_timer.h"
#include "crypto/sha2.h"
#include "extensions/browser/content_hash_tree.h"
#include "extensions/common/file_util.h"

namespace {

using SortedFilePathSet = std::set<base::FilePath>;

}  // namespace

namespace extensions {

ContentHash::ExtensionKey::ExtensionKey(const ExtensionId& extension_id,
                                        const base::FilePath& extension_root,
                                        const base::Version& extension_version,
                                        const ContentVerifierKey& verifier_key)
    : extension_id(extension_id),
      extension_root(extension_root),
      extension_version(extension_version),
      verifier_key(verifier_key) {}

// Used to std::move() ContentHash.
ContentHash::ExtensionKey::ExtensionKey() : verifier_key(nullptr, 0) {}

ContentHash::ExtensionKey::ExtensionKey(
    const ContentHash::ExtensionKey& other) = default;

ContentHash::ExtensionKey& ContentHash::ExtensionKey::operator=(
    const ContentHash::ExtensionKey& other) = default;

// static
ContentHash ContentHash::Create(const ExtensionKey& key) {
  return ContentHash::CreateImpl(key, 0 /* mode */,
                                 false /* create_computed_hashes_file */,
                                 IsCancelledCallback());
}

// static
ContentHash ContentHash::Create(const ExtensionKey& key,
                                int mode,
                                const IsCancelledCallback& is_cancelled) {
  return ContentHash::CreateImpl(
      key, mode, true /* create_computed_hashes_file */, is_cancelled);
}

ContentHash::ContentHash() {}

ContentHash::ContentHash(const ExtensionKey& key, Status status)
    : key_(key), status_(status) {}

ContentHash::ContentHash(
    const ExtensionKey& key,
    std::unique_ptr<VerifiedContents> verified_contents,
    std::unique_ptr<ComputedHashes::Reader> computed_hashes)
    : key_(key),
      verified_contents_(std::move(verified_contents)),
      computed_hashes_(std::move(computed_hashes)) {
  if (!verified_contents_)
    status_ = Status::kInvalid;
  else if (!computed_hashes_)
    status_ = Status::kHasVerifiedContents;
  else
    status_ = Status::kSucceeded;
}

ContentHash::ContentHash(ContentHash&& other)
    : key_(other.key_),
      status_(std::move(other.status_)),
      verified_contents_(std::move(other.verified_contents_)),
      computed_hashes_(std::move(other.computed_hashes_)),
      hash_mismatch_unix_paths_(std::move(other.hash_mismatch_unix_paths_)),
      block_size_(std::move(other.block_size_)) {}

ContentHash& ContentHash::operator=(ContentHash&& other) {
  key_ = other.key_;
  status_ = std::move(other.status_);
  verified_contents_ = std::move(other.verified_contents_);
  computed_hashes_ = std::move(other.computed_hashes_);
  hash_mismatch_unix_paths_ = std::move(other.hash_mismatch_unix_paths_);
  block_size_ = std::move(other.block_size_);
  return *this;
}

ContentHash::~ContentHash() = default;

// static
ContentHash ContentHash::CreateImpl(const ExtensionKey& key,
                                    int mode,
                                    bool create_computed_hashes_file,
                                    const IsCancelledCallback& is_cancelled) {
  base::AssertBlockingAllowed();

  // verified_contents.json:
  base::FilePath verified_contents_path =
      file_util::GetVerifiedContentsPath(key.extension_root);
  if (!base::PathExists(verified_contents_path))
    return ContentHash(key, Status::kInvalid);

  auto verified_contents = std::make_unique<VerifiedContents>(
      key.verifier_key.data, key.verifier_key.size);
  if (!verified_contents->InitFrom(verified_contents_path)) {
    if (mode & static_cast<int>(Mode::kDeleteInvalidVerifiedContents)) {
      if (!base::DeleteFile(verified_contents_path, false))
        LOG(WARNING) << "Failed to delete " << verified_contents_path.value();
    }
    return ContentHash(key, Status::kInvalid);
  }

  // computed_hashes.json: create if necessary.
  base::FilePath computed_hashes_path =
      file_util::GetComputedHashesPath(key.extension_root);

  ContentHash hash(key, std::move(verified_contents), nullptr);
  if (create_computed_hashes_file) {
    // If force recreate wasn't requested, existence of computed_hashes.json is
    // enough, otherwise create computed_hashes.json.
    bool need_to_create =
        (mode & static_cast<int>(
                    Mode::kForceRecreateExistingComputedHashesFile)) != 0 ||
        !base::PathExists(computed_hashes_path);
    if (need_to_create)
      hash.CreateHashes(computed_hashes_path, is_cancelled);
  }

  // computed_hashes.json: read.
  if (!base::PathExists(computed_hashes_path))
    return hash;

  auto computed_hashes = std::make_unique<ComputedHashes::Reader>();
  if (!computed_hashes->InitFromFile(computed_hashes_path))
    return hash;

  hash.status_ = Status::kSucceeded;
  hash.computed_hashes_ = std::move(computed_hashes);

  return hash;
}

bool ContentHash::CreateHashes(const base::FilePath& hashes_file,
                               const IsCancelledCallback& is_cancelled) {
  base::ElapsedTimer timer;
  // Make sure the directory exists.
  if (!base::CreateDirectoryAndGetError(hashes_file.DirName(), nullptr))
    return false;

  base::FileEnumerator enumerator(key_.extension_root, true, /* recursive */
                                  base::FileEnumerator::FILES);
  // First discover all the file paths and put them in a sorted set.
  SortedFilePathSet paths;
  for (;;) {
    if (is_cancelled && is_cancelled.Run())
      return false;

    base::FilePath full_path = enumerator.Next();
    if (full_path.empty())
      break;
    paths.insert(full_path);
  }

  // Now iterate over all the paths in sorted order and compute the block hashes
  // for each one.
  ComputedHashes::Writer writer;
  for (SortedFilePathSet::iterator i = paths.begin(); i != paths.end(); ++i) {
    if (is_cancelled && is_cancelled.Run())
      return false;

    const base::FilePath& full_path = *i;
    base::FilePath relative_unix_path;
    key_.extension_root.AppendRelativePath(full_path, &relative_unix_path);
    relative_unix_path = relative_unix_path.NormalizePathSeparatorsTo('/');

    if (!verified_contents_->HasTreeHashRoot(relative_unix_path))
      continue;

    std::string contents;
    if (!base::ReadFileToString(full_path, &contents)) {
      LOG(ERROR) << "Could not read " << full_path.MaybeAsASCII();
      continue;
    }

    // Iterate through taking the hash of each block of size (block_size_) of
    // the file.
    std::vector<std::string> hashes;
    ComputedHashes::ComputeHashesForContent(contents, block_size_, &hashes);
    std::string root =
        ComputeTreeHashRoot(hashes, block_size_ / crypto::kSHA256Length);
    if (!verified_contents_->TreeHashRootEquals(relative_unix_path, root)) {
      VLOG(1) << "content mismatch for " << relative_unix_path.AsUTF8Unsafe();
      hash_mismatch_unix_paths_.insert(relative_unix_path);
      continue;
    }

    writer.AddHashes(relative_unix_path, block_size_, hashes);
  }
  bool result = writer.WriteToFile(hashes_file);
  UMA_HISTOGRAM_TIMES("ExtensionContentHashFetcher.CreateHashesTime",
                      timer.Elapsed());

  if (result)
    status_ = Status::kSucceeded;

  return result;
}

const VerifiedContents& ContentHash::verified_contents() const {
  DCHECK(status_ >= Status::kHasVerifiedContents && verified_contents_);
  return *verified_contents_;
}

ComputedHashes::Reader& ContentHash::computed_hashes() const {
  DCHECK(status_ == Status::kSucceeded && computed_hashes_);
  return *computed_hashes_;
}

}  // namespace extensions
