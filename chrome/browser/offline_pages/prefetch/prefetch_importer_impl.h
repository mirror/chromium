// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_OFFLINE_PAGES_PREFETCH_PREFETCH_IMPORTER_IMPL_H_
#define CHROME_BROWSER_OFFLINE_PAGES_PREFETCH_PREFETCH_IMPORTER_IMPL_H_

#include <memory>

#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "components/offline_pages/core/prefetch/prefetch_importer.h"

namespace content {
class BrowserContext;
}

namespace offline_pages {

enum class SavePageResult;

class PrefetchImporterImpl : public PrefetchImporter {
 public:
  explicit PrefetchImporterImpl(content::BrowserContext* context);
  ~PrefetchImporterImpl() override;

  // PrefetchImporter implementation.
  void ImportDownload(const GURL& url,
                      const GURL& original_url,
                      const base::string16& title,
                      const ClientId& client_id,
                      const base::FilePath& file_path,
                      int64_t file_size,
                      const CompletedCallback& callback) override;

 private:
  void OnDownloadImported(const CompletedCallback& callback,
                          SavePageResult result,
                          int64_t offline_id);

  content::BrowserContext* context_;
  base::WeakPtrFactory<PrefetchImporterImpl> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(PrefetchImporterImpl);
};

}  // namespace offline_pages

#endif  // CHROME_BROWSER_OFFLINE_PAGES_PREFETCH_PREFETCH_IMPORTER_IMPL_H_
