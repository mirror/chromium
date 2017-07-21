// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/arc/fileapi/arc_file_system_host_impl.h"

#include "base/logging.h"
#include "content/public/browser/browser_thread.h"

using content::BrowserThread;

namespace arc {

ArcFileSystemHostImpl::ArcFileSystemHostImpl(Delegate* delegate)
    : delegate_(delegate) {}

ArcFileSystemHostImpl::~ArcFileSystemHostImpl() = default;

void ArcFileSystemHostImpl::GetFileName(const std::string& url,
                                        const GetFileNameCallback& callback) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  NOTIMPLEMENTED();
}

void ArcFileSystemHostImpl::GetFileSize(const std::string& url,
                                        const GetFileSizeCallback& callback) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  NOTIMPLEMENTED();
}

void ArcFileSystemHostImpl::GetFileType(const std::string& url,
                                        const GetFileTypeCallback& callback) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  NOTIMPLEMENTED();
}

void ArcFileSystemHostImpl::OnDocumentChanged(int64_t watcher_id,
                                              ChangeType type) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  delegate_->OnDocumentChanged(watcher_id, type);
}

void ArcFileSystemHostImpl::OpenFileToRead(
    const std::string& url,
    const OpenFileToReadCallback& callback) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
  NOTIMPLEMENTED();
}

}  // namespace arc
