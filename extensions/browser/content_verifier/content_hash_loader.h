// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef _EXTENSIONS_BROWSER_CONTENT_VERIFIER_CONTENT_HASH_LOADER_H_
#define _EXTENSIONS_BROWSER_CONTENT_VERIFIER_CONTENT_HASH_LOADER_H_

#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/callback_forward.h"
#include "base/memory/weak_ptr.h"
#include "base/callback.h"
#include "base/files/file_path.h"
#include "base/version.h"
#include "extensions/browser/content_verifier_delegate.h"
#include "extensions/browser/computed_hashes.h"
//#include "extensions/browser/content_verifier.h"
//#include "extensions/browser/content_verifier_delegate.h"
#include "net/url_request/url_fetcher_delegate.h"

#include "base/sequence_checker.h"

//struct ContentVerifierKey;

namespace crypto {
class SecureHash;
}

namespace net {
class URLRequestContextGetter;
class URLFetcher;
}

namespace extensions {
class ContentVerifier;
class VerifiedContents;
class ContentVerifierDelegate;

struct ContentHashRunResult {
  ContentHashRunResult();
  ~ContentHashRunResult();

  bool GetHashForBlock(int block_index,
                       const std::string** result) const;

  // Result related.
  bool file_missing_from_verified_contents = false;
  bool hash_mismatch = false;
  bool ok = false;

  // The blocksize used for generating the hashes.
  int block_size;
  std::vector<std::string> hashes_;

  int block_count() const;
};

// TODO: Own file.
// NOTE: RefCounted: TODO.
class ContentHash : public base::RefCounted<ContentHash> {
 public:
  ContentHash();

  void SetVerifiedContents(std::unique_ptr<VerifiedContents> verified_contents);
  void SetComputedHashes(
      std::unique_ptr<ComputedHashes::Reader> computed_hashes);

  VerifiedContents& verified_contents() { return *verified_contents_; }
  ComputedHashes::Reader& computed_hashes() { return *computed_hashes_; }

  void RunHash(const base::FilePath& relative_path,
               ContentHashRunResult* result);

 private:
  friend class base::RefCounted<ContentHash>;
  ~ContentHash();

  // VerifiedContents verified_contents
  std::unique_ptr<VerifiedContents> verified_contents_;
  std::unique_ptr<ComputedHashes::Reader> computed_hashes_;

  DISALLOW_COPY_AND_ASSIGN(ContentHash);
};

class ContentHashLoader {
 public:
  enum class LoadResult {
    kFailed,
    kOk,
  };
  using LoadCallback =
      base::OnceCallback<void(LoadResult /* result */,
                              scoped_refptr<ContentHash> /* hash */)>;
  ContentHashLoader(
      ContentVerifierDelegate* delegate,
      net::URLRequestContextGetter* request_context,
      const std::string& extension_id,
      const base::Version& extension_version,
      const base::FilePath& extension_root,
      const base::FilePath& relative_path,
      const ContentVerifierKey& key);
  ~ContentHashLoader();

  void Load(LoadCallback callback);

 private:
  class ContentHashResult {
   public:
    enum class ReadResult {
      kNone,
      kNoVerifiedContents,
      kInvalidVerifiedContents,
      kGotVerifiedContents,
      kNoComputedHashes,
      kInvalidComputedHashes,
      kOk,
    };

    ContentHashResult();
    ContentHashResult(ReadResult read_result);
    ~ContentHashResult();

    ReadResult read_result;
    scoped_refptr<ContentHash> content_hash;
    void set_read_result(ReadResult value) {
      read_result = value;
    }

   private:
    DISALLOW_COPY_AND_ASSIGN(ContentHashResult);
  };
  struct CreateComputedHashesResult {
    bool success;
    std::set<base::FilePath> hash_mismatch_unix_paths;
    CreateComputedHashesResult();
    ~CreateComputedHashesResult();
  };
  static void CreateComputedHashesOnBlockingSequence(
      const base::FilePath& extension_root,
      int block_size_,
      //ContentHash* content_hash_ptr,
      scoped_refptr<ContentHash> content_hash_ptr,
      CreateComputedHashesResult* result,
      bool* result_bool);
  void LoadImpl(bool should_fetch_on_failure, LoadCallback callback);

  void OnReadContentHash(bool should_fetch_on_failure,
                         std::unique_ptr<ContentHashResult> result);

  // |is_hash_mismatch| = false means hash loading failed.
  void DispatchFailureCallback(bool is_hash_mismatch);

  static std::unique_ptr<ContentHashResult> ReadContentHash(
      const std::string& extension_id,
      const base::Version& extension_version,
      const base::FilePath& extension_root,
      const base::FilePath& relative_path,
      const ContentVerifierKey& key);

  void FetchVerifiedContentsThenCreateComputedHashes();
  void CreateComputedHashes(scoped_refptr<ContentHash> content_hash);
  //void CreateComputedHashesOnBlockingSequence(
  //    const base::FilePath& extension_root,
  //    //ContentHash* content_hash_ptr,
  //    scoped_refptr<ContentHash> content_hash_ptr,
  //    bool* result);
  void CreatedComputedHashes(//std::unique_ptr<ContentHash> content_hash,
                             scoped_refptr<ContentHash> content_hash,
                             std::unique_ptr<CreateComputedHashesResult> result,
                             bool* result_bool);
  void FetchComplete(bool fetch_result,
                     std::unique_ptr<std::string> response);
  void OnVerifiedContentsWritten(size_t expected_size, int write_result);
  void VerifiedContentsWriteComplete(bool success);

  ContentVerifierDelegate* const delegate_;
  net::URLRequestContextGetter* const request_context_;
  std::string extension_id_;
  base::Version extension_version_;
  base::FilePath extension_root_;
  base::FilePath relative_path_;
  ContentVerifierKey key_;


  LoadCallback callback_;

  // The block size to use for hashing.
  // COPIED FROM: ContenteHashFetcherJob::block_size_.
  const int block_size_ = 4096;

  class Fetcher;

  bool has_fetched_verified_contents_ = false;

  std::unique_ptr<Fetcher> fetcher_;

  base::WeakPtrFactory<ContentHashLoader> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(ContentHashLoader);
};

// NOTE: RefCounted: hash fetch (1) and DoneReading (2) can be out of order, so
// these two will add ref to |this|, ExtensionProtocol will hold this with
// scoped_refptr.
class VerifierData : public base::RefCounted<VerifierData> {
 public:
  using FailureCallback = base::OnceClosure;
  VerifierData(const std::string& extension_id,
               //FailureCallback verify_failed_callback);
               const base::Version& extension_version,
               const base::FilePath& extension_root,
               const base::FilePath& relative_path,
               const ContentVerifierKey& key,
               scoped_refptr<ContentVerifier> verifier);

  // Must be called on IO thread.
  void Start();
  void BytesRead(int count, const char* data);
  void DoneReading();

 private:
  friend class base::RefCounted<VerifierData>;
  ~VerifierData();

  void GotContentVerificationHash(ContentHashLoader::LoadResult result,
                                  scoped_refptr<ContentHash> content_hash);

  void Complete();
  bool FinishBlock();

  void DispatchFailureCallback(bool hash_mismatch);

  //FailureCallback failure_callback_;
  std::string extension_id_;
  base::Version extension_version_;
  base::FilePath extension_root_;
  base::FilePath relative_path_;
  ContentVerifierKey key_;
  scoped_refptr<ContentVerifier> content_verifier_;

  bool done_reading_ = false;

  // While we're waiting for the callback from the ContentHashReader, we need
  // to queue up bytes any bytes that are read.
  std::string queue_;

  // The total bytes we've read.
  int64_t total_bytes_read_ = 0;

  // The index of the block we're currently on.
  int current_block_ = 0;

  // The hash we're building up for the bytes of |current_block_|.
  std::unique_ptr<crypto::SecureHash> current_hash_;

  // The number of bytes we've already input into |current_hash_|.
  int current_hash_byte_count_ = 0;

  // TODO:
//  bool completed_ = false;

  // TODO: Combine with |content_hash_|.
  bool hashes_ready_ = false;

  // TODO: Not entirely correct, copied from ContentHashFetcher.
  //const int block_size_ = 4096;

  ContentHashRunResult content_hash_run_result_;

  scoped_refptr<ContentHash> content_hash_;

  SEQUENCE_CHECKER(sequence_checker_);

  DISALLOW_COPY_AND_ASSIGN(VerifierData);
};



}  // namespace extensions

#endif  // _EXTENSIONS_BROWSER_CONTENT_VERIFIER_CONTENT_HASH_LOADER_H_
