// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/offline_pages/prefetch/prefetch_importer_impl.h"

#include "base/bind.h"
#include "base/logging.h"
#include "chrome/browser/offline_pages/offline_page_model_factory.h"
#include "components/offline_pages/core/offline_page_model.h"
#include "url/gurl.h"

namespace offline_pages {

PrefetchImporterImpl::PrefetchImporterImpl(content::BrowserContext* context)
    : context_(context), weak_ptr_factory_(this) {}

PrefetchImporterImpl::~PrefetchImporterImpl() = default;

void PrefetchImporterImpl::ImportDownload(const GURL& url,
                                          const GURL& original_url,
                                          const base::string16& title,
                                          const ClientId& client_id,
                                          const base::FilePath& file_path,
                                          int64_t file_size,
                                          const CompletedCallback& callback) {
  OfflinePageModel* offline_page_model =
      OfflinePageModelFactory::GetForBrowserContext(context_);
  DCHECK(offline_page_model);

  OfflinePageModel::ImportPageParams import_page_params;
  import_page_params.url = url;
  import_page_params.original_url = original_url;
  import_page_params.client_id = client_id;
  import_page_params.title = title;
  import_page_params.file_path = file_path;
  import_page_params.file_size = file_size;
  offline_page_model->ImportPage(
      import_page_params, base::Bind(&PrefetchImporterImpl::OnDownloadImported,
                                     weak_ptr_factory_.GetWeakPtr(), callback));
}

void PrefetchImporterImpl::OnDownloadImported(const CompletedCallback& callback,
                                              SavePageResult result,
                                              int64_t offline_id) {
  callback.Run(result == SavePageResult::SUCCESS, offline_id);
}

}  // namespace offline_pages
