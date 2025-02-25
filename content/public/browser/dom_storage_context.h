// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_PUBLIC_BROWSER_DOM_STORAGE_CONTEXT_H_
#define CONTENT_PUBLIC_BROWSER_DOM_STORAGE_CONTEXT_H_

#include <string>
#include <vector>

#include "base/callback.h"

class GURL;

namespace content {

class BrowserContext;
struct LocalStorageUsageInfo;
class SessionStorageNamespace;
struct SessionStorageUsageInfo;

// Represents the per-BrowserContext Local Storage data.
class DOMStorageContext {
 public:
  typedef base::Callback<
      void(const std::vector<LocalStorageUsageInfo>&)>
          GetLocalStorageUsageCallback;

  typedef base::OnceCallback<void(const std::vector<SessionStorageUsageInfo>&)>
      GetSessionStorageUsageCallback;

  // Returns a collection of origins using local storage to the given callback.
  virtual void GetLocalStorageUsage(
      const GetLocalStorageUsageCallback& callback) = 0;

  // Returns a collection of origins using session storage to the given
  // callback.
  virtual void GetSessionStorageUsage(
      GetSessionStorageUsageCallback callback) = 0;

  // Deletes the local storage data for the physical origin of |origin_url|,
  // including all suborigins at the physical origin. |callback| is called when
  // the deletion is sent to the database and GetLocalStorageUsage() will not
  // return entries for |origin_url| anymore.
  //
  // See https://w3c.github.io/webappsec-suborigins/.
  virtual void DeleteLocalStorageForPhysicalOrigin(
      const GURL& origin_url,
      base::OnceClosure callback) = 0;

  // Deletes the local storage for the origin of |origin_url|. |callback| is
  // called when the deletion is sent to the database and GetLocalStorageUsage()
  // will not return entries for |origin_url| anymore.
  virtual void DeleteLocalStorage(const GURL& origin_url,
                                  base::OnceClosure callback) = 0;

  // Deletes the session storage data identified by |usage_info|.
  virtual void DeleteSessionStorage(
      const SessionStorageUsageInfo& usage_info) = 0;

  // If this is called, sessionStorage data will be stored on disk, and can be
  // restored after a browser restart (with RecreateSessionStorage). This
  // function must be called right after DOMStorageContextWrapper is created,
  // and before it's used.
  virtual void SetSaveSessionStorageOnDisk() = 0;

  // Creates a SessionStorageNamespace with the given |namespace_id|. Used
  // after tabs are restored by session restore. When created, the
  // SessionStorageNamespace with the correct |namespace_id| will be
  // associated with the persisted sessionStorage data.
  virtual scoped_refptr<SessionStorageNamespace> RecreateSessionStorage(
      const std::string& namespace_id) = 0;

  // Starts deleting sessionStorages which don't have an associated
  // SessionStorageNamespace alive. Called when SessionStorageNamespaces have
  // been created after a session restore, or a session restore won't happen.
  virtual void StartScavengingUnusedSessionStorage() = 0;

 protected:
  virtual ~DOMStorageContext() {}
};

}  // namespace content

#endif  // CONTENT_PUBLIC_BROWSER_DOM_STORAGE_CONTEXT_H_
