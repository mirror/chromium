// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_FILE_URL_LOADER_FACTORY_H_
#define CONTENT_BROWSER_FILE_URL_LOADER_FACTORY_H_

#include "base/files/file_path.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/sequenced_task_runner.h"
#include "content/common/content_export.h"
#include "content/public/common/simple_url_loader_factory.h"

namespace content {

// A URLLoaderFactory used for the file:// scheme used when Network Service is
// enabled.
class CONTENT_EXPORT FileURLLoaderFactory : public SimpleURLLoaderFactory {
 public:
  // SequencedTaskRunner must be allowed to block and should have background
  // priority since it will be used to schedule synchronous file I/O tasks.
  FileURLLoaderFactory(const base::FilePath& profile_path,
                       scoped_refptr<base::SequencedTaskRunner> task_runner);
  ~FileURLLoaderFactory() override;

 private:
  // SimpleURLLoaderFactory:
  void CreateLoaderAndStart(const ResourceRequest& request,
                            mojom::URLLoaderRequest loader,
                            mojom::URLLoaderClientPtr client) override;

  const base::FilePath profile_path_;
  const scoped_refptr<base::SequencedTaskRunner> task_runner_;

  DISALLOW_COPY_AND_ASSIGN(FileURLLoaderFactory);
};

}  // namespace content

#endif  // CONTENT_BROWSER_FILE_URL_LOADER_FACTORY_H_
