// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/download/save_file.h"

#include "base/logging.h"
#include "content/public/browser/browser_thread.h"
#include "net/base/file_stream.h"

using content::BrowserThread;

SaveFile::SaveFile(const SaveFileCreateInfo* info)
    : BaseFile(FilePath(), info->url, GURL(), 0, linked_ptr<net::FileStream>()),
      info_(info) {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::FILE));

  DCHECK(info);
  DCHECK(info->path.empty());
}

SaveFile::~SaveFile() {
  DCHECK(BrowserThread::CurrentlyOn(BrowserThread::FILE));
}
