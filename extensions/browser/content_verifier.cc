// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "extensions/browser/content_verifier.h"

#include <algorithm>
#include <utility>
#include <vector>

#include "base/files/file_path.h"
#include "base/stl_util.h"
#include "base/strings/string_util.h"
#include "content/public/browser/browser_context.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/storage_partition.h"
#include "extensions/browser/content_hash_fetcher.h"
#include "extensions/browser/content_hash_reader.h"
#include "extensions/browser/content_verifier_delegate.h"
#include "extensions/browser/content_verifier_io_data.h"
#include "extensions/browser/extension_registry.h"
#include "extensions/browser/management_policy.h"
#include "extensions/common/constants.h"
#include "extensions/common/extension_l10n_util.h"
#include "extensions/common/file_util.h"

//#include "extensions/browser/content_verify_job.h"  // VerifierData.

namespace extensions {

namespace {

ContentVerifier::TestObserver* g_test_observer = NULL;

// This function converts paths like "//foo/bar", "./foo/bar", and
// "/foo/bar" to "foo/bar". It also converts path separators to "/".
base::FilePath NormalizeRelativePath(const base::FilePath& path) {
  if (path.ReferencesParent())
    return base::FilePath();

  std::vector<base::FilePath::StringType> parts;
  path.GetComponents(&parts);
  if (parts.empty())
    return base::FilePath();

  // Remove the first component if it is '.' or '/' or '//'.
  const base::FilePath::StringType separators(
      base::FilePath::kSeparators, base::FilePath::kSeparatorsLength);
  if (!parts[0].empty() &&
      (parts[0] == base::FilePath::kCurrentDirectory ||
       parts[0].find_first_not_of(separators) == std::string::npos))
    parts.erase(parts.begin());

  // Note that elsewhere we always normalize path separators to '/' so this
  // should work for all platforms.
  return base::FilePath(
      base::JoinString(parts, base::FilePath::StringType(1, '/')));
}

}  // namespace

// static
bool ContentVerifier::ShouldRepairIfCorrupted(
    const ManagementPolicy* management_policy,
    const Extension* extension) {
  return management_policy->MustRemainEnabled(extension, nullptr) ||
         management_policy->MustRemainInstalled(extension, nullptr);
}

// static
void ContentVerifier::SetObserverForTests(TestObserver* observer) {
  g_test_observer = observer;
}

ContentVerifier::ContentVerifier(
    content::BrowserContext* context,
    std::unique_ptr<ContentVerifierDelegate> delegate)
    : shutdown_(false),
      context_(context),
      delegate_(std::move(delegate)),
      fetcher_(new ContentHashFetcher(
          content::BrowserContext::GetDefaultStoragePartition(context)
              ->GetURLRequestContext(),
          delegate_.get(),
          base::Bind(&ContentVerifier::OnFetchComplete, this))),
      request_context_(
          content::BrowserContext::GetDefaultStoragePartition(context)
              ->GetURLRequestContext()),
      observer_(this),
      io_data_(new ContentVerifierIOData) {}

ContentVerifier::~ContentVerifier() {
}

/*
void ContentVerifier::DoVerify(
    const std::string& extension_id,
    // Maybe no callback needed? ContentVerifier can decide itself.
    VerifyCallback callback) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::IO);
  // TODO: What about version.
  if (inflight_.find(extension_id) != inflight_.end()) {
    inflight_[extension_id].callbacks_.push_back(std::move(callback));
    return;
  }
  io_checker_.Check(extension_id,
                    base::BindOnce(&ContentVerifier::DoneVerify, this));
}
void ContentVerifier::DoVerify2(
    const std::string& extension_id, FilePath& relative_path,
    VerifyCallback callback) {
  // TRICKY, need to go to FILE thread for single relative_path.
  // That is the common case.
}

void ContentVerifier::DoneVerify(VerifyResult result,
                                 std::unique_ptr<VerifyData> data) {
  // Process each callback, prefer calling verify failed once.
}

// TODO: bool create?
// TODO: const CVHashes might be important.
using GotHash =
    base::OnceCallback<void(CVHResult result, scoped_refptr<const CVHashes> data)>;
void ContentVerifier::GetContentVerificationHash(
    const std::string& extension_id,
    const base::Version& extension_version,
    const base::FilePath& extension_root,
    GotHashCallback callback) {
  if (!IO) PostTask(IO); OR: DCHECK(IO);
  // TODO: How to make |container| strong owned?
  Key key(extension_id, extension_version, extension_root);
  if (!inflight) {
    auto loader = std::make_unique<ContentHashLoader>(extension_id,
        // WeakPtr would be better for CV::Shutdown.
        base::Bind(&GotHashHelper, this/WeakPtr),
        ...);
    loader->Load();
    container[key].loader = std::move(loader);
  } else {
    // container[key].callbacks.push_back(std::move(callback));
  }
}

void GotHashHelper(result, unique_ptr<CVHashes> hashes) {
  // WARNING: Consider extension unload.
  // Utilize callbacks in container[key], hashes.
  container.erase(key);
}

ContentHashLoader::ContentHashLoader(done_callback)
    : weak_factory_(this), done_callback_(std::move(done_callback)) {
  // sequence_checker AND/OR GetCurrentThreadIdentifier().
}
ContentHashLoader::Load() {
  DCHECK_CURRENTLY_ON(IO);
  base::PostTaskWithTraitsAndReply(
      FROM_HERE, {base;:MayBlock(), USER_VISIBLE},
      base::Bind(&ReadContentHash, extension_root_),
      base::BindOnce(&ContentHashLoader::Loaded, weak_factory_.GetWeakPtr()));
}
void ContentHashLoader::ReadContentHash() {
  // Assert IO allowed.
  // READ (no create) verified contents and computed hashes, return false.
  enum Result {
    NO_VERIFIED_CONTENTS,
    INVALID_VERIFIED_CONTENTS,
    NO_COMPUTED_HASHES,
    INVALID_COMPUTED_HASHES,
    OK,
  };
  return result, etc.;
}
void ContentHashLoader::Loaded(result) {
  if (result == NO_VERIFIED_CONTENTS || result == INVALID_VERIFIED_CONTENTS) {
    Fetch(VERIFIED_CONTENTS);
  } else if (result == NO_COMPUTED_HASHES || result == INVALID_COMPUTED_HASHES) {
    Compute(result.verified_contents, COMPUTED_HASHES);
  } else if (result == OK) {
    std::move(done_callback_).Run(result);
  }
}
void Fetch() {
  // How to manage URLFetcherDelegate lifetime?
}
void ContentHashLoader::Fetch() {
  // TODO: Do we need this post task?
  base::PostTaskAndReply(
      FROM_HERE,
      content::BrowserThread::UI,
      base::Bind(&Fetch),
      base::BindOnce(&ContentHashLoader::FetchDone, weak_ptr_.GetWeakPtr()));
  url_fetcher_ = URLFetcher::Create(.., this);
  // Will call callback_.
}
void ContentHashLoader::OnURLFetchComplete(..) {
  std::unique_ptr<URLFetcher> url_fetcher = std::move(url_fetcher_);
  // We are in the same sequence as ContentHashLoader::Fetch().

  // Got verified_contents.json's contents.
  // valid_verified_contents = JSONReader::Read().
  if (!valid_verified_contents) {
    callback_.Run(FAILED_HASHES);
    return;
  } else {
    auto result = std::make_unique<Result>(verified_contents, null);
    base::PostTaskWithTraitsAndReplyWithResult(
        FROM_HERE,
        {MayBlock()},
        base::Bind(&WriteFile, ..., base::Unretained(result.get())),
        base::BindOnce(&OnWriteFileComplete, this, base::Passed(&result)));
  }
  // |url_fetcher_| destroyed.
}
void ContentHashLoader::OnWriteFileComplete(bool success, unique_ptr<Result> result) {
  // sequence_checker.
  if (!success) {
    // DROP VerifiedContents?
    std::move(callback_).Run();
    return;
  }
  // MaybeCreateHashes() -> CreateHashes().
  bool created = MaybeCreateHashes(result.get());
  if (!created) {
    std::move(callback_).Run(ERROR);
    return;
  }
  std::move(callback_).Run(std::move(result));
}
bool ContentHashFetcher::CreateHashes(Result* result) {
  ...;
  return true/false;
}

// OK
// q: file_missing_from_verified_contents_
// q: hash_mismatch_unix_paths

*/

void ContentVerifier::Start() {
  ExtensionRegistry* registry = ExtensionRegistry::Get(context_);
  observer_.Add(registry);
}

void ContentVerifier::Shutdown() {
  shutdown_ = true;
  delegate_->Shutdown();
  content::BrowserThread::PostTask(
      content::BrowserThread::IO,
      FROM_HERE,
      base::Bind(&ContentVerifierIOData::Clear, io_data_));
  observer_.RemoveAll();
  fetcher_.reset();
}

ContentVerifyJob* ContentVerifier::CreateJobFor(
    const std::string& extension_id,
    const base::FilePath& extension_root,
    const base::FilePath& relative_path) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::IO);

  const ContentVerifierIOData::ExtensionData* data =
      io_data_->GetData(extension_id);
  if (!data)
    return NULL;

  base::FilePath normalized_unix_path = NormalizeRelativePath(relative_path);

  std::set<base::FilePath> unix_paths;
  unix_paths.insert(normalized_unix_path);
  if (!ShouldVerifyAnyPaths(extension_id, extension_root, unix_paths))
    return NULL;
  // TODO(asargent) - we can probably get some good performance wins by having
  // a cache of ContentHashReader's that we hold onto past the end of each job.
  return new ContentVerifyJob(
      new ContentHashReader(extension_id, data->version, extension_root,
                            normalized_unix_path, delegate_->GetPublicKey()),
      base::BindOnce(&ContentVerifier::VerifyFailed, this, extension_id));
}

void ContentVerifier::Fail() {
  printf("Fail\n");
  DCHECK_CURRENTLY_ON(content::BrowserThread::IO);
}

// ContentVerifyJob::failure_callback_.
void ContentVerifier::VerifyFailed(const std::string& extension_id,
                                   ContentVerifyJob::FailureReason reason) {
  if (!content::BrowserThread::CurrentlyOn(content::BrowserThread::UI)) {
    content::BrowserThread::PostTask(
        content::BrowserThread::UI,
        FROM_HERE,
        base::Bind(&ContentVerifier::VerifyFailed, this, extension_id, reason));
    return;
  }
  if (shutdown_)
    return;

  VLOG(1) << "VerifyFailed " << extension_id << " reason:" << reason;

  ExtensionRegistry* registry = ExtensionRegistry::Get(context_);
  const Extension* extension =
      registry->GetExtensionById(extension_id, ExtensionRegistry::EVERYTHING);

  if (!extension)
    return;

  if (reason == ContentVerifyJob::MISSING_ALL_HASHES) {
    // If we failed because there were no hashes yet for this extension, just
    // request some.
    fetcher_->DoFetch(extension, true /* force */);
  } else {
    delegate_->VerifyFailed(extension_id, reason);
  }
}

void ContentVerifier::OnExtensionLoaded(
    content::BrowserContext* browser_context,
    const Extension* extension) {
  if (shutdown_)
    return;

  ContentVerifierDelegate::Mode mode = delegate_->ShouldBeVerified(*extension);
  if (mode != ContentVerifierDelegate::NONE) {
    // The browser image paths from the extension may not be relative (eg
    // they might have leading '/' or './'), so we strip those to make
    // comparing to actual relative paths work later on.
    std::set<base::FilePath> original_image_paths =
        delegate_->GetBrowserImagePaths(extension);

    std::unique_ptr<std::set<base::FilePath>> image_paths(
        new std::set<base::FilePath>);
    for (const auto& path : original_image_paths) {
      image_paths->insert(NormalizeRelativePath(path));
    }

    std::unique_ptr<ContentVerifierIOData::ExtensionData> data(
        new ContentVerifierIOData::ExtensionData(
            std::move(image_paths),
            extension->version() ? *extension->version() : base::Version()));
    content::BrowserThread::PostTask(content::BrowserThread::IO,
                                     FROM_HERE,
                                     base::Bind(&ContentVerifierIOData::AddData,
                                                io_data_,
                                                extension->id(),
                                                base::Passed(&data)));
    fetcher_->ExtensionLoaded(extension);
  }
}

void ContentVerifier::OnExtensionUnloaded(
    content::BrowserContext* browser_context,
    const Extension* extension,
    UnloadedExtensionReason reason) {
  if (shutdown_)
    return;
  content::BrowserThread::PostTask(
      content::BrowserThread::IO,
      FROM_HERE,
      base::Bind(
          &ContentVerifierIOData::RemoveData, io_data_, extension->id()));
  if (fetcher_)
    fetcher_->ExtensionUnloaded(extension);
}

void ContentVerifier::OnFetchCompleteHelper(
    const std::string& extension_id,
    bool should_verify_any_paths_result) {
  printf("OnFetchCompleteHelper, extension_id: %s, should_verify_any_paths_result: %d\n",
         extension_id.c_str(), should_verify_any_paths_result);
  if (should_verify_any_paths_result)
    delegate_->VerifyFailed(extension_id, ContentVerifyJob::MISSING_ALL_HASHES);
}

void ContentVerifier::OnFetchComplete(
    const std::string& extension_id,
    bool success,
    bool was_force_check,
    const std::set<base::FilePath>& hash_mismatch_unix_paths) {
  if (g_test_observer)
    g_test_observer->OnFetchComplete(extension_id, success);

  if (shutdown_)
    return;

  VLOG(1) << "OnFetchComplete " << extension_id << " success:" << success;

  ExtensionRegistry* registry = ExtensionRegistry::Get(context_);
  const Extension* extension =
      registry->GetExtensionById(extension_id, ExtensionRegistry::EVERYTHING);
  if (!delegate_ || !extension)
    return;

  ContentVerifierDelegate::Mode mode = delegate_->ShouldBeVerified(*extension);
  if (was_force_check && !success &&
      mode == ContentVerifierDelegate::ENFORCE_STRICT) {
    // We weren't able to get verified_contents.json or weren't able to compute
    // hashes.
    delegate_->VerifyFailed(extension_id, ContentVerifyJob::MISSING_ALL_HASHES);
  } else {
    printf("OnFetchComplete, ext = %s, success = %d, num hashes mismatch: %d\n",
           extension_id.c_str(), success, (int)hash_mismatch_unix_paths.size());
    content::BrowserThread::PostTaskAndReplyWithResult(
        content::BrowserThread::IO, FROM_HERE,
        base::Bind(&ContentVerifier::ShouldVerifyAnyPaths, this, extension_id,
                   extension->path(), hash_mismatch_unix_paths),
        base::Bind(&ContentVerifier::OnFetchCompleteHelper, this,
                   extension_id));
  }
}

bool ContentVerifier::ShouldVerifyAnyPaths(
    const std::string& extension_id,
    const base::FilePath& extension_root,
    const std::set<base::FilePath>& relative_unix_paths) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::IO);
  const ContentVerifierIOData::ExtensionData* data =
      io_data_->GetData(extension_id);
  if (!data)
    return false;

  const std::set<base::FilePath>& browser_images = *(data->browser_image_paths);

  base::FilePath locales_dir = extension_root.Append(kLocaleFolder);
  std::unique_ptr<std::set<std::string>> all_locales;

  const base::FilePath manifest_file(kManifestFilename);
  const base::FilePath messages_file(kMessagesFilename);
  for (const base::FilePath& relative_unix_path : relative_unix_paths) {
    if (relative_unix_path == manifest_file)
      continue;

    if (base::ContainsKey(browser_images, relative_unix_path))
      continue;

    base::FilePath full_path =
        extension_root.Append(relative_unix_path.NormalizePathSeparators());

    if (full_path == file_util::GetIndexedRulesetPath(extension_root))
      continue;

    if (locales_dir.IsParent(full_path)) {
      if (!all_locales) {
        // TODO(asargent) - see if we can cache this list longer to avoid
        // having to fetch it more than once for a given run of the
        // browser. Maybe it can never change at runtime? (Or if it can, maybe
        // there is an event we can listen for to know to drop our cache).
        all_locales.reset(new std::set<std::string>);
        extension_l10n_util::GetAllLocales(all_locales.get());
      }

      // Since message catalogs get transcoded during installation, we want
      // to skip those paths. See if this path looks like
      // _locales/<some locale>/messages.json - if so then skip it.
      if (full_path.BaseName() == messages_file &&
          full_path.DirName().DirName() == locales_dir &&
          base::ContainsKey(*all_locales,
                            full_path.DirName().BaseName().MaybeAsASCII())) {
        continue;
      }
    }
    return true;
  }
  return false;
}

VerifierData* ContentVerifier::CreateVerifierDataFor(
    const std::string& extension_id,
    const base::FilePath& extension_root,
    const base::FilePath& relative_path) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::IO);

  const ContentVerifierIOData::ExtensionData* data =
      io_data_->GetData(extension_id);
  if (!data)
    return nullptr;

  base::FilePath normalized_unix_path = NormalizeRelativePath(relative_path);

  std::set<base::FilePath> unix_paths;
  unix_paths.insert(normalized_unix_path);
  if (!ShouldVerifyAnyPaths(extension_id, extension_root, unix_paths))
    return nullptr;

  return new VerifierData(extension_id,
                          data->version,
                          extension_root,
                          normalized_unix_path,
                          delegate_->GetPublicKey(),
                          this);
}

ContentVerifier::ContentHashLoaderData::ContentHashLoaderData() {}
ContentVerifier::ContentHashLoaderData::~ContentHashLoaderData() {}

void ContentVerifier::GetContentVerificationHash(
    const std::string& extension_id,
    const base::Version& extension_version,
    const base::FilePath& extension_root,
    const base::FilePath& relative_path,
    const ContentVerifierKey& key,
    ContentHashLoaderCallback callback) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::IO);

  printf("CV::GetContentVerificationHash, extension_id = %s\n",
         extension_id.c_str());

  ContentHashLoaderData& loader_data = loaders_[extension_id];
  printf("Callbacks: %d\n",
         (int)loader_data.callbacks.size());
  //const bool should_start = loader_data.callbacks.empty();
  const bool should_start = !loader_data.loader;
  if (!loader_data.loader) {
    loader_data.loader = std::make_unique<ContentHashLoader>(
        delegate_.get(),
        request_context_,
        extension_id,
        extension_version,
        extension_root,
        relative_path,
        key
        );
  }
  loader_data.callbacks.push_back(std::move(callback));
  if (should_start) {
    loader_data.loader->Load(
        base::BindOnce(&ContentVerifier::GotHashHelper, this,
                       extension_id));
  } else {
    printf("Skip\n");
  }
}
void ContentVerifier::GotHashHelper(
    const std::string& extension_id,
    ContentHashLoader::LoadResult result,
    scoped_refptr<ContentHash> hash) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::IO);
  // WARNING: Consider extension unload.
  // Utilize callbacks in container[key], hashes.
  printf("**** Test: ContentVerifier::GotHashHelper: extension_id: %s ****\n",
         extension_id.c_str());
  DCHECK_GT(loaders_.count(extension_id), 0u);
  ContentHashLoaderData& loader_data = loaders_[extension_id];
  printf("Num callbacks found: %d\n", (int)loader_data.callbacks.size());
  // TODO: Short circuit failure here, global from ContentVerifier?
  for (auto& callback : loader_data.callbacks) {
    std::move(callback).Run(result, hash);
  }
  loaders_.erase(extension_id);
}

}  // namespace extensions
