// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_FILE_PROTOCOL_HANDLER_NETWORK_SERVICE_H_
#define CONTENT_BROWSER_FILE_PROTOCOL_HANDLER_NETWORK_SERVICE_H_

#include "base/files/file_path.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/sequenced_task_runner.h"
#include "content/common/content_export.h"
#include "content/public/browser/protocol_handler.h"

namespace content {

// A URL handler for the file:// scheme used when Network Service is enabled.
class CONTENT_EXPORT FileProtocolHandlerNetworkService
    : public ProtocolHandler {
 public:
  // SequencedTaskRunner must be allowed to block and should have background
  // priority since it will be used to schedule synchronous file I/O tasks.
  FileProtocolHandlerNetworkService(
      const base::FilePath& profile_path,
      scoped_refptr<base::SequencedTaskRunner> task_runner);
  ~FileProtocolHandlerNetworkService() override;

  // ProtocolHandler:
  void CreateAndStartLoader(const ResourceRequest& request,
                            mojom::URLLoaderRequest loader,
                            mojom::URLLoaderClientPtr client) override;

 private:
  const base::FilePath profile_path_;
  const scoped_refptr<base::SequencedTaskRunner> task_runner_;

  DISALLOW_COPY_AND_ASSIGN(FileProtocolHandlerNetworkService);
};

}  // namespace content

#endif  // CONTENT_BROWSER_FILE_PROTOCOL_HANDLER_NETWORK_SERVICE_H_
