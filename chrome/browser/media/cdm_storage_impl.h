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
#include "base/threading/thread_checker.h"
#include "content/public/browser/web_contents_observer.h"
#include "media/mojo/interfaces/cdm_storage.mojom.h"
#include "mojo/public/cpp/bindings/binding.h"
#include "url/origin.h"

namespace storage {
class FileSystemContext;
}

namespace content {
class RenderFrameHost;
}  // namespace content

// This class implements the media::mojom::CdmStorage using the
// PluginPrivateFileSystem for backwards compatibility with CDMs running
// as a pepper plugin.
class CdmStorageImpl : public media::mojom::CdmStorage,
                       public content::WebContentsObserver {
 public:
  // Create a CdmStorageImpl object and bind it to |request|.
  static void Create(content::RenderFrameHost* render_frame_host,
                     media::mojom::CdmStorageRequest request);

  // media::mojom::CdmStorage implementation.
  void Initialize(const std::string& key_system) final;
  void Open(const std::string& file_name, OpenCallback callback) final;

  // content::WebContentsObserver implementation.
  void RenderFrameDeleted(content::RenderFrameHost* render_frame_host) final;
  void DidFinishNavigation(content::NavigationHandle* navigation_handle) final;

 private:
  friend class CdmStorageTest;

  CdmStorageImpl(int child_process_id,
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
  void OnFileOpened(const std::string& file_name,
                    OpenCallback callback,
                    base::File file,
                    const base::Closure& on_close_callback);

  // Called when the connection to the client is closed. This causes |this|
  // to be destroyed as it is no longer connected.
  void OnConnectionClosed();

  int child_process_id_;
  url::Origin origin_;
  mojo::Binding<media::mojom::CdmStorage> binding_;

  std::string plugin_id_;
  scoped_refptr<storage::FileSystemContext> filesystem_context_;

  THREAD_CHECKER(thread_checker_);

  base::WeakPtrFactory<CdmStorageImpl> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(CdmStorageImpl);
};

#endif  // CHROME_BROWSER_MEDIA_CDM_STORAGE_IMPL_H_
