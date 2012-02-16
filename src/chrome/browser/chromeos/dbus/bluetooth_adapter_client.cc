// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/chromeos/dbus/bluetooth_adapter_client.h"

#include <map>

#include "base/bind.h"
#include "base/logging.h"
#include "base/stl_util.h"
#include "chrome/browser/chromeos/dbus/bluetooth_manager_client.h"
#include "chrome/browser/chromeos/system/runtime_environment.h"
#include "dbus/bus.h"
#include "dbus/message.h"
#include "dbus/object_path.h"
#include "dbus/object_proxy.h"
#include "third_party/cros_system_api/dbus/service_constants.h"

namespace {

// Utility function to convert an array of dbus dict_entry objects into a
// DictionaryValue object.
//
// The dict_entry objects must have keys that are strings and values that are
// simple variants.
//
// When converting integral types, we use Integer Value objects to represent
// uint8, int16, uint16, int32, and uint32 values and Double Value objects to
// represent int64 and uint64 values.
//
// We intend to move this to the chrome dbus library's MessageReader class when
// it's more fully baked.
//
// TODO(vlaviano):
// - Can we handle integral types better?
// - Add support for nested complex types.
// - Write an equivalent function to convert in the opposite direction.
// - Write unit tests.
bool PopArrayOfDictEntries(dbus::MessageReader* reader,
                           dbus::Message* message,
                           DictionaryValue* dictionary) {
  DCHECK(reader);
  DCHECK(message);
  DCHECK(dictionary);
  dbus::MessageReader array_reader(message);
  if (!reader->PopArray(&array_reader)) {
    return false;
  }
  while (array_reader.HasMoreData()) {
    dbus::MessageReader dict_entry_reader(message);
    if (!array_reader.PopDictEntry(&dict_entry_reader)) {
      return false;
    }
    std::string key;
    if (!dict_entry_reader.PopString(&key)) {
      return false;
    }
    dbus::MessageReader variant_reader(message);
    if (!dict_entry_reader.PopVariant(&variant_reader)) {
      return false;
    }
    const dbus::Message::DataType type = variant_reader.GetDataType();
    switch (type) {
      case dbus::Message::BYTE: {
        uint8 value = 0;
        if (!variant_reader.PopByte(&value)) {
          return false;
        }
        dictionary->SetInteger(key, value);
        break;
      }
      case dbus::Message::BOOL: {
        bool value = false;
        if (!variant_reader.PopBool(&value)) {
          return false;
        }
        dictionary->SetBoolean(key, value);
        break;
      }
      case dbus::Message::INT16: {
        int16 value = 0;
        if (!variant_reader.PopInt16(&value)) {
          return false;
        }
        dictionary->SetInteger(key, value);
        break;
      }
      case dbus::Message::UINT16: {
        uint16 value = 0;
        if (!variant_reader.PopUint16(&value)) {
          return false;
        }
        dictionary->SetInteger(key, value);
        break;
      }
      case dbus::Message::INT32: {
        int32 value = 0;
        if (!variant_reader.PopInt32(&value)) {
          return false;
        }
        dictionary->SetInteger(key, value);
        break;
      }
      case dbus::Message::UINT32: {
        uint32 value = 0;
        if (!variant_reader.PopUint32(&value)) {
          return false;
        }
        dictionary->SetInteger(key, value);
        break;
      }
      case dbus::Message::INT64: {
        int64 value = 0;
        if (!variant_reader.PopInt64(&value)) {
          return false;
        }
        dictionary->SetDouble(key, value);
        break;
      }
      case dbus::Message::UINT64: {
        uint64 value = 0;
        if (!variant_reader.PopUint64(&value)) {
          return false;
        }
        dictionary->SetDouble(key, value);
        break;
      }
      case dbus::Message::DOUBLE: {
        double value = 0;
        if (!variant_reader.PopDouble(&value)) {
          return false;
        }
        dictionary->SetDouble(key, value);
        break;
      }
      case dbus::Message::STRING: {
        std::string value;
        if (!variant_reader.PopString(&value)) {
          return false;
        }
        dictionary->SetString(key, value);
        break;
      }
      case dbus::Message::OBJECT_PATH: {
        dbus::ObjectPath value;
        if (!variant_reader.PopObjectPath(&value)) {
          return false;
        }
        dictionary->SetString(key, value.value());
        break;
      }
      case dbus::Message::ARRAY: {
        // Not yet supported.
        return false;
      }
      case dbus::Message::STRUCT: {
        // Not yet supported.
        return false;
      }
      case dbus::Message::DICT_ENTRY: {
        // Not yet supported.
        return false;
      }
      case dbus::Message::VARIANT: {
        // Not yet supported.
        return false;
      }
      default:
        LOG(FATAL) << "Unknown type: " << type;
    }
  }
  return true;
}

}  // namespace

namespace chromeos {

// The BluetoothAdapterClient implementation used in production.
class BluetoothAdapterClientImpl: public BluetoothAdapterClient,
                                  private BluetoothManagerClient::Observer {
 public:
  explicit BluetoothAdapterClientImpl(dbus::Bus* bus,
                                      BluetoothManagerClient* manager_client)
      : weak_ptr_factory_(this),
        bus_(bus) {
    VLOG(1) << "Creating BluetoothAdapterClientImpl";

    DCHECK(manager_client);
    manager_client->AddObserver(this);
  }

  virtual ~BluetoothAdapterClientImpl() {
  }

  // BluetoothAdapterClient override.
  virtual void AddObserver(BluetoothAdapterClient::Observer* observer) {
    VLOG(1) << "AddObserver";
    DCHECK(observer);
    observers_.AddObserver(observer);
  }

  // BluetoothAdapterClient override.
  virtual void RemoveObserver(BluetoothAdapterClient::Observer* observer) {
    VLOG(1) << "RemoveObserver";
    DCHECK(observer);
    observers_.RemoveObserver(observer);
  }

  // BluetoothAdapterClient override.
  virtual void StartDiscovery(const dbus::ObjectPath& object_path) {
    VLOG(1) << "StartDiscovery: " << object_path.value();

    dbus::MethodCall method_call(
        bluetooth_adapter::kBluetoothAdapterInterface,
        bluetooth_adapter::kStartDiscovery);

    dbus::ObjectProxy* adapter_proxy = GetObjectProxyForPath(object_path);

    adapter_proxy->CallMethod(
        &method_call,
        dbus::ObjectProxy::TIMEOUT_USE_DEFAULT,
        base::Bind(&BluetoothAdapterClientImpl::OnStartDiscovery,
                   weak_ptr_factory_.GetWeakPtr(), object_path));
  }

  // BluetoothAdapterClient override.
  virtual void StopDiscovery(const dbus::ObjectPath& object_path) {
    VLOG(1) << "StopDiscovery: " << object_path.value();

    dbus::MethodCall method_call(
        bluetooth_adapter::kBluetoothAdapterInterface,
        bluetooth_adapter::kStopDiscovery);

    dbus::ObjectProxy* adapter_proxy = GetObjectProxyForPath(object_path);

    adapter_proxy->CallMethod(
        &method_call,
        dbus::ObjectProxy::TIMEOUT_USE_DEFAULT,
        base::Bind(&BluetoothAdapterClientImpl::OnStopDiscovery,
                   weak_ptr_factory_.GetWeakPtr(), object_path));
  }

 private:
  // BluetoothManagerClient::Observer override.
  virtual void AdapterAdded(const dbus::ObjectPath& object_path) OVERRIDE {
    VLOG(1) << "AdapterAdded: " << object_path.value();
  }

  // BluetoothManagerClient::Observer override.
  virtual void AdapterRemoved(const dbus::ObjectPath& object_path) OVERRIDE {
    VLOG(1) << "AdapterRemoved: " << object_path.value();
    RemoveObjectProxyForPath(object_path);
  }

  // Ensures that we have a dbus object proxy for an adapter with dbus
  // object path |object_path|, and if not, creates it and stores it in
  // our |proxy_map_| map.
  dbus::ObjectProxy* GetObjectProxyForPath(
      const dbus::ObjectPath& object_path) {
    VLOG(1) << "GetObjectProxyForPath: " << object_path.value();

    ProxyMap::iterator it = proxy_map_.find(object_path);
    if (it != proxy_map_.end())
      return it->second;

    DCHECK(bus_);
    dbus::ObjectProxy* adapter_proxy = bus_->GetObjectProxy(
        bluetooth_adapter::kBluetoothAdapterServiceName, object_path);

    proxy_map_[object_path] = adapter_proxy;

    adapter_proxy->ConnectToSignal(
        bluetooth_adapter::kBluetoothAdapterInterface,
        bluetooth_adapter::kDeviceCreatedSignal,
        base::Bind(&BluetoothAdapterClientImpl::DeviceCreatedReceived,
                   weak_ptr_factory_.GetWeakPtr(), object_path),
        base::Bind(&BluetoothAdapterClientImpl::DeviceCreatedConnected,
                   weak_ptr_factory_.GetWeakPtr(), object_path));

    adapter_proxy->ConnectToSignal(
        bluetooth_adapter::kBluetoothAdapterInterface,
        bluetooth_adapter::kDeviceRemovedSignal,
        base::Bind(&BluetoothAdapterClientImpl::DeviceRemovedReceived,
                   weak_ptr_factory_.GetWeakPtr(), object_path),
        base::Bind(&BluetoothAdapterClientImpl::DeviceRemovedConnected,
                   weak_ptr_factory_.GetWeakPtr(), object_path));

    adapter_proxy->ConnectToSignal(
        bluetooth_adapter::kBluetoothAdapterInterface,
        bluetooth_adapter::kPropertyChangedSignal,
        base::Bind(&BluetoothAdapterClientImpl::PropertyChangedReceived,
                   weak_ptr_factory_.GetWeakPtr(), object_path),
        base::Bind(&BluetoothAdapterClientImpl::PropertyChangedConnected,
                   weak_ptr_factory_.GetWeakPtr(), object_path));

    adapter_proxy->ConnectToSignal(
        bluetooth_adapter::kBluetoothAdapterInterface,
        bluetooth_adapter::kDeviceFoundSignal,
        base::Bind(&BluetoothAdapterClientImpl::DeviceFoundReceived,
                   weak_ptr_factory_.GetWeakPtr(), object_path),
        base::Bind(&BluetoothAdapterClientImpl::DeviceFoundConnected,
                   weak_ptr_factory_.GetWeakPtr(), object_path));

    adapter_proxy->ConnectToSignal(
        bluetooth_adapter::kBluetoothAdapterInterface,
        bluetooth_adapter::kDeviceDisappearedSignal,
        base::Bind(&BluetoothAdapterClientImpl::DeviceDisappearedReceived,
                   weak_ptr_factory_.GetWeakPtr(), object_path),
        base::Bind(&BluetoothAdapterClientImpl::DeviceDisappearedConnected,
                   weak_ptr_factory_.GetWeakPtr(), object_path));

    return adapter_proxy;
  }

  // Removes the dbus object proxy for the adapter with dbus object path
  // |object_path| from our |proxy_map_| map.
  void RemoveObjectProxyForPath(const dbus::ObjectPath& object_path) {
    VLOG(1) << "RemoveObjectProxyForPath: " << object_path.value();
    proxy_map_.erase(object_path);
  }

  // Called by dbus:: when a DeviceCreated signal is received.
  void DeviceCreatedReceived(const dbus::ObjectPath& object_path,
                             dbus::Signal* signal) {
    DCHECK(signal);
    dbus::MessageReader reader(signal);
    dbus::ObjectPath device_path;
    if (!reader.PopObjectPath(&device_path)) {
      LOG(ERROR) << object_path.value()
                 << ": DeviceCreated signal has incorrect parameters: "
                 << signal->ToString();
      return;
    }
    VLOG(1) << object_path.value() << ": Device created: "
            << device_path.value();

    FOR_EACH_OBSERVER(BluetoothAdapterClient::Observer, observers_,
                      DeviceCreated(object_path, device_path));
  }

  // Called by dbus:: when the DeviceCreated signal is initially connected.
  void DeviceCreatedConnected(const dbus::ObjectPath& object_path,
                              const std::string& interface_name,
                              const std::string& signal_name,
                              bool success) {
    LOG_IF(WARNING, !success) << object_path.value()
                              << ": Failed to connect to DeviceCreated signal.";
  }

  // Called by dbus:: when a DeviceRemoved signal is received.
  void DeviceRemovedReceived(const dbus::ObjectPath& object_path,
                             dbus::Signal* signal) {
    DCHECK(signal);
    dbus::MessageReader reader(signal);
    dbus::ObjectPath device_path;
    if (!reader.PopObjectPath(&device_path)) {
      LOG(ERROR) << object_path.value()
                 << ": DeviceRemoved signal has incorrect parameters: "
                 << signal->ToString();
      return;
    }
    VLOG(1) << object_path.value() << ": Device removed: "
            << device_path.value();

    FOR_EACH_OBSERVER(BluetoothAdapterClient::Observer, observers_,
                      DeviceRemoved(object_path, device_path));
  }

  // Called by dbus:: when the DeviceRemoved signal is initially connected.
  void DeviceRemovedConnected(const dbus::ObjectPath& object_path,
                              const std::string& interface_name,
                              const std::string& signal_name,
                              bool success) {
    LOG_IF(WARNING, !success) << object_path.value()
                              << ": Failed to connect to DeviceRemoved signal.";
  }

  // Called by dbus:: when a PropertyChanged signal is received.
  void PropertyChangedReceived(const dbus::ObjectPath& object_path,
                               dbus::Signal* signal) {
    DCHECK(signal);
    dbus::MessageReader reader(signal);
    std::string property_name;
    if (!reader.PopString(&property_name)) {
      LOG(ERROR) << object_path.value()
                 << ": PropertyChanged signal has incorrect parameters: "
                 << signal->ToString();
      return;
    }

    if (property_name != bluetooth_adapter::kDiscoveringProperty) {
      VLOG(1) << object_path.value() << ": PropertyChanged: " << property_name;
      // We don't care.
      return;
    }

    bool discovering = false;
    if (!reader.PopVariantOfBool(&discovering)) {
      LOG(ERROR) << object_path.value()
                 << ": PropertyChanged signal has incorrect parameters: "
                 << signal->ToString();
      return;
    }
    VLOG(1) << object_path.value() << ": PropertyChanged: Discovering = "
            << discovering;

    FOR_EACH_OBSERVER(BluetoothAdapterClient::Observer, observers_,
                      DiscoveringPropertyChanged(object_path, discovering));
  }

  // Called by dbus:: when the PropertyChanged signal is initially connected.
  void PropertyChangedConnected(const dbus::ObjectPath& object_path,
                                const std::string& interface_name,
                                const std::string& signal_name,
                                bool success) {
    LOG_IF(WARNING, !success)
        << object_path.value()
        << ": Failed to connect to PropertyChanged signal.";
  }

  // Called by dbus:: when a DeviceFound signal is received.
  void DeviceFoundReceived(const dbus::ObjectPath& object_path,
                           dbus::Signal* signal) {
    DCHECK(signal);
    dbus::MessageReader reader(signal);
    std::string address;
    if (!reader.PopString(&address)) {
      LOG(ERROR) << object_path.value()
                 << ": DeviceFound signal has incorrect parameters: "
                 << signal->ToString();
      return;
    }
    VLOG(1) << object_path.value() << ": Device found: " << address;

    DictionaryValue device_properties;
    if (!PopArrayOfDictEntries(&reader, signal, &device_properties)) {
      LOG(ERROR) << object_path.value()
                 << ": DeviceFound signal has incorrect parameters: "
                 << signal->ToString();
      return;
    }

    FOR_EACH_OBSERVER(BluetoothAdapterClient::Observer, observers_,
                      DeviceFound(object_path, address, device_properties));
  }

  // Called by dbus:: when the DeviceFound signal is initially connected.
  void DeviceFoundConnected(const dbus::ObjectPath& object_path,
                            const std::string& interface_name,
                            const std::string& signal_name,
                            bool success) {
    LOG_IF(WARNING, !success) << object_path.value()
                              << ": Failed to connect to DeviceFound signal.";
  }

  // Called by dbus:: when a DeviceDisappeared signal is received.
  void DeviceDisappearedReceived(const dbus::ObjectPath& object_path,
                                 dbus::Signal* signal) {
    DCHECK(signal);
    dbus::MessageReader reader(signal);
    std::string address;
    if (!reader.PopString(&address)) {
      LOG(ERROR) << object_path.value()
                 << ": DeviceDisappeared signal has incorrect parameters: "
                 << signal->ToString();
      return;
    }
    VLOG(1) << object_path.value() << ": Device disappeared: " << address;
    FOR_EACH_OBSERVER(BluetoothAdapterClient::Observer, observers_,
                      DeviceDisappeared(object_path, address));
  }

  // Called by dbus:: when the DeviceDisappeared signal is initially connected.
  void DeviceDisappearedConnected(const dbus::ObjectPath& object_path,
                                  const std::string& interface_name,
                                  const std::string& signal_name,
                                  bool success) {
    LOG_IF(WARNING, !success)
        << object_path.value()
        << ": Failed to connect to DeviceDisappeared signal.";
  }

  // Called when a response for StartDiscovery() is received.
  void OnStartDiscovery(const dbus::ObjectPath& object_path,
                        dbus::Response* response) {
    VLOG(1) << "OnStartDiscovery: " << object_path.value();
    LOG_IF(WARNING, !response) << object_path.value()
                               << ": OnStartDiscovery: failed.";
  }

  // Called when a response for StopDiscovery() is received.
  void OnStopDiscovery(const dbus::ObjectPath& object_path,
                       dbus::Response* response) {
    VLOG(1) << "OnStopDiscovery: " << object_path.value();
    LOG_IF(WARNING, !response) << object_path.value()
                               << ": OnStopDiscovery: failed.";
  }

  // Weak pointer factory for generating 'this' pointers that might live longer
  // than we do.
  base::WeakPtrFactory<BluetoothAdapterClientImpl> weak_ptr_factory_;

  dbus::Bus* bus_;

  // We maintain a collection of dbus object proxies, one for each adapter.
  typedef std::map<const dbus::ObjectPath, dbus::ObjectProxy*> ProxyMap;
  ProxyMap proxy_map_;

  // List of observers interested in event notifications from us.
  ObserverList<BluetoothAdapterClient::Observer> observers_;

  DISALLOW_COPY_AND_ASSIGN(BluetoothAdapterClientImpl);
};

// The BluetoothAdapterClient implementation used on Linux desktop, which does
// nothing.
class BluetoothAdapterClientStubImpl : public BluetoothAdapterClient {
 public:
  // BluetoothAdapterClient override.
  virtual void AddObserver(Observer* observer) {
    VLOG(1) << "AddObserver";
  }

  // BluetoothAdapterClient override.
  virtual void RemoveObserver(Observer* observer) {
    VLOG(1) << "RemoveObserver";
  }

  // BluetoothAdapterClient override.
  virtual void StartDiscovery(const dbus::ObjectPath& object_path) {
    VLOG(1) << "StartDiscovery: " << object_path.value();
  }

  // BluetoothAdapterClient override.
  virtual void StopDiscovery(const dbus::ObjectPath& object_path) {
    VLOG(1) << "StopDiscovery: " << object_path.value();
  }
};

BluetoothAdapterClient::BluetoothAdapterClient() {
}

BluetoothAdapterClient::~BluetoothAdapterClient() {
}

BluetoothAdapterClient* BluetoothAdapterClient::Create(
    dbus::Bus* bus,
    BluetoothManagerClient* manager_client) {
  if (system::runtime_environment::IsRunningOnChromeOS()) {
    return new BluetoothAdapterClientImpl(bus, manager_client);
  } else {
    return new BluetoothAdapterClientStubImpl();
  }
}

}  // namespace chromeos
