// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "webkit/fileapi/isolated_context.h"

#include "base/basictypes.h"
#include "base/logging.h"
#include "base/rand_util.h"
#include "base/string_number_conversions.h"
#include "base/string_util.h"

namespace fileapi {

static base::LazyInstance<IsolatedContext>::Leaky g_isolated_context =
    LAZY_INSTANCE_INITIALIZER;

// static
IsolatedContext* IsolatedContext::GetInstance() {
  return g_isolated_context.Pointer();
}

std::string IsolatedContext::RegisterIsolatedFileSystem(
    const std::set<FilePath>& files) {
  base::AutoLock locker(lock_);
  std::string filesystem_id = GetNewFileSystemId();
  // Stores basename to fullpath map, as we store the basenames as
  // the filesystem's toplevel entries.
  PathMap toplevels;
  for (std::set<FilePath>::const_iterator iter = files.begin();
       iter != files.end(); ++iter) {
    // The given path should not contain any '..' and should be absolute.
    if (iter->ReferencesParent() || !iter->IsAbsolute())
      continue;

    // Register the basename -> fullpath map. (We only expose the basename
    // part to the user scripts)
    FilePath fullpath = iter->NormalizePathSeparators();
    FilePath basename = iter->BaseName();
    // TODO(kinuko): Append a suffix or something if we have multiple pathnames
    // with the same basename. For now we only register the first one.
    toplevels.insert(std::make_pair(basename, fullpath));
  }

  // TODO(kinuko): we may not want to register the file system if there're
  // no valid paths in the given file set.

  toplevel_map_[filesystem_id] = toplevels;

  // Each file system is created with refcount == 0.
  ref_counts_[filesystem_id] = 0;

  return filesystem_id;
}

void IsolatedContext::RevokeIsolatedFileSystem(
    const std::string& filesystem_id) {
  base::AutoLock locker(lock_);
  RevokeWithoutLocking(filesystem_id);
}

void IsolatedContext::AddReference(const std::string& filesystem_id) {
  base::AutoLock locker(lock_);
  DCHECK(ref_counts_.find(filesystem_id) != ref_counts_.end());
  ref_counts_[filesystem_id]++;
}

void IsolatedContext::RemoveReference(const std::string& filesystem_id) {
  base::AutoLock locker(lock_);
  // This could get called for non-existent filesystem if it has been
  // already deleted by RevokeIsolatedFileSystem.
  if (ref_counts_.find(filesystem_id) == ref_counts_.end())
    return;
  DCHECK(ref_counts_[filesystem_id] > 0);
  if (--ref_counts_[filesystem_id] == 0)
    RevokeWithoutLocking(filesystem_id);
}

bool IsolatedContext::CrackIsolatedPath(const FilePath& virtual_path,
                                        std::string* filesystem_id,
                                        FilePath* root_path,
                                        FilePath* platform_path) const {
  DCHECK(filesystem_id);
  DCHECK(platform_path);

  // This should not contain any '..' references.
  if (virtual_path.ReferencesParent())
    return false;

  // The virtual_path should comprise <filesystem_id> and <relative_path> parts.
  std::vector<FilePath::StringType> components;
  virtual_path.GetComponents(&components);
  if (components.size() < 1)
    return false;

  base::AutoLock locker(lock_);
  std::string fsid = FilePath(components[0]).MaybeAsASCII();
  if (fsid.empty())
    return false;
  IDToPathMap::const_iterator found_toplevels = toplevel_map_.find(fsid);
  if (found_toplevels == toplevel_map_.end())
    return false;
  *filesystem_id = fsid;
  if (components.size() == 1) {
    platform_path->clear();
    return true;
  }
  // components[1] should be a toplevel path of the dropped paths.
  PathMap::const_iterator found = found_toplevels->second.find(
      FilePath(components[1]));
  if (found == found_toplevels->second.end())
    return false;
  FilePath path = found->second;
  if (root_path)
    *root_path = path;
  for (size_t i = 2; i < components.size(); ++i) {
    path = path.Append(components[i]);
  }
  *platform_path = path;
  return true;
}

bool IsolatedContext::GetTopLevelPaths(const std::string& filesystem_id,
                                       std::vector<FilePath>* paths) const {
  DCHECK(paths);
  base::AutoLock locker(lock_);
  IDToPathMap::const_iterator found = toplevel_map_.find(filesystem_id);
  if (found == toplevel_map_.end())
    return false;
  paths->clear();
  PathMap toplevels = found->second;
  for (PathMap::const_iterator iter = toplevels.begin();
       iter != toplevels.end(); ++iter) {
    // Each path map entry holds a map of a toplevel name to its full path.
    paths->push_back(iter->second);
  }
  return true;
}

FilePath IsolatedContext::CreateVirtualPath(
    const std::string& filesystem_id, const FilePath& relative_path) const {
  FilePath full_path;
  full_path = full_path.AppendASCII(filesystem_id);
  if (relative_path.value() != FILE_PATH_LITERAL("/"))
    full_path = full_path.Append(relative_path);
  return full_path;
}

IsolatedContext::IsolatedContext() {
}

IsolatedContext::~IsolatedContext() {
}

void IsolatedContext::RevokeWithoutLocking(
    const std::string& filesystem_id) {
  toplevel_map_.erase(filesystem_id);
  ref_counts_.erase(filesystem_id);
}

std::string IsolatedContext::GetNewFileSystemId() const {
  // Returns an arbitrary random string which must be unique in the map.
  uint32 random_data[4];
  std::string id;
  do {
    base::RandBytes(random_data, sizeof(random_data));
    id = base::HexEncode(random_data, sizeof(random_data));
  } while (toplevel_map_.find(id) != toplevel_map_.end());
  return id;
}

}  // namespace fileapi
