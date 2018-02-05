// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/file_select_helper.h"

#include <Cocoa/Cocoa.h>
#include <sys/stat.h>

#include <vector>

#include "base/bind.h"
#include "base/callback.h"
#include "base/files/file.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/mac/foundation_util.h"
#include "base/memory/scoped_ptr.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_contents_observer.h"
#include "third_party/zlib/google/zip.h"
#include "ui/shell_dialogs/selected_file_info.h"

namespace {

void DeleteFiles(scoped_ptr<std::vector<base::FilePath>> paths) {
  for (auto& file_path : *paths)
    base::DeleteFile(file_path, false);
}

class TemporaryFileTracker : public content::WebContentsObserver {
 public:
  TemporaryFileTracker(content::WebContents* web_contents,
                       content::RenderViewHost* render_view_host,
                       scoped_ptr<std::vector<base::FilePath>> temporary_files);
  virtual ~TemporaryFileTracker();

 private:
  // content::WebContentsObserver
  void RenderViewDeleted(content::RenderViewHost* render_view_host) override;
  void RenderViewHostChanged(content::RenderViewHost* old_host,
                             content::RenderViewHost* new_host) override;
  void WebContentsDestroyed() override;

  content::RenderViewHost* render_view_host_;
  scoped_ptr<std::vector<base::FilePath>> temporary_files_;
};

TemporaryFileTracker::TemporaryFileTracker(
    content::WebContents* web_contents,
    content::RenderViewHost* render_view_host,
    scoped_ptr<std::vector<base::FilePath>> temporary_files)
    : WebContentsObserver(web_contents),
      render_view_host_(render_view_host),
      temporary_files_(std::move(temporary_files)) {
  if (!web_contents || !render_view_host)
    delete this;
}

TemporaryFileTracker::~TemporaryFileTracker() {
  content::BrowserThread::PostBlockingPoolTask(
      FROM_HERE, base::Bind(&DeleteFiles, base::Passed(&temporary_files_)));
}

void TemporaryFileTracker::RenderViewDeleted(
    content::RenderViewHost* render_view_host) {
  DCHECK_EQ(render_view_host, render_view_host_);
  delete this;
}

void TemporaryFileTracker::RenderViewHostChanged(
    content::RenderViewHost* old_host,
    content::RenderViewHost* new_host) {
  DCHECK_EQ(old_host, render_view_host_);
  delete this;
}

void TemporaryFileTracker::WebContentsDestroyed() {
  delete this;
}

// Given the |path| of a package, returns the destination that the package
// should be zipped to. Returns an empty path on any errors.
base::FilePath ZipDestination(const base::FilePath& path) {
  base::FilePath dest;

  if (!base::GetTempDir(&dest)) {
    // Couldn't get the temporary directory.
    return base::FilePath();
  }

  // TMPDIR/<bundleID>/zip_cache/<guid>

  NSString* bundleID = [[NSBundle mainBundle] bundleIdentifier];
  dest = dest.Append([bundleID fileSystemRepresentation]);

  dest = dest.Append("zip_cache");

  NSString* guid = [[NSProcessInfo processInfo] globallyUniqueString];
  dest = dest.Append([guid fileSystemRepresentation]);

  return dest;
}

// Returns the path of the package and its components relative to the package's
// parent directory.
std::vector<base::FilePath> RelativePathsForPackage(
    const base::FilePath& package) {
  // Get the base directory.
  base::FilePath base_dir = package.DirName();

  // Add the package as the first relative path.
  std::vector<base::FilePath> relative_paths;
  relative_paths.push_back(package.BaseName());

  // Add the components of the package as relative paths.
  base::FileEnumerator file_enumerator(
      package,
      true /* recursive */,
      base::FileEnumerator::FILES | base::FileEnumerator::DIRECTORIES);
  for (base::FilePath path = file_enumerator.Next(); !path.empty();
       path = file_enumerator.Next()) {
    base::FilePath relative_path;
    bool success = base_dir.AppendRelativePath(path, &relative_path);
    if (success)
      relative_paths.push_back(relative_path);
  }

  return relative_paths;
}

}  // namespace

base::FilePath FileSelectHelper::ZipPackage(const base::FilePath& path) {
  base::FilePath dest(ZipDestination(path));
  if (dest.empty())
    return dest;

  if (!base::CreateDirectory(dest.DirName()))
    return base::FilePath();

  base::File file(dest, base::File::FLAG_CREATE | base::File::FLAG_WRITE);
  if (!file.IsValid())
    return base::FilePath();

  std::vector<base::FilePath> files_to_zip(RelativePathsForPackage(path));
  base::FilePath base_dir = path.DirName();
  bool success = zip::ZipFiles(base_dir, files_to_zip, file.GetPlatformFile());

  int result = -1;
  if (success)
    result = fchmod(file.GetPlatformFile(), S_IRUSR);

  return result >= 0 ? dest : base::FilePath();
}

void FileSelectHelper::ProcessSelectedFilesMac(
    const std::vector<ui::SelectedFileInfo>& files) {
  // Make a mutable copy of the input files.
  std::vector<ui::SelectedFileInfo> files_out(files);
  std::vector<base::FilePath> temporary_files;

  for (auto& file_info : files_out) {
    NSString* filename = base::mac::FilePathToNSString(file_info.local_path);
    BOOL isPackage =
        [[NSWorkspace sharedWorkspace] isFilePackageAtPath:filename];
    if (isPackage && base::DirectoryExists(file_info.local_path)) {
      base::FilePath result = ZipPackage(file_info.local_path);

      if (!result.empty()) {
        temporary_files.push_back(result);
        file_info.local_path = result;
        file_info.file_path = result;
        file_info.display_name.append(".zip");
      }
    }
  }

  content::BrowserThread::PostTask(
      content::BrowserThread::UI,
      FROM_HERE,
      base::Bind(&FileSelectHelper::ProcessSelectedFilesMacOnUIThread,
                 base::Unretained(this),
                 files_out,
                 temporary_files));
}

void FileSelectHelper::ProcessSelectedFilesMacOnUIThread(
    scoped_ptr<std::vector<ui::SelectedFileInfo>> files,
    scoped_ptr<std::vector<base::FilePath>> temporary_files) {
  DCHECK_CURRENTLY_ON(content::BrowserThread::UI);

  if (!temporary_files->empty())
    new TemporaryFileTracker(web_contents_, render_view_host_,
                             std::move(temporary_files));
  NotifyRenderViewHostAndEnd(*files);
}
