// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef DEVICE_BLUETOOTH_BLUETOOTH_LOCAL_GATT_SERVICE_H_
#define DEVICE_BLUETOOTH_BLUETOOTH_LOCAL_GATT_SERVICE_H_

#include <stdint.h>
#include <vector>

#include "base/callback.h"
#include "base/callback_forward.h"
#include "base/macros.h"
#include "device/bluetooth/bluetooth_export.h"
#include "device/bluetooth/bluetooth_gatt_service.h"
#include "device/bluetooth/bluetooth_uuid.h"

namespace device {

class BluetoothLocalGattCharacteristic;
class BluetoothLocalGattDescriptor;

// BluetoothLocalGattService represents a local GATT service.
//
// Instances of the BluetoothLocalGattService class are used to represent a
// locally hosted GATT attribute hierarchy when the local
// adapter is used in the "peripheral" role. Such instances are meant to be
// constructed directly and registered. Once registered, a GATT attribute
// hierarchy will be visible to remote devices in the "central" role.
//
// Note: We use virtual inheritance on the gatt service since it will be
// inherited by platform specific versions of the gatt service classes also. The
// platform specific local gatt service classes will inherit both this class and
// their gatt service class, hence causing an inheritance diamond.
class DEVICE_BLUETOOTH_EXPORT BluetoothLocalGattService
    : public virtual BluetoothGattService {
 public:
  // The Delegate class is used to send certain events that need to be handled
  // when the device is in peripheral mode. The delegate handles read and write
  // requests that are issued from remote clients.
  class Delegate {
   public:
    // Callbacks used for communicating GATT request responses.
    typedef base::Callback<void(const std::vector<uint8_t>)> ValueCallback;
    typedef base::Closure ErrorCallback;

    // Called when a remote device in the central role requests to read the
    // value of the characteristic |characteristic| starting at offset |offset|.
    // This method is only called if the characteristic was specified as
    // readable and any authentication and authorization challenges were
    // satisfied by the remote device.
    //
    // To respond to the request with success and return the requested value,
    // the delegate must invoke |callback| with the value. Doing so will
    // automatically update the value property of |characteristic|. To respond
    // to the request with failure (e.g. if an invalid offset was given),
    // delegates must invoke |error_callback|. If neither callback parameter is
    // invoked, the request will time out and result in an error. Therefore,
    // delegates MUST invoke either |callback| or |error_callback|.
    virtual void OnCharacteristicReadRequest(
        const BluetoothLocalGattService* service,
        const BluetoothLocalGattCharacteristic* characteristic,
        int offset,
        const ValueCallback& callback,
        const ErrorCallback& error_callback) = 0;

    // Called when a remote device in the central role requests to write the
    // value of the characteristic |characteristic| starting at offset |offset|.
    // This method is only called if the characteristic was specified as
    // writable and any authentication and authorization challenges were
    // satisfied by the remote device.
    //
    // To respond to the request with success the delegate must invoke
    // |callback| with the new value of the characteristic. Doing so will
    // automatically update the value property of |characteristic|. To respond
    // to the request with failure (e.g. if an invalid offset was given),
    // delegates must invoke |error_callback|. If neither callback parameter is
    // invoked, the request will time out and result in an error. Therefore,
    // delegates MUST invoke either |callback| or |error_callback|.
    virtual void OnCharacteristicWriteRequest(
        const BluetoothLocalGattService* service,
        const BluetoothLocalGattCharacteristic* characteristic,
        const std::vector<uint8_t>& value,
        int offset,
        const ValueCallback& callback,
        const ErrorCallback& error_callback) = 0;

    // Called when a remote device in the central role requests to read the
    // value of the descriptor |descriptor| starting at offset |offset|.
    // This method is only called if the characteristic was specified as
    // readable and any authentication and authorization challenges were
    // satisfied by the remote device.
    //
    // To respond to the request with success and return the requested value,
    // the delegate must invoke |callback| with the value. Doing so will
    // automatically update the value property of |descriptor|. To respond
    // to the request with failure (e.g. if an invalid offset was given),
    // delegates must invoke |error_callback|. If neither callback parameter is
    // invoked, the request will time out and result in an error. Therefore,
    // delegates MUST invoke either |callback| or |error_callback|.
    virtual void OnDescriptorReadRequest(
        const BluetoothLocalGattService* service,
        const BluetoothLocalGattDescriptor* descriptor,
        int offset,
        const ValueCallback& callback,
        const ErrorCallback& error_callback) = 0;

    // Called when a remote device in the central role requests to write the
    // value of the descriptor |descriptor| starting at offset |offset|.
    // This method is only called if the characteristic was specified as
    // writable and any authentication and authorization challenges were
    // satisfied by the remote device.
    //
    // To respond to the request with success the delegate must invoke
    // |callback| with the new value of the descriptor. Doing so will
    // automatically update the value property of |descriptor|. To respond
    // to the request with failure (e.g. if an invalid offset was given),
    // delegates must invoke |error_callback|. If neither callback parameter is
    // invoked, the request will time out and result in an error. Therefore,
    // delegates MUST invoke either |callback| or |error_callback|.
    virtual void OnDescriptorWriteRequest(
        const BluetoothLocalGattService* service,
        const BluetoothLocalGattDescriptor* descriptor,
        const std::vector<uint8_t>& value,
        int offset,
        const ValueCallback& callback,
        const ErrorCallback& error_callback) = 0;
  };

  ~BluetoothLocalGattService() override;

  // Constructs a BluetoothLocalGattService that can be locally hosted when the
  // local adapter is in the peripheral role. The resulting object can then be
  // made available by calling the "Register" method. This method constructs a
  // service with UUID |uuid|. Whether the constructed service is primary or
  // secondary is determined by |is_primary|. |delegate| is used to send certain
  // peripheral role events. If |delegate| is NULL, then this service will
  // employ a default behavior when responding to read and write requests based
  // on the cached value of its characteristics and descriptors at a given time.
  // TODO(rkc): Implement included services.
  static BluetoothLocalGattService* Create(
      const BluetoothUUID& uuid,
      bool is_primary,
      BluetoothLocalGattService* included_service,
      Delegate* delegate);

  // Registers this GATT service. Calling Register will make this service and
  // all of its associated attributes available on the local adapters GATT
  // database and the service UUID will be advertised to nearby devices if the
  // local adapter is discoverable. Call Unregister to make this service no
  // longer available.
  //
  // These methods only make sense for services that are local and will hence
  // fail if this instance represents a remote GATT service. |callback| is
  // called to denote success and |error_callback| to denote failure.
  virtual void Register(const base::Closure& callback,
                        const ErrorCallback& error_callback) = 0;
  virtual void Unregister(const base::Closure& callback,
                          const ErrorCallback& error_callback) = 0;

 protected:
  BluetoothLocalGattService();

 private:
  DISALLOW_COPY_AND_ASSIGN(BluetoothLocalGattService);
};

}  // namespace device

#endif  // DEVICE_BLUETOOTH_BLUETOOTH_LOCAL_GATT_SERVICE_H_
