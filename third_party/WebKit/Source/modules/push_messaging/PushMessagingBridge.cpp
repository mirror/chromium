// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "modules/push_messaging/PushMessagingBridge.h"

#include "core/dom/Document.h"
#include "core/frame/Frame.h"
#include "core/frame/LocalFrame.h"
#include "core/frame/WebLocalFrameImpl.h"
#include "modules/permissions/PermissionUtils.h"
#include "platform/wtf/Functional.h"
#include "public/platform/modules/push_messaging/WebPushClient.h"
#include "public/web/WebFrameClient.h"
#include "services/service_manager/public/cpp/interface_provider.h"

namespace blink {

namespace {

WebFrameClient& GetWebFrameClient(LocalFrame& frame) {
  WebLocalFrameImpl* web_frame = WebLocalFrameImpl::FromFrame(frame);
  DCHECK(web_frame);
  DCHECK(web_frame->Client());
  return *web_frame->Client();
}

}  // namespace

// static
PushMessagingBridge* PushMessagingBridge::From(
    ServiceWorkerRegistration* service_worker_registration) {
  DCHECK(service_worker_registration);

  PushMessagingBridge* bridge = static_cast<PushMessagingBridge*>(
      Supplement<ServiceWorkerRegistration>::From(service_worker_registration,
                                                  SupplementName()));

  if (!bridge) {
    bridge = new PushMessagingBridge(*service_worker_registration);
    Supplement<ServiceWorkerRegistration>::ProvideTo(
        *service_worker_registration, SupplementName(), bridge);
  }

  return bridge;
}

PushMessagingBridge::PushMessagingBridge(
    ServiceWorkerRegistration& registration)
    : Supplement<ServiceWorkerRegistration>(registration) {}

PushMessagingBridge::~PushMessagingBridge() = default;

// static
const char* PushMessagingBridge::SupplementName() {
  return "PushMessagingBridge";
}

void PushMessagingBridge::RequestPermission(
    base::OnceCallback<void(mojom::blink::PermissionStatus)> callback) {
  ExecutionContext* context = GetExecutionContext();
  if (!permission_service_) {
    ConnectToPermissionService(context,
                               mojo::MakeRequest(&permission_service_));
  }

  Document* document = ToDocumentOrNull(context);
  Frame* frame = document ? document->GetFrame() : nullptr;

  permission_service_->RequestPermission(
      CreatePermissionDescriptor(mojom::blink::PermissionName::NOTIFICATIONS),
      Frame::HasTransientUserActivation(frame, true /* checkIfMainThread */),
      std::move(callback));
}

void PushMessagingBridge::GetPermissionStatus(
    base::OnceCallback<void(mojom::blink::PermissionStatus)> callback) {
  if (!permission_service_) {
    ConnectToPermissionService(GetExecutionContext(),
                               mojo::MakeRequest(&permission_service_));
  }

  permission_service_->HasPermission(
      CreatePermissionDescriptor(mojom::blink::PermissionName::NOTIFICATIONS),
      std::move(callback));
}

void PushMessagingBridge::GetApplicationServerKeyFromManifest(
    base::OnceCallback<void(const String&)> callback) {
  DCHECK(GetExecutionContext()->IsDocument());

  Document* document = ToDocument(GetExecutionContext());
  LocalFrame* frame = document->GetFrame();
  DCHECK(frame);

  GetWebFrameClient(*frame).PushClient()->GetApplicationServerKeyFromManifest(
      WTF::Bind(&PushMessagingBridge::GotApplicationServerKeyFromManifest,
                WrapWeakPersistent(this), std::move(callback)));
}

void PushMessagingBridge::GotApplicationServerKeyFromManifest(
    base::OnceCallback<void(const String&)> callback,
    const WebString& application_server_key) {
  std::move(callback).Run(application_server_key);
}

ExecutionContext* PushMessagingBridge::GetExecutionContext() {
  return GetSupplementable()->GetExecutionContext();
}

}  // namespace blink
