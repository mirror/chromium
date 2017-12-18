// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "extensions/browser/content_verifier/content_hash_loader.h"

#include "content/public/browser/browser_thread.h"
#include "base/task_scheduler/post_task.h"
#include "extensions/browser/verified_contents.h"
#include "extensions/browser/content_verifier.h"
#include "extensions/browser/verified_contents.h"

#include "extensions/browser/content_hash_tree.h"

#include "base/files/file_util.h"
#include "extensions/common/file_util.h"


#include "crypto/secure_hash.h"
#include "crypto/sha2.h"

#include "net/url_request/url_request_context_getter.h"
#include "extensions/browser/content_verifier_delegate.h"
#include "net/url_request/url_fetcher.h"
#include "net/traffic_annotation/network_traffic_annotation.h"
#include "net/base/load_flags.h"
#include "net/url_request/url_request_status.h"
#include "base/json/json_reader.h"


#include "base/files/file_enumerator.h"
#include "base/files/file_util.h"
#include "extensions/browser/extension_file_task_runner.h"

namespace {

typedef std::set<base::FilePath> SortedFilePathSet;

// Helper function to let us pass ownership of a string via base::Bind with the
// contents to be written into a file. Also ensures that the directory for
// |path| exists, creating it if needed.
static int WriteFileHelper(const base::FilePath& path,
                           std::unique_ptr<std::string> content) {
  base::FilePath dir = path.DirName();
  if (!base::CreateDirectoryAndGetError(dir, nullptr))
    return -1;
  return base::WriteFile(path, content->data(), content->size());
}


}  // namespace

namespace extensions {

ContentHashRunResult::ContentHashRunResult() {
}

ContentHashRunResult::~ContentHashRunResult() {
}

int ContentHashRunResult::block_count() const {
  DCHECK(ok);
  return hashes_.size();
}

bool ContentHashRunResult::GetHashForBlock(int block_index,
                                           const std::string** result) const {
  if (!ok)
    return false;
  DCHECK(block_index >= 0);

  if (static_cast<unsigned>(block_index) >= hashes_.size())
    return false;
  *result = &hashes_[block_index];

  return true;
}


ContentHash::ContentHash() {
}

ContentHash::~ContentHash() {
}

void ContentHash::SetVerifiedContents(
    std::unique_ptr<VerifiedContents> verified_contents) {
  verified_contents_ = std::move(verified_contents);
}

void ContentHash::SetComputedHashes(
    std::unique_ptr<ComputedHashes::Reader> reader) {
  computed_hashes_ = std::move(reader);
}

void ContentHash::RunHash(const base::FilePath& relative_path,
                          ContentHashRunResult* result) {
  DCHECK(verified_contents_.get());
  DCHECK(computed_hashes_.get());

  if (!verified_contents_->HasTreeHashRoot(relative_path)) {
    result->file_missing_from_verified_contents = true;
    return;
  }

  if (!computed_hashes_->GetHashes(relative_path, &result->block_size,
                                   &result->hashes_) ||
      result->block_size % crypto::kSHA256Length != 0) {
    // TODO: Though this is error retrieving hash.
    result->hash_mismatch = true;
    return;
  }

  std::string root =
      ComputeTreeHashRoot(result->hashes_, result->block_size / crypto::kSHA256Length);
  if (!verified_contents_->TreeHashRootEquals(relative_path, root)) {
    result->hash_mismatch = true;
    return;
  }

  result->ok = true;
}

class ContentHashLoader::Fetcher : public net::URLFetcherDelegate {
 public:
  using Callback = base::OnceCallback<void(bool /* fetch_result */,
                                           std::unique_ptr<std::string> /* response */)>;
  Fetcher(Callback callback) : callback_(std::move(callback)) {}
  ~Fetcher() override {}

  void Fetch(const GURL& fetch_url,
             net::URLRequestContextGetter* request_context) {
    // TODO: Do we need to run this on UI?
    net::NetworkTrafficAnnotationTag traffic_annotation =
        net::DefineNetworkTrafficAnnotation("content_hash_verification_job", R"(
          semantics {
            sender: "Web Store Content Verification"
            description:
              "The request sent to retrieve the file required for content "
              "verification for an extension from the Web Store."
            trigger:
              "An extension from the Web Store is missing the "
              "verified_contents.json file required for extension content "
              "verification."
            data: "The extension id and extension version."
            destination: GOOGLE_OWNED_SERVICE
          }
          policy {
            cookies_allowed: NO
            setting:
              "This feature cannot be directly disabled; it is enabled if any "
              "extension from the webstore is installed in the browser."
            policy_exception_justification:
              "Not implemented, not required. If the user has extensions from "
              "the Web Store, this feature is required to ensure the "
              "extensions match what is distributed by the store."
          })");
    url_fetcher_ = net::URLFetcher::Create(fetch_url, net::URLFetcher::GET,
                                           this, traffic_annotation);
    url_fetcher_->SetRequestContext(request_context);
    url_fetcher_->SetLoadFlags(net::LOAD_DO_NOT_SEND_COOKIES |
                               net::LOAD_DO_NOT_SAVE_COOKIES |
                               net::LOAD_DISABLE_CACHE);
    url_fetcher_->SetAutomaticallyRetryOnNetworkChanges(3);
    url_fetcher_->Start();
  }

  // URLFetcherDelegate:
  void OnURLFetchComplete(const net::URLFetcher* source) override {
    DCHECK_CURRENTLY_ON(content::BrowserThread::IO);
    printf("**** new URLFetchComplete...\n");
    printf("IS_IO: %d\n", content::BrowserThread::CurrentlyOn(content::BrowserThread::IO));
    printf("IS_UI: %d\n", content::BrowserThread::CurrentlyOn(content::BrowserThread::UI));
    std::unique_ptr<net::URLFetcher> url_fetcher = std::move(url_fetcher_);
    printf("OnURLFetchComplete\n");
    std::unique_ptr<std::string> response(new std::string);
    if (!url_fetcher->GetStatus().is_success() ||
        !url_fetcher->GetResponseAsString(response.get())) {
      //DoneFetchingVerifiedContents(false);
      return;
    }
    //printf("*** response = %s\n", response->c_str());

    // Parse the response to make sure it is valid json (on staging sometimes it
    // can be a login redirect html, xml file, etc. if you aren't logged in with
    // the right cookies).  TODO(asargent) - It would be a nice enhancement to
    // move to parsing this in a sandboxed helper (crbug.com/372878).
    std::unique_ptr<base::Value> parsed(base::JSONReader::Read(*response));
    if (parsed) {
      printf("JSON parsed ok\n");
      parsed.reset();  // no longer needed
      std::move(callback_).Run(true /* fetch_result */, std::move(response));
    } else {
      //DoneFetchingVerifiedContents(false);
      std::move(callback_).Run(false /* fetch_result */, nullptr);
    }
  }
 private:
  // Used for fetching content signatures.
  // TODO: THREADING ISSUES.
  std::unique_ptr<net::URLFetcher> url_fetcher_;

  Callback callback_;

  DISALLOW_COPY_AND_ASSIGN(Fetcher);
};


ContentHashLoader::ContentHashLoader(
    ContentVerifierDelegate* delegate,
    net::URLRequestContextGetter* request_context,
    const std::string& extension_id,
    const base::Version& extension_version,
    const base::FilePath& extension_root,
    const base::FilePath& relative_path,
    const ContentVerifierKey& key)
    :
      delegate_(delegate),
      request_context_(request_context),
      extension_id_(extension_id),
      extension_version_(extension_version),
      extension_root_(extension_root),
      relative_path_(relative_path),
      key_(key),
      weak_factory_(this) {
  printf("ContentHashLoader [%p]\n", this);
}

ContentHashLoader::~ContentHashLoader() {
}

void ContentHashLoader::LoadImpl(bool should_fetch_on_failure,
                                 LoadCallback callback) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::IO);
  printf("CHL:LoadImpl [%p]\n", this);
  DCHECK(!callback_);
  callback_ = std::move(callback);
  base::PostTaskWithTraitsAndReplyWithResult(
      FROM_HERE, {base::MayBlock(), base::TaskPriority::USER_VISIBLE},
      base::BindOnce(&ContentHashLoader::ReadContentHash,
                     extension_id_,
                     extension_version_,
                     extension_root_,
                     relative_path_,
                     key_),
      base::BindOnce(&ContentHashLoader::OnReadContentHash,
                     weak_factory_.GetWeakPtr(),
                     should_fetch_on_failure));
}

void ContentHashLoader::Load(LoadCallback callback) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::IO);
  LoadImpl(true /* should_fetch_on_failure */, std::move(callback));
}

ContentHashLoader::ContentHashResult::ContentHashResult()
    : read_result(ContentHashResult::ReadResult::kNone),
      content_hash(new ContentHash) {
}

ContentHashLoader::ContentHashResult::ContentHashResult(
    ContentHashResult::ReadResult read_result)
  : read_result(read_result),
    content_hash(new ContentHash) {
}

ContentHashLoader::ContentHashResult::~ContentHashResult() {}

// static.
std::unique_ptr<ContentHashLoader::ContentHashResult>
ContentHashLoader::ReadContentHash(
    const std::string& extension_id,
    const base::Version& extension_version,
    const base::FilePath& extension_root,
    const base::FilePath& relative_path,   // UNUSED.
    const ContentVerifierKey& key) {
  base::FilePath verified_contents_path =
      file_util::GetVerifiedContentsPath(extension_root);
  // TODO: Is this PathExists check necessary? Do we need to differentiate
  // between kNoVerifiedContents and kInvalidVerifiedContents.
  if (!base::PathExists(verified_contents_path))
    return std::make_unique<ContentHashResult>(
        ContentHashResult::ReadResult::kNoVerifiedContents);


  auto result = std::make_unique<ContentHashResult>();
  std::unique_ptr<VerifiedContents> verified_contents =
      VerifiedContents::CreateFromFile(key.data,
                                       key.size,
                                       verified_contents_path);
  if (!verified_contents) {
    return std::make_unique<ContentHashResult>(
        ContentHashResult::ReadResult::kInvalidVerifiedContents);
  }
  result->content_hash->SetVerifiedContents(std::move(verified_contents));
  //result->content_hash->SetVerifiedContents(
  //    std::make_unique<VerifiedContents>(key.data, key.size));
  //VerifiedContents& verified_contents = result->content_hash->verified_contents();
  //if (!verified_contents.InitFrom(verified_contents_path) ||
  //    !verified_contents.valid_signature() ||
  //    verified_contents.version() != extension_version ||
  //    verified_contents.extension_id() != extension_id) {
  //  return std::make_unique<ContentHashResult>(
  //      ContentHashResult::ReadResult::kInvalidVerifiedContents);
  //}
  result->set_read_result(ContentHashResult::ReadResult::kGotVerifiedContents);

  //have_verified_contents_ = true;


  // Computed hashes.
  base::FilePath computed_hashes_path =
      file_util::GetComputedHashesPath(extension_root);
  if (!base::PathExists(computed_hashes_path)) {
    return std::make_unique<ContentHashResult>(
        ContentHashResult::ReadResult::kNoComputedHashes);
  }

  std::unique_ptr<ComputedHashes::Reader> computed_hashes =
      ComputedHashes::Reader::CreateFromFile(computed_hashes_path);
  if (!computed_hashes) {
    return std::make_unique<ContentHashResult>(
        ContentHashResult::ReadResult::kInvalidComputedHashes);
  }
  result->content_hash->SetComputedHashes(std::move(computed_hashes));
  //result->content_hash->SetComputedHashes(
  //    std::make_unique<ComputedHashes::Reader>());
  //ComputedHashes::Reader& reader = result->content_hash->computed_hashes();
  //if (!reader.InitFromFile(computed_hashes_path)) {
  //  return std::make_unique<ContentHashResult>(
  //      ContentHashResult::ReadResult::kInvalidComputedHashes);
  //}
  result->set_read_result(ContentHashResult::ReadResult::kOk);

  //have_computed_hashes_ = true;
  return result;
}

void ContentHashLoader::OnReadContentHash(
    bool should_fetch_on_failure,
    std::unique_ptr<ContentHashResult> result) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::IO);
  printf("*********** OnReadContentHash: result.read_result = %d\n",
         result->read_result);
//  DCHECK(callback_);
//  LoadResult load_result = result->read_result == ContentHashResult::ReadResult::kOk ?
//      LoadResult::kOk : LoadResult::kFailed;
//  std::move(callback_).Run(load_result, std::move(result->content_hash));

  {  // Test only.
    //FetchAndCreateHashes();
    //pending_result_ = std::move(result);
    FetchVerifiedContentsThenCreateComputedHashes();  // INFINITE loop.
    return;
  }


  if (result->read_result != ContentHashResult::ReadResult::kOk) {
    // Attempt to fetch verified_contents.json and create computed_hashes.json
    // as appropriate.
    if (result->read_result >= ContentHashResult::ReadResult::kGotVerifiedContents) {
      // Already have verified_contents.json, but computed hashes is missing.
      // Only need to compute hashes.
      // TODO: Is this recipe for other bugs? Old code seems to do this, see
      // ContentHashFetcherJob::LoadVerifiedContents.
      // OLD: Follow DoneFetchingVerifiedContents(true);
      CreateComputedHashes(result->content_hash);
    } else {
      if (should_fetch_on_failure) {
        FetchVerifiedContentsThenCreateComputedHashes();
      } else {
        std::move(callback_).Run(LoadResult::kFailed,
                                 std::move(result->content_hash));
      }
    }
  } else {
    DCHECK(callback_);
    std::move(callback_).Run(LoadResult::kOk,
                             std::move(result->content_hash));
  }
}

void ContentHashLoader::FetchVerifiedContentsThenCreateComputedHashes() {
  GURL fetch_url = delegate_->GetSignatureFetchUrl(
      extension_id_, extension_version_);
  fetcher_ = std::make_unique<Fetcher>(
      base::BindOnce(&ContentHashLoader::FetchComplete, weak_factory_.GetWeakPtr()));
  fetcher_->Fetch(fetch_url, request_context_);
}

void ContentHashLoader::FetchComplete(bool fetch_result,
                                      std::unique_ptr<std::string> response) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::IO);  // |weak_factory_| deferenced.
  std::unique_ptr<Fetcher> fetcher = std::move(fetcher_);
  if (fetch_result) {
    printf("**********************************************\n");
    printf("**** new FetchComplete: response = %s\n",
           response->substr(0, 255).c_str());
    printf("**********************************************\n");
    base::FilePath destination =
        file_util::GetVerifiedContentsPath(extension_root_);
    size_t size = response->size();
    base::PostTaskWithTraitsAndReplyWithResult(
        FROM_HERE, {base::MayBlock(), base::TaskPriority::USER_VISIBLE},
        base::Bind(&WriteFileHelper, destination, base::Passed(&response)),
        base::Bind(&ContentHashLoader::OnVerifiedContentsWritten,
                   weak_factory_.GetWeakPtr(),
                   size));
  } else {
    // TODO: Propagate global failure.
  }
}

void ContentHashLoader::OnVerifiedContentsWritten(size_t expected_size,
                                                  int write_result) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::IO);
  bool success =
      (write_result >= 0 && static_cast<size_t>(write_result) == expected_size);
  VerifiedContentsWriteComplete(success);
}

void ContentHashLoader::VerifiedContentsWriteComplete(bool success) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::IO);
  if (success) {
    // We have written verified_contents.json, now we need to start reading from
    // the beginning of |ContentHashLoader|. But make sure we don't try to
    // fetch verified_contents.json again in case of any interim failure.
    LoadCallback callback = std::move(callback_);
    LoadImpl(false /* should_fetch_on_failure */, std::move(callback));
  } else {
    // TODO: Pass to global failure.
  }
}

ContentHashLoader::CreateComputedHashesResult::CreateComputedHashesResult() {}
ContentHashLoader::CreateComputedHashesResult::~CreateComputedHashesResult() {}

// static.
void ContentHashLoader::CreateComputedHashesOnBlockingSequence(
    const base::FilePath& extension_root,
    int block_size_,
    //ContentHash* content_hash_ptr,
    scoped_refptr<ContentHash> content_hash_ptr,
    CreateComputedHashesResult* result,
    bool* result_bool) {
  base::FilePath hashes_file =
      file_util::GetComputedHashesPath(extension_root);
  if (base::PathExists(hashes_file)) {
    // TODO: What if DeleteFile fails?
    base::DeleteFile(hashes_file, false /* recursive */);
  }
  if (!base::CreateDirectoryAndGetError(hashes_file.DirName(), nullptr)) {
    *result_bool = false;
    return;
  }
  //DCHECK(content_hash_ptr->verified_contents() != nullptr);

  // TODO: DROPPED: base::FileEnumerator enumerator(extension_root...
  base::FileEnumerator enumerator(extension_root,
                                  true, /* recursive */
                                  base::FileEnumerator::FILES);
  // First discover all the file paths and put them in a sorted set.
  SortedFilePathSet paths;
  while (true) {
    base::FilePath full_path = enumerator.Next();
    if (full_path.empty())
      break;
    paths.insert(full_path);
  }
  // Now iterate over all the paths in sorted order and compute the block hashes
  // for each one.
  ComputedHashes::Writer writer;
  for (SortedFilePathSet::iterator i = paths.begin(); i != paths.end(); ++i) {
    const base::FilePath& full_path = *i;
    base::FilePath relative_unix_path;
    extension_root.AppendRelativePath(full_path, &relative_unix_path);
    relative_unix_path = relative_unix_path.NormalizePathSeparatorsTo('/');

    if (!content_hash_ptr->verified_contents().HasTreeHashRoot(
        relative_unix_path)) {
      continue;
    }

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
    if (!content_hash_ptr->verified_contents().TreeHashRootEquals(
            relative_unix_path, root)) {
      VLOG(1) << "content mismatch for " << relative_unix_path.AsUTF8Unsafe();
      // TODO: Hash mismatch unix paths.
      //hash_mismatch_unix_paths_.insert(relative_unix_path);
      result->hash_mismatch_unix_paths.insert(relative_unix_path);
      continue;
    }

    writer.AddHashes(relative_unix_path, block_size_, hashes);
  }

  *result_bool = writer.WriteToFile(hashes_file);
  result->success = *result_bool;
  //UMA_HISTOGRAM_TIMES("ExtensionContentHashFetcher.CreateHashesTime",
  //                    timer.Elapsed());
}

void ContentHashLoader::CreateComputedHashes(
    scoped_refptr<ContentHash> content_hash) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::IO);  // We access weak_factory_.
  //ContentHash* content_hash_ptr = content_hash.get();
  bool* result_b = new bool;
  auto result = std::make_unique<CreateComputedHashesResult>();
  // history_service.cc: example.
  // c/b/chromeos/base/locale_util.cc: example.
  GetExtensionFileTaskRunner()->PostTaskAndReply(
      FROM_HERE,
      base::BindOnce(&ContentHashLoader::CreateComputedHashesOnBlockingSequence,
                 extension_root_,
                 block_size_,
                 //content_hash_ptr,
                 content_hash,
                 base::Unretained(result.get()),
                 base::Unretained(result_b)),
      base::BindOnce(&ContentHashLoader::CreatedComputedHashes,
                 weak_factory_.GetWeakPtr(),
                 //base::Passed(&content_hash),
                 content_hash,
                 std::move(result),
                 base::Owned(result_b)));
}


void ContentHashLoader::CreatedComputedHashes(
    //std::unique_ptr<ContentHash> content_hash,
    scoped_refptr<ContentHash> content_hash,
    std::unique_ptr<CreateComputedHashesResult> result,
    bool* result_bool) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::IO);
  // ZZZ: CONTINUE.
  if (!*result_bool) {
  }
  //if (!result->success) {
  //} else {
  //  // TODO: Look at result->hash_mismatch_unix_paths.
  //  // ContentVerifier::ShouldVerifyAnyPaths(result->hash_mismatch_unix_paths)
  //}
}

// ---- VerifierData ----
VerifierData::VerifierData(const std::string& extension_id,
                           const base::Version& extension_version,
                           const base::FilePath& extension_root,
                           const base::FilePath& relative_path,
                           const ContentVerifierKey& key,
                           scoped_refptr<ContentVerifier> verifier)
    : extension_id_(extension_id),
      extension_version_(extension_version),
      extension_root_(extension_root),
      relative_path_(relative_path),
      key_(key),
      content_verifier_(verifier) {
}
VerifierData::~VerifierData() {}

void VerifierData::Start(
    //const base::Version& extension_version,
    ) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  printf("VerifierData::Start\n");
  content_verifier_->GetContentVerificationHash(
      extension_id_,
      extension_version_,
      extension_root_,
      relative_path_,
      key_,
      base::Bind(&VerifierData::GotContentVerificationHash, this));
}

void VerifierData::BytesRead(int count, const char* data) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  //DCHECK(thread_checker_.CalledOnValidThread());
  //DCHECK(!completed_);
  if (!hashes_ready_) {
    queue_.append(data, count);
    return;
  }
  DCHECK_GE(count, 0);
  int bytes_added = 0;
  DCHECK(content_hash_run_result_.ok);

  while (bytes_added < count) {
    if (current_block_ >= content_hash_run_result_.block_count())
      return DispatchFailureCallback(true /* hash_mismatch */);

    if (!current_hash_) {
      current_hash_byte_count_ = 0;
      current_hash_ = crypto::SecureHash::Create(crypto::SecureHash::SHA256);
    }
    // Compute how many bytes we should hash, and add them to the current hash.
    int bytes_to_hash =
        std::min(content_hash_run_result_.block_size - current_hash_byte_count_,
                 count - bytes_added);
    DCHECK_GT(bytes_to_hash, 0);
    current_hash_->Update(data + bytes_added, bytes_to_hash);
    bytes_added += bytes_to_hash;
    current_hash_byte_count_ += bytes_to_hash;
    total_bytes_read_ += bytes_to_hash;

    // If we finished reading a block worth of data, finish computing the hash
    // for it and make sure the expected hash matches.
    if (current_hash_byte_count_ == content_hash_run_result_.block_size &&
        !FinishBlock()) {
      DispatchFailureCallback(true /* hash_mismatch */);
      return;
    }
  }
}

void VerifierData::DoneReading() {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  done_reading_ = true;
  if (hashes_ready_) {
    if (!FinishBlock()) {
      DispatchFailureCallback(true);  // hash mismatch.
      return;
    }
  }
}

void VerifierData::DispatchFailureCallback(bool hash_mismatch) {
  CHECK(0);  // DEAL WITH THIS.
  if (!hash_mismatch) {
    // Hash loading failed.
    // TODO: What to do here?
    // Only disable if ENFORCE_STRICT. Let ContentVerifier decide?
  }
}

void VerifierData::GotContentVerificationHash(
    ContentHashLoader::LoadResult result,
    scoped_refptr<ContentHash> content_hash) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  printf("**** GotContentVerificationHash: content_hash: [%p]\n",
         content_hash.get());
  content_hash_ = content_hash;  // Fix.
  if (result == ContentHashLoader::LoadResult::kOk) {
    printf("OK!, done_reading_ = %d\n", done_reading_);
    hashes_ready_ = true;
    content_hash_->RunHash(relative_path_, &content_hash_run_result_);

    // TODO:
    if (!content_hash_run_result_.ok) {
      // WHAT DO WE DO HERE.
      // Previously this would be ~ ContentVerifyJob::OnHashesReady(false).
      DispatchFailureCallback(false);
      return;
    }

    // Copy from previous code (ContentVerifyJob::OnHashesReady).
    if (!queue_.empty()) {
      std::string tmp;
      queue_.swap(tmp);
      BytesRead(tmp.size(), base::string_as_array(&tmp));
    }
    if (done_reading_) {
      if (!FinishBlock()) {
        DispatchFailureCallback(true);  // Hash mismatch.
      }
    } else {
      content_hash_ = content_hash;
    }
  } else {
    DispatchFailureCallback(false);
  }
}

void VerifierData::Complete() {
  DCHECK(done_reading_);
  DCHECK(content_hash_.get());
}

bool VerifierData::FinishBlock() {
  DCHECK(content_hash_run_result_.ok);
  if (current_hash_byte_count_ == 0) {
    if (!done_reading_ ||
        // If we have checked all blocks already, then nothing else to do here.
        current_block_ == content_hash_run_result_.block_count()) {
      return true;
    }
  }
  if (!current_hash_) {
    // This happens when we fail to read the resource. Compute empty content's
    // hash in this case.
    current_hash_ = crypto::SecureHash::Create(crypto::SecureHash::SHA256);
  }
  std::string final(crypto::kSHA256Length, 0);
  current_hash_->Finish(base::string_as_array(& final), final.size());
  current_hash_.reset();
  current_hash_byte_count_ = 0;

  int block = current_block_++;

  const std::string* expected_hash = NULL;
  if (!content_hash_run_result_.GetHashForBlock(block, &expected_hash))
    return false;

  if (*expected_hash == final) {
    printf("**** HASH MATCH ****\n");
  } else {
    printf("**** mismatch :( ****\n");
  }
//  if (!content_hash_run_result_.GetHashForBlock(block, &expected_hash) ||
//      *expected_hash != final) {
//    return false;
//  }

  return true;
}


}  // namespace extensions
