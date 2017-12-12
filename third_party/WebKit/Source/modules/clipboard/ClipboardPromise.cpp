// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "modules/clipboard/ClipboardPromise.h"

#include "bindings/core/v8/ScriptPromiseResolver.h"
#include "core/clipboard/DataObject.h"
#include "core/clipboard/DataTransfer.h"
#include "core/clipboard/DataTransferItem.h"
#include "core/clipboard/DataTransferItemList.h"
#include "core/dom/ExecutionContext.h"
#include "core/frame/LocalFrame.h"
#include "modules/permissions/PermissionUtils.h"
#include "platform/clipboard/ClipboardMimeTypes.h"
#include "public/platform/Platform.h"
#include "public/platform/TaskType.h"
#include "public/platform/modules/permissions/permission.mojom-blink.h"
#include "third_party/WebKit/public/platform/WebClipboard.h"

// And now, a brief note about clipboard permissions.
//
// There are 2 clipboard permissions defined in the spec:
// * clipboard-read
// * clipboard-write
// See https://w3c.github.io/clipboard-apis/#clipboard-permissions
//
// However, in Chromium, we want to present a single "Clipboard" permission to
// the user. And (for consistency with what we've shipped in previous versions
// of Chrome) we want to automatically grant clipboard-write access (as long as
// Chrome is the foreground app and the request comes from the active tab).
// AND
// We need to allow the user to be able to request full read/write access to
// the clipboard (without requiring a gesture).
//
// We do this by:
// (1) Using clipboard-read permission as a stand-in for "full read/write"
// (2) Always allowing write requests as long as the document is currently
//     focused (and Chome is the currently focused app).
// (3) Allowing write requests if the document is currently visible AND the
//     clipboard-read (aka: "full") permission has already been granted.
// (4) Allowing read requests if the document is currently visible. In this
//     case, we'll ask the user for permission if it is not already granted.

namespace blink {

using mojom::blink::PermissionStatus;
using mojom::blink::PermissionService;
using mojom::PageVisibilityState;

ScriptPromise ClipboardPromise::CreateForRead(ScriptState* script_state) {
  ClipboardPromise* clipboard_promise = new ClipboardPromise(script_state);
  clipboard_promise->GetTaskRunner()->PostTask(
      BLINK_FROM_HERE, WTF::Bind(&ClipboardPromise::HandleRead,
                                 WrapPersistent(clipboard_promise)));
  return clipboard_promise->script_promise_resolver_->Promise();
}

ScriptPromise ClipboardPromise::CreateForReadText(ScriptState* script_state) {
  ClipboardPromise* clipboard_promise = new ClipboardPromise(script_state);
  clipboard_promise->GetTaskRunner()->PostTask(
      BLINK_FROM_HERE, WTF::Bind(&ClipboardPromise::HandleReadText,
                                 WrapPersistent(clipboard_promise)));
  return clipboard_promise->script_promise_resolver_->Promise();
}

ScriptPromise ClipboardPromise::CreateForWrite(ScriptState* script_state,
                                               DataTransfer* data) {
  ClipboardPromise* clipboard_promise = new ClipboardPromise(script_state);
  clipboard_promise->GetTaskRunner()->PostTask(
      BLINK_FROM_HERE,
      WTF::Bind(&ClipboardPromise::HandleWrite,
                WrapPersistent(clipboard_promise), WrapPersistent(data)));
  return clipboard_promise->script_promise_resolver_->Promise();
}

ScriptPromise ClipboardPromise::CreateForWriteText(ScriptState* script_state,
                                                   const String& data) {
  ClipboardPromise* clipboard_promise = new ClipboardPromise(script_state);
  clipboard_promise->GetTaskRunner()->PostTask(
      BLINK_FROM_HERE, WTF::Bind(&ClipboardPromise::HandleWriteText,
                                 WrapPersistent(clipboard_promise), data));
  return clipboard_promise->script_promise_resolver_->Promise();
}

ClipboardPromise::ClipboardPromise(ScriptState* script_state)
    : ContextLifecycleObserver(blink::ExecutionContext::From(script_state)),
      script_state_(script_state),
      script_promise_resolver_(ScriptPromiseResolver::Create(script_state)),
      buffer_(mojom::ClipboardBuffer::kStandard),
      write_data_() {}

scoped_refptr<WebTaskRunner> ClipboardPromise::GetTaskRunner() {
  // TODO(garykac): Replace MiscPlatformAPI with TaskType specific to clipboard.
  return GetExecutionContext()->GetTaskRunner(TaskType::kMiscPlatformAPI);
}

PermissionService* ClipboardPromise::GetPermissionService() {
  if (!permission_service_) {
    ConnectToPermissionService(ExecutionContext::From(script_state_),
                               mojo::MakeRequest(&permission_service_));
  }
  return permission_service_.get();
}

// Return true if the associated document is currently visible (ignoring whether
// or not it is currently focused).
bool ClipboardPromise::IsVisibleDocument(ExecutionContext* context) {
  DCHECK(context->IsDocument());
  Document* doc = ToDocumentOrNull(context);
  return doc && doc->GetPageVisibilityState() == PageVisibilityState::kVisible;
}

// Return true if the associated document is focused (and the browser is the
// current foreground application).
bool ClipboardPromise::IsFocusedDocument(ExecutionContext* context) {
  DCHECK(context->IsDocument());
  Document* doc = ToDocumentOrNull(context);
  return doc && doc->hasFocus();
}

void ClipboardPromise::RequestReadPermission(
    PermissionService::RequestPermissionCallback callback) {
  DCHECK(script_promise_resolver_);

  ExecutionContext* context = ExecutionContext::From(script_state_);
  DCHECK(context->IsSecureContext());  // [SecureContext] in IDL

  // Document must be visible (but not necessarily focused).
  if (!IsVisibleDocument(context) || !GetPermissionService()) {
    script_promise_resolver_->Reject();
    return;
  }
  permission_service_->RequestPermission(
      CreateClipboardPermissionDescriptor(
          mojom::blink::PermissionName::CLIPBOARD_READ, false),
      context->GetSecurityOrigin(), false, std::move(callback));
}

void ClipboardPromise::CheckWritePermission(
    PermissionService::HasPermissionCallback callback) {
  DCHECK(script_promise_resolver_);

  ExecutionContext* context = ExecutionContext::From(script_state_);
  DCHECK(context->IsSecureContext());  // [SecureContext] in IDL

  // The write was not auto-granted, so (1) make sure that the document is
  // visible and (2) check to see if READ (=full) permission has already been
  // granted. Note that this will not request permission -- if READ permission
  // has not already been granted, then the call will be rejected.
  if (!IsVisibleDocument(context) || !GetPermissionService()) {
    script_promise_resolver_->Reject();
    return;
  }

  write_callback_ = std::move(callback);
  permission_service_->HasPermission(
      CreateClipboardPermissionDescriptor(
          mojom::blink::PermissionName::CLIPBOARD_READ, false),
      context->GetSecurityOrigin(),
      WTF::Bind(&ClipboardPromise::CheckWritePermissionCallback,
                WrapPersistent(this)));
}

void ClipboardPromise::CheckWritePermissionCallback(
    PermissionStatus status) {
  // If clipboard access has been explicitly denied, then respect that setting.
  if (status == PermissionStatus::DENIED) {
    script_promise_resolver_->Reject();
    return;
  }

  // Automatically grant write permission if the document has focus.
  if (IsFocusedDocument(ExecutionContext::From(script_state_))) {
    std::move(write_callback_).Run(PermissionStatus::GRANTED);
    return;
  }

  std::move(write_callback_).Run(status);
}

void ClipboardPromise::HandleRead() {
  RequestReadPermission(WTF::Bind(
      &ClipboardPromise::HandleReadTextWithPermission, WrapPersistent(this)));
}

// TODO(garykac): This currently only handles plain text.
void ClipboardPromise::HandleReadWithPermission(PermissionStatus status) {
  if (status != PermissionStatus::GRANTED) {
    script_promise_resolver_->Reject();
    return;
  }

  String plain_text = Platform::Current()->Clipboard()->ReadPlainText(buffer_);

  const DataTransfer::DataTransferType type =
      DataTransfer::DataTransferType::kCopyAndPaste;
  const DataTransferAccessPolicy access =
      DataTransferAccessPolicy::kDataTransferReadable;
  DataObject* data = DataObject::CreateFromString(plain_text);
  DataTransfer* dt = DataTransfer::Create(type, access, data);
  script_promise_resolver_->Resolve(dt);
}

void ClipboardPromise::HandleReadText() {
  RequestReadPermission(WTF::Bind(
      &ClipboardPromise::HandleReadTextWithPermission, WrapPersistent(this)));
}

void ClipboardPromise::HandleReadTextWithPermission(PermissionStatus status) {
  if (status != PermissionStatus::GRANTED) {
    script_promise_resolver_->Reject();
    return;
  }

  String text = Platform::Current()->Clipboard()->ReadPlainText(buffer_);
  script_promise_resolver_->Resolve(text);
}

// TODO(garykac): This currently only handles plain text.
void ClipboardPromise::HandleWrite(DataTransfer* data) {
  // Scan DataTransfer and extract data types that we support.
  size_t num_items = data->items()->length();
  for (unsigned long i = 0; i < num_items; i++) {
    DataTransferItem* item = data->items()->item(i);
    DataObjectItem* objectItem = item->GetDataObjectItem();
    if (objectItem->Kind() == DataObjectItem::kStringKind &&
        objectItem->GetType() == kMimeTypeTextPlain) {
      write_data_ = objectItem->GetAsString();
      break;
    }
  }
  CheckWritePermission(WTF::Bind(
      &ClipboardPromise::HandleWriteWithPermission, WrapPersistent(this)));
}

void ClipboardPromise::HandleWriteWithPermission(PermissionStatus status) {
  if (status != PermissionStatus::GRANTED) {
    script_promise_resolver_->Reject();
    return;
  }

  Platform::Current()->Clipboard()->WritePlainText(write_data_);
  script_promise_resolver_->Resolve();
}

void ClipboardPromise::HandleWriteText(const String& data) {
  write_data_ = data;
  CheckWritePermission(WTF::Bind(
      &ClipboardPromise::HandleWriteTextWithPermission, WrapPersistent(this)));
}

void ClipboardPromise::HandleWriteTextWithPermission(PermissionStatus status) {
  if (status != PermissionStatus::GRANTED) {
    script_promise_resolver_->Reject();
    return;
  }

  DCHECK(script_promise_resolver_);
  Platform::Current()->Clipboard()->WritePlainText(write_data_);
  script_promise_resolver_->Resolve();
}

void ClipboardPromise::Trace(blink::Visitor* visitor) {
  visitor->Trace(script_promise_resolver_);
  ContextLifecycleObserver::Trace(visitor);
}

}  // namespace blink
