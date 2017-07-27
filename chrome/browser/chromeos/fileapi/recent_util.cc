// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/fileapi/recent_util.h"

#include <string>
#include <utility>
#include <vector>

#include "chrome/browser/browser_process.h"
#include "chrome/browser/chromeos/profiles/profile_helper.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/profiles/profile_manager.h"
#include "chrome/common/chrome_constants.h"
#include "components/user_manager/user_manager.h"
#include "content/public/browser/browser_thread.h"
#include "net/base/escape.h"

using content::BrowserThread;

namespace chromeos {

namespace {

// Borrowed from chrome/browser/chromeos/drive/file_system_util.cc
// TODO(nya): Share the implementation.
Profile* ExtractProfile(const base::FilePath& path) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);

  const std::vector<Profile*>& profiles =
      g_browser_process->profile_manager()->GetLoadedProfiles();
  for (size_t i = 0; i < profiles.size(); ++i) {
    Profile* original_profile = profiles[i]->GetOriginalProfile();
    if (original_profile == profiles[i] &&
        !ProfileHelper::IsSigninProfile(original_profile) &&
        !ProfileHelper::IsLockScreenAppProfile(original_profile)) {
      const base::FilePath base = GetRecentMountPointPath(original_profile);
      if (base == path || base.IsParent(path))
        return original_profile;
    }
  }
  return nullptr;
}

bool ExtractRelativePath(const base::FilePath& path,
                         base::FilePath* relative_path) {
  std::vector<base::FilePath::StringType> components;
  path.GetComponents(&components);
  if (components.size() < 3)
    return false;
  if (components[0] != FILE_PATH_LITERAL("/"))
    return false;
  if (components[1] != FILE_PATH_LITERAL("special"))
    return false;
  static const base::FilePath::CharType kPrefix[] = FILE_PATH_LITERAL("recent");
  if (components[2].compare(0, arraysize(kPrefix) - 1, kPrefix) != 0)
    return false;

  relative_path->clear();
  for (size_t i = 3; i < components.size(); ++i)
    *relative_path = relative_path->Append(components[i]);
  return true;
}

// Borrowed from chrome/browser/chromeos/drive/file_system_util.cc
// TODO(nya): Share the implementation.
base::FilePath GetRecentMountPointPathForUserIdHash(
    const std::string& user_id_hash) {
  static const base::FilePath::CharType kSpecialMountPointRoot[] =
      FILE_PATH_LITERAL("/special");
  static const char kRecentMountPointNameBase[] = "recent";
  return base::FilePath(kSpecialMountPointRoot)
      .AppendASCII(net::EscapeQueryParamValue(
          kRecentMountPointNameBase +
              (user_id_hash.empty() ? "" : "-" + user_id_hash),
          false));
}

void ParseRecentUrlOnUIThread(
    scoped_refptr<storage::FileSystemContext> file_system_context,
    const storage::FileSystemURL& url,
    ParseRecentUrlCallback callback) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);

  Profile* profile;
  base::FilePath path;
  if (!url.is_valid() || url.type() != storage::kFileSystemTypeRecent ||
      !(profile = ExtractProfile(url.path())) ||
      !ExtractRelativePath(url.path(), &path)) {
    BrowserThread::PostTask(
        BrowserThread::IO, FROM_HERE,
        base::BindOnce(std::move(callback), RecentContext(), base::FilePath()));
    return;
  }

  BrowserThread::PostTask(
      BrowserThread::IO, FROM_HERE,
      base::BindOnce(
          std::move(callback),
          RecentContext(file_system_context.get(), url.origin(), profile),
          path));
}

}  // namespace

// Borrowed from chrome/browser/chromeos/drive/file_system_util.cc
// TODO(nya): Share the implementation.
base::FilePath GetRecentMountPointPath(Profile* profile) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);

  std::string id = ProfileHelper::GetUserIdHashFromProfile(profile);
  if (id.empty() || id == chrome::kLegacyProfileDir) {
    // ProfileHelper::GetUserIdHashFromProfile works only when multi-profile is
    // enabled. In that case, we fall back to use UserManager (it basically just
    // returns currently active users's hash in such a case.) I still try
    // ProfileHelper first because it works better in tests.
    const user_manager::User* const user =
        user_manager::UserManager::IsInitialized()
            ? ProfileHelper::Get()->GetUserByProfile(
                  profile->GetOriginalProfile())
            : nullptr;
    if (user)
      id = user->username_hash();
  }
  return GetRecentMountPointPathForUserIdHash(id);
}

// Borrowed from chrome/browser/chromeos/drive/file_system_util.cc
// TODO(nya): Share the implementation.
std::string GetRecentMountPointName(Profile* profile) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  return GetRecentMountPointPath(profile).BaseName().AsUTF8Unsafe();
}

void ParseRecentUrlOnIOThread(storage::FileSystemContext* file_system_context,
                              const storage::FileSystemURL& url,
                              ParseRecentUrlCallback callback) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  BrowserThread::PostTask(
      BrowserThread::UI, FROM_HERE,
      base::BindOnce(&ParseRecentUrlOnUIThread,
                     make_scoped_refptr(file_system_context), url,
                     base::Passed(std::move(callback))));
}

}  // namespace chromeos
