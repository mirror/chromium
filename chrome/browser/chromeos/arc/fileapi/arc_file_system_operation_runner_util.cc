// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/arc/fileapi/arc_file_system_operation_runner_util.h"

#include <utility>
#include <vector>

#include "components/arc/arc_service_manager.h"
#include "content/public/browser/browser_thread.h"
#include "url/gurl.h"

using content::BrowserThread;

namespace arc {

namespace file_system_operation_runner_util {

namespace {

// TODO(crbug.com/745648): Use correct BrowserContext.
ArcFileSystemOperationRunner* GetArcFileSystemOperationRunner() {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);

  auto* result = ArcFileSystemOperationRunner::GetForBrowserContext(
      ArcServiceManager::Get()->browser_context());
  DLOG_IF(ERROR, !result) << "ArcFileSystemOperationRunner unavailable. "
                          << "File system operations are dropped.";
  return result;
}

void GetFileSizeOnUIThread(const GURL& url,
                           base::OnceCallback<void(int64_t)> callback) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  auto* runner = GetArcFileSystemOperationRunner();
  if (!runner) {
    std::move(callback).Run(-1);
    return;
  }
  runner->GetFileSize(url,
                      base::AdaptCallbackForRepeating(std::move(callback)));
}

void OpenFileToReadOnUIThread(
    const GURL& url,
    base::OnceCallback<void(mojo::ScopedHandle)> callback) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  auto* runner = GetArcFileSystemOperationRunner();
  if (!runner) {
    std::move(callback).Run(mojo::ScopedHandle());
    return;
  }
  runner->OpenFileToRead(url,
                         base::AdaptCallbackForRepeating(std::move(callback)));
}

}  // namespace

void GetFileSizeOnIOThread(const GURL& url,
                           const GetFileSizeCallback& callback) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  BrowserThread::PostAsyncTaskAndReply(
      BrowserThread::UI, FROM_HERE, base::BindOnce(&GetFileSizeOnUIThread, url),
      base::OnceCallback<void(int64_t)>(callback));
}

void OpenFileToReadOnIOThread(const GURL& url,
                              const OpenFileToReadCallback& callback) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  BrowserThread::PostAsyncTaskAndReply(
      BrowserThread::UI, FROM_HERE,
      base::BindOnce(&OpenFileToReadOnUIThread, url),
      base::OnceCallback<void(mojo::ScopedHandle)>(callback));
}

}  // namespace file_system_operation_runner_util

}  // namespace arc
