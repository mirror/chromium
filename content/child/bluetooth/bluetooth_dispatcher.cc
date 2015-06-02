// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/child/bluetooth/bluetooth_dispatcher.h"

#include "base/lazy_instance.h"
#include "base/memory/scoped_ptr.h"
#include "base/message_loop/message_loop.h"
#include "base/thread_task_runner_handle.h"
#include "content/child/thread_safe_sender.h"
#include "content/common/bluetooth/bluetooth_messages.h"
#include "third_party/WebKit/public/platform/modules/bluetooth/WebBluetoothDevice.h"
#include "third_party/WebKit/public/platform/modules/bluetooth/WebBluetoothError.h"
#include "third_party/WebKit/public/platform/modules/bluetooth/WebBluetoothGATTRemoteServer.h"
#include "third_party/WebKit/public/platform/modules/bluetooth/WebBluetoothGATTService.h"

using blink::WebBluetoothConnectGATTCallbacks;
using blink::WebBluetoothDevice;
using blink::WebBluetoothError;
using blink::WebBluetoothGATTRemoteServer;
using blink::WebBluetoothGATTService;
using blink::WebBluetoothRequestDeviceCallbacks;
using blink::WebString;
using blink::WebVector;

struct BluetoothPrimaryServiceRequest {
  BluetoothPrimaryServiceRequest(
      blink::WebString device_instance_id,
      blink::WebString service_uuid,
      blink::WebBluetoothGetPrimaryServiceCallbacks* callbacks)
      : device_instance_id(device_instance_id),
        service_uuid(service_uuid),
        callbacks(callbacks) {}
  ~BluetoothPrimaryServiceRequest() {}

  blink::WebString device_instance_id;
  blink::WebString service_uuid;
  scoped_ptr<blink::WebBluetoothGetPrimaryServiceCallbacks> callbacks;
};

namespace content {

namespace {

base::LazyInstance<base::ThreadLocalPointer<BluetoothDispatcher>>::Leaky
    g_dispatcher_tls = LAZY_INSTANCE_INITIALIZER;

BluetoothDispatcher* const kHasBeenDeleted =
    reinterpret_cast<BluetoothDispatcher*>(0x1);

int CurrentWorkerId() {
  return WorkerTaskRunner::Instance()->CurrentWorkerId();
}

WebBluetoothError::ErrorType WebBluetoothErrorFromBluetoothError(
    BluetoothError error_type) {
  switch (error_type) {
    case BluetoothError::NETWORK_ERROR:
      return WebBluetoothError::NetworkError;
    case BluetoothError::NOT_FOUND:
      return WebBluetoothError::NotFoundError;
    case BluetoothError::SECURITY:
      return WebBluetoothError::SecurityError;
  }
  NOTIMPLEMENTED();
  return WebBluetoothError::NotFoundError;
}

WebBluetoothDevice::VendorIDSource GetWebVendorIdSource(
    device::BluetoothDevice::VendorIDSource vendor_id_source) {
  switch (vendor_id_source) {
    case device::BluetoothDevice::VENDOR_ID_UNKNOWN:
      return WebBluetoothDevice::VendorIDSource::Unknown;
    case device::BluetoothDevice::VENDOR_ID_BLUETOOTH:
      return WebBluetoothDevice::VendorIDSource::Bluetooth;
    case device::BluetoothDevice::VENDOR_ID_USB:
      return WebBluetoothDevice::VendorIDSource::USB;
  }
  NOTREACHED();
  return WebBluetoothDevice::VendorIDSource::Unknown;
}

}  // namespace

BluetoothDispatcher::BluetoothDispatcher(ThreadSafeSender* sender)
    : thread_safe_sender_(sender) {
  g_dispatcher_tls.Pointer()->Set(this);
}

BluetoothDispatcher::~BluetoothDispatcher() {
  g_dispatcher_tls.Pointer()->Set(kHasBeenDeleted);
}

BluetoothDispatcher* BluetoothDispatcher::GetOrCreateThreadSpecificInstance(
    ThreadSafeSender* thread_safe_sender) {
  if (g_dispatcher_tls.Pointer()->Get() == kHasBeenDeleted) {
    NOTREACHED() << "Re-instantiating TLS BluetoothDispatcher.";
    g_dispatcher_tls.Pointer()->Set(NULL);
  }
  if (g_dispatcher_tls.Pointer()->Get())
    return g_dispatcher_tls.Pointer()->Get();

  BluetoothDispatcher* dispatcher = new BluetoothDispatcher(thread_safe_sender);
  if (CurrentWorkerId())
    WorkerTaskRunner::Instance()->AddStopObserver(dispatcher);
  return dispatcher;
}

bool BluetoothDispatcher::Send(IPC::Message* msg) {
  return thread_safe_sender_->Send(msg);
}

void BluetoothDispatcher::OnMessageReceived(const IPC::Message& msg) {
  bool handled = true;
  IPC_BEGIN_MESSAGE_MAP(BluetoothDispatcher, msg)
  IPC_MESSAGE_HANDLER(BluetoothMsg_RequestDeviceSuccess,
                      OnRequestDeviceSuccess);
  IPC_MESSAGE_HANDLER(BluetoothMsg_RequestDeviceError, OnRequestDeviceError);
  IPC_MESSAGE_HANDLER(BluetoothMsg_ConnectGATTSuccess, OnConnectGATTSuccess);
  IPC_MESSAGE_HANDLER(BluetoothMsg_ConnectGATTError, OnConnectGATTError);
  IPC_MESSAGE_HANDLER(BluetoothMsg_GetPrimaryServiceSuccess,
                      OnGetPrimaryServiceSuccess);
  IPC_MESSAGE_HANDLER(BluetoothMsg_GetPrimaryServiceError,
                      OnGetPrimaryServiceError);
  IPC_MESSAGE_UNHANDLED(handled = false)
  IPC_END_MESSAGE_MAP()
  DCHECK(handled) << "Unhandled message:" << msg.type();
}

void BluetoothDispatcher::requestDevice(
    blink::WebBluetoothRequestDeviceCallbacks* callbacks) {
  int request_id = pending_requests_.Add(callbacks);
  Send(new BluetoothHostMsg_RequestDevice(CurrentWorkerId(), request_id));
}

void BluetoothDispatcher::connectGATT(
    const blink::WebString& device_instance_id,
    blink::WebBluetoothConnectGATTCallbacks* callbacks) {
  int request_id = pending_connect_requests_.Add(callbacks);
  Send(new BluetoothHostMsg_ConnectGATT(CurrentWorkerId(), request_id,
                                        device_instance_id.utf8()));
}

void BluetoothDispatcher::getPrimaryService(
    const blink::WebString& device_instance_id,
    const blink::WebString& service_uuid,
    blink::WebBluetoothGetPrimaryServiceCallbacks* callbacks) {
  int request_id =
      pending_primary_service_requests_.Add(new BluetoothPrimaryServiceRequest(
          device_instance_id, service_uuid, callbacks));
  Send(new BluetoothHostMsg_GetPrimaryService(CurrentWorkerId(), request_id,
                                              device_instance_id.utf8(),
                                              service_uuid.utf8()));
}

void BluetoothDispatcher::OnWorkerRunLoopStopped() {
  delete this;
}

void BluetoothDispatcher::OnRequestDeviceSuccess(
    int thread_id,
    int request_id,
    const BluetoothDevice& device) {
  DCHECK(pending_requests_.Lookup(request_id)) << request_id;

  WebVector<WebString> uuids(device.uuids.size());
  for (size_t i = 0; i < device.uuids.size(); ++i)
    uuids[i] = WebString::fromUTF8(device.uuids[i].c_str());

  pending_requests_.Lookup(request_id)
      ->onSuccess(new WebBluetoothDevice(
          WebString::fromUTF8(device.instance_id), WebString(device.name),
          device.device_class, GetWebVendorIdSource(device.vendor_id_source),
          device.vendor_id, device.product_id, device.product_version,
          device.paired, uuids));
  pending_requests_.Remove(request_id);
}

void BluetoothDispatcher::OnRequestDeviceError(int thread_id,
                                               int request_id,
                                               BluetoothError error_type) {
  DCHECK(pending_requests_.Lookup(request_id)) << request_id;
  pending_requests_.Lookup(request_id)
      ->onError(new WebBluetoothError(
          // TODO(ortuno): Return more descriptive error messages.
          // http://crbug.com/490419
          WebBluetoothErrorFromBluetoothError(error_type), ""));
  pending_requests_.Remove(request_id);
}

void BluetoothDispatcher::OnConnectGATTSuccess(
    int thread_id,
    int request_id,
    const std::string& device_instance_id) {
  DCHECK(pending_connect_requests_.Lookup(request_id)) << request_id;
  pending_connect_requests_.Lookup(request_id)
      ->onSuccess(new WebBluetoothGATTRemoteServer(
          WebString::fromUTF8(device_instance_id), true /* connected */));
  pending_connect_requests_.Remove(request_id);
}

void BluetoothDispatcher::OnConnectGATTError(int thread_id,
                                             int request_id,
                                             BluetoothError error_type) {
  DCHECK(pending_connect_requests_.Lookup(request_id)) << request_id;
  pending_connect_requests_.Lookup(request_id)
      ->onError(new WebBluetoothError(
          // TODO(ortuno): Return more descriptive error messages.
          // http://crbug.com/490419
          WebBluetoothErrorFromBluetoothError(error_type), ""));
  pending_connect_requests_.Remove(request_id);
}

void BluetoothDispatcher::OnGetPrimaryServiceSuccess(
    int thread_id,
    int request_id,
    const std::string& service_instance_id) {
  DCHECK(pending_primary_service_requests_.Lookup(request_id)) << request_id;
  BluetoothPrimaryServiceRequest* request =
      pending_primary_service_requests_.Lookup(request_id);
  request->callbacks->onSuccess(new WebBluetoothGATTService(
      WebString::fromUTF8(service_instance_id), request->service_uuid,
      true /* isPrimary */, request->device_instance_id));
  pending_primary_service_requests_.Remove(request_id);
}

void BluetoothDispatcher::OnGetPrimaryServiceError(int thread_id,
                                                   int request_id,
                                                   BluetoothError error_type) {
  DCHECK(pending_primary_service_requests_.Lookup(request_id)) << request_id;

  // Since we couldn't find the service return null. See Step 3 of
  // getPrimaryService algorithm:
  // https://webbluetoothcg.github.io/web-bluetooth/#dom-bluetoothgattremoteserver-getprimaryservice
  if (error_type == BluetoothError::NOT_FOUND) {
    pending_primary_service_requests_.Lookup(request_id)
        ->callbacks->onSuccess(nullptr);
    pending_primary_service_requests_.Remove(request_id);
    return;
  }

  pending_primary_service_requests_.Lookup(request_id)
      ->callbacks->onError(new WebBluetoothError(
          // TODO(ortuno): Return more descriptive error messages.
          // http://crbug.com/490419
          WebBluetoothErrorFromBluetoothError(error_type), ""));
  pending_primary_service_requests_.Remove(request_id);
}

}  // namespace content
