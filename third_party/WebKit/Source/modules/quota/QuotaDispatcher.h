// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef QuotaDispatcher_h
#define QuotaDispatcher_h

#include <stdint.h>

#include <map>
#include <memory>
#include <set>

#include "base/macros.h"
#include "core/dom/ExecutionContext.h"
#include "platform/Supplementable.h"
#include "third_party/WebKit/common/quota/quota_dispatcher_host.mojom-blink.h"

namespace service_manager {
class InterfaceProvider;
}

namespace blink {

class SecurityOrigin;

// Dispatches and sends quota related messages sent to/from a child process
// from/to the main browser process. There is one QuotaDispatcher per execution
// context.
// TODO(sashab): Remove this class.
class QuotaDispatcher final : public GarbageCollectedFinalized<QuotaDispatcher>,
                              public Supplement<ExecutionContext> {
  USING_GARBAGE_COLLECTED_MIXIN(QuotaDispatcher)
 public:
  static QuotaDispatcher* From(ExecutionContext*);
  static const char* SupplementName();

  explicit QuotaDispatcher(ExecutionContext&);
  ~QuotaDispatcher();

  void QueryStorageUsageAndQuota(
      service_manager::InterfaceProvider*,
      const scoped_refptr<const SecurityOrigin>,
      mojom::StorageType,
      mojom::blink::QuotaDispatcherHost::QueryStorageUsageAndQuotaCallback);
  void RequestStorageQuota(
      service_manager::InterfaceProvider*,
      const scoped_refptr<const SecurityOrigin>,
      mojom::StorageType,
      int64_t requested_size,
      mojom::blink::QuotaDispatcherHost::RequestStorageQuotaCallback);

  virtual void Trace(Visitor*);

 private:
  // Binds the interface (if not already bound) with the given interface
  // provider, and returns it,
  mojom::blink::QuotaDispatcherHost* QuotaHost(
      service_manager::InterfaceProvider*);

  mojom::blink::QuotaDispatcherHostPtr quota_host_;

  DISALLOW_COPY_AND_ASSIGN(QuotaDispatcher);
};

}  // namespace blink

#endif  // QuotaDispatcher_h
