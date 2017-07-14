// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SharedWorkerContentSettingsProxy_h
#define SharedWorkerContentSettingsProxy_h

#include "public/platform/WebContentSettingsClient.h"
#include "public/platform/WebSecurityOrigin.h"
#include "public/web/shared_worker_content_settings.mojom-blink.h"

namespace blink {

// This class provides some content settings information.
// Each information are offered via mojo connection which is established on each
// method calls.
// This is assumed to be used only by shared worker.
class SharedWorkerContentSettingsProxy : public WebContentSettingsClient {
 public:
  SharedWorkerContentSettingsProxy(const KURL& origin_url,
                                   bool is_unique_origin,
                                   int route_id);
  ~SharedWorkerContentSettingsProxy() override;

  // WebContentSettingsClient overrides.
  bool AllowIndexedDB(const WebString& name, const WebSecurityOrigin&) override;
  bool RequestFileSystemAccessSync() override;

 private:
  // Please use origin_url_ with IsolatedCopy() as in AllowIndexedDB() or
  // RequestFileSystemAccessSync(), because it is created on the main thread but
  // used on the worker thread.
  const KURL origin_url_;
  const bool is_unique_origin_;
  const int route_id_;

  // Returns an initialized SharedWorkerContentSettingsManagerPtr.
  // A connection will be established on each calls to this method, because this
  // pointer can be used only in the process which made it.
  mojom::blink::SharedWorkerContentSettingsManagerPtr GetService();
};

}  // namespace blink

#endif  // SharedWorkerContentSettingsProxy_h
