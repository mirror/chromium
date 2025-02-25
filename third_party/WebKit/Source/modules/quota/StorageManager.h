// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef StorageManager_h
#define StorageManager_h

#include "platform/bindings/ScriptWrappable.h"
#include "platform/heap/Heap.h"
#include "public/platform/modules/permissions/permission.mojom-blink.h"
#include "third_party/WebKit/common/quota/quota_dispatcher_host.mojom-blink.h"

namespace blink {

class ExecutionContext;
class ScriptPromise;
class ScriptPromiseResolver;
class ScriptState;

class StorageManager final : public ScriptWrappable {
  DEFINE_WRAPPERTYPEINFO();

 public:
  ScriptPromise persisted(ScriptState*);
  ScriptPromise persist(ScriptState*);

  ScriptPromise estimate(ScriptState*);

 private:
  mojom::blink::PermissionService& GetPermissionService(ExecutionContext*);
  void PermissionServiceConnectionError();
  void PermissionRequestComplete(ScriptPromiseResolver*,
                                 mojom::blink::PermissionStatus);

  // Binds the interface (if not already bound) with the given interface
  // provider, and returns it,
  mojom::blink::QuotaDispatcherHost& GetQuotaHost(ExecutionContext*);

  mojom::blink::PermissionServicePtr permission_service_;
  mojom::blink::QuotaDispatcherHostPtr quota_host_;
};

}  // namespace blink

#endif  // StorageManager_h
