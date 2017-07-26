// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "modules/storage/StorageNamespaceController.h"

#include "core/exported/WebViewBase.h"
#include "core/frame/ContentSettingsClient.h"
#include "modules/storage/InspectorDOMStorageAgent.h"
#include "modules/storage/StorageNamespace.h"
#include "platform/wtf/PtrUtil.h"
#include "public/platform/WebStorageNamespace.h"
#include "public/web/WebViewClient.h"

namespace blink {

#define STATIC_ASSERT_MATCHING_ENUM(enum_name1, enum_name2)                   \
  static_assert(static_cast<int>(enum_name1) == static_cast<int>(enum_name2), \
                "mismatching enums: " #enum_name1)
STATIC_ASSERT_MATCHING_ENUM(kLocalStorage,
                            ContentSettingsClient::StorageType::kLocal);
STATIC_ASSERT_MATCHING_ENUM(kSessionStorage,
                            ContentSettingsClient::StorageType::kSession);

const char* StorageNamespaceController::SupplementName() {
  return "StorageNamespaceController";
}

StorageNamespaceController::StorageNamespaceController(WebViewBase& web_view)
    : web_view_(&web_view), inspector_agent_(nullptr) {}

StorageNamespaceController::~StorageNamespaceController() {}

DEFINE_TRACE(StorageNamespaceController) {
  Supplement<Page>::Trace(visitor);
  visitor->Trace(inspector_agent_);
}

StorageNamespace* StorageNamespaceController::SessionStorage(
    bool optional_create) {
  if (!session_storage_ && optional_create)
    session_storage_ = CreateSessionStorageNamespace();
  return session_storage_.get();
}

void StorageNamespaceController::ProvideStorageNamespaceTo(
    Page& page,
    WebViewBase& web_view) {
  StorageNamespaceController::ProvideTo(
      page, SupplementName(), new StorageNamespaceController(web_view));
}

std::unique_ptr<StorageNamespace>
StorageNamespaceController::CreateSessionStorageNamespace() {
  if (!web_view_->Client())
    return nullptr;
  return WTF::WrapUnique(new StorageNamespace(
      WTF::WrapUnique(web_view_->Client()->CreateSessionStorageNamespace())));
}

bool StorageNamespaceController::CanAccessStorage(LocalFrame* frame,
                                                  StorageType type) const {
  DCHECK(frame->GetContentSettingsClient());
  return frame->GetContentSettingsClient()->AllowStorage(
      static_cast<ContentSettingsClient::StorageType>(type));
}

}  // namespace blink
