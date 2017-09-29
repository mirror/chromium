// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_MEDIA_CDM_STORAGE_IMPL_H_
#define CHROME_BROWSER_MEDIA_CDM_STORAGE_IMPL_H_

#include <set>
#include <string>
#include <tuple>

#include "base/callback_forward.h"
#include "base/files/file.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/memory/weak_ptr.h"
#include "base/threading/thread_checker.h"
#include "content/common/content_export.h"
#include "media/mojo/interfaces/cdm_storage.mojom.h"
#include "mojo/public/cpp/bindings/binding.h"
#include "url/origin.h"

namespace storage {
class FileSystemContext;
}

namespace content {
class RenderFrameHost;

// This class implements the media::mojom::CdmStorage using the
// PluginPrivateFileSystem for backwards compatibility with CDMs running
// as a pepper plugin.
// TODO(jrummell): Use FrameServiceBase<> once landed.
class CONTENT_EXPORT CdmStorageImpl : public media::mojom::CdmStorage {
 public:
  // The file system is different for each plugin type and each origin. So
  // track files in use based on the tuple plugin type, origin, and file name.
  using FileLockKey = std::tuple<std::string, url::Origin, std::string>;

  // Create a CdmStorageImpl object and bind it to |request|.
  static void Create(content::RenderFrameHost* render_frame_host,
                     const std::string& filesystem_plugin_id,
                     media::mojom::CdmStorageRequest request);

  // media::mojom::CdmStorage implementation.
  void Open(const std::string& file_name, OpenCallback callback) final;

 private:
  friend class CdmStorageTest;

  CdmStorageImpl(int child_process_id,
                 const url::Origin& origin,
                 const std::string& filesystem_plugin_id,
                 scoped_refptr<storage::FileSystemContext> file_system_context,
                 media::mojom::CdmStorageRequest request);
  ~CdmStorageImpl() final;

  // Called when the file system has been opened (OpenPluginPrivateFileSystem
  // is asynchronous).
  void OnFileSystemOpened(const FileLockKey& key,
                          const std::string& fsid,
                          OpenCallback callback,
                          base::File::Error error);

  // Called when the requested file has been opened.
  void OnFileOpened(const FileLockKey& key,
                    OpenCallback callback,
                    base::File file,
                    const base::Closure& on_close_callback);

  // Called when the connection to the client is closed or RenderFrameHost is
  // no longer valid. This causes |this| to be destroyed.
  void Close();

  int child_process_id_;
  url::Origin origin_;
  mojo::Binding<media::mojom::CdmStorage> binding_;

  // Files are stored in the PluginPrivateFileSystem, so keep track of the
  // plugin ID in order to open the files in the correct context. As the files
  // were initially used by the CDM running as a pepper plugin, the plugin ID
  // must be based on the pepper plugin MIME type (as was done by pepper).
  std::string filesystem_plugin_id_;
  scoped_refptr<storage::FileSystemContext> filesystem_context_;

  // As a lock is taken on a file when Open() is called, it needs to be
  // released if the async operations to open the file haven't completed
  // by the time |this| is destroyed. So keep track of FileLockKey for files
  // that have a lock on them but failed to finish.
  std::set<FileLockKey> pending_open_;

  THREAD_CHECKER(thread_checker_);

  base::WeakPtrFactory<CdmStorageImpl> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(CdmStorageImpl);
};

}  // namespace content

#endif  // CHROME_BROWSER_MEDIA_CDM_STORAGE_IMPL_H_
