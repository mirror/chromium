// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_RENDERER_HOST_WEB_DATABASE_HOST_IMPL_H_
#define CONTENT_BROWSER_RENDERER_HOST_WEB_DATABASE_HOST_IMPL_H_

#include "content/common/web_database.mojom.h"
#include "storage/browser/database/database_tracker.h"

namespace content {

class WebDatabaseHostImpl : public content::mojom::WebDatabaseHost {
 public:
  explicit WebDatabaseHostImpl(
      scoped_refptr<storage::DatabaseTracker> db_tracker);
  ~WebDatabaseHostImpl() override;

  static void Create(scoped_refptr<storage::DatabaseTracker> db_tracker,
                     content::mojom::WebDatabaseHostRequest request);

 private:
  // WebDatabaseHost methods.
  void OpenFile(const base::string16& vfs_file_name,
                int32_t desired_flags,
                OpenFileCallback callback) override;

  // The database tracker for the current browser context.
  scoped_refptr<storage::DatabaseTracker> db_tracker_;
};

}  // namespace content

#endif  // CONTENT_BROWSER_RENDERER_HOST_WEB_DATABASE_HOST_IMPL_H_
