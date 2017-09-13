// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_MEDIA_CDM_STORAGE_IMPL_H_
#define CHROME_BROWSER_MEDIA_CDM_STORAGE_IMPL_H_

#include <string>

#include "base/callback_forward.h"
#include "base/files/file.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/memory/weak_ptr.h"
#include "media/mojo/interfaces/cdm_storage.mojom.h"
#include "mojo/public/cpp/bindings/binding.h"
#include "url/origin.h"

namespace storage {
class FileSystemContext;
}

namespace content {
class RenderFrameHost;
class RenderProcessHost;
}  // namespace content

class CdmStorageImpl : public media::mojom::CdmStorage {
 public:
  // Create a CdmStorageImpl object and bind it to |request|. Create() can
  // be called on any thread, but the created CdmStorageImpl will only run
  // on the FILE thread.
  static void Create(content::RenderFrameHost* render_frame_host,
                     media::mojom::CdmStorageRequest request);

  // media::mojom::CdmStorage implementation.
  void Initialize(const std::string& cdm_type,
                  InitializeCallback callback) final;
  void Open(const std::string& name, OpenCallback callback) final;
  void Close(const std::string& name, CloseCallback callback) final;

 private:
  friend class CdmStorageTest;

  CdmStorageImpl(content::RenderProcessHost* render_process_host,
                 const url::Origin& origin,
                 scoped_refptr<storage::FileSystemContext> file_system_context,
                 media::mojom::CdmStorageRequest request);
  ~CdmStorageImpl() final;

  // Called when the file system has been opened (OpenPluginPrivateFileSystem
  // is asynchronous).
  void OnFileSystemOpened(const std::string& file_name,
                          const std::string& fsid,
                          OpenCallback callback,
                          base::File::Error error);

  // Called when the requested file has been opened.
  void OnFileOpened(const std::string& name,
                    OpenCallback callback,
                    base::File file,
                    const base::Closure& on_close_callback);

  // Called when the connection to the client is closed.
  void OnConnectionClosed();

  content::RenderProcessHost* render_process_host_;
  url::Origin origin_;
  mojo::Binding<media::mojom::CdmStorage> binding_;

  std::string plugin_id_;
  scoped_refptr<storage::FileSystemContext> filesystem_context_;

  base::WeakPtrFactory<CdmStorageImpl> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(CdmStorageImpl);
};

#endif  // CHROME_BROWSER_MEDIA_CDM_STORAGE_IMPL_H_
