// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef DEVICE_BLUETOOTH_BLUETOOTH_ADAPTER_H_
#define DEVICE_BLUETOOTH_BLUETOOTH_ADAPTER_H_

#include <stdint.h>

#include <list>
#include <memory>
#include <set>
#include <string>
#include <unordered_map>
#include <utility>
#include <vector>

#include "base/callback.h"
#include "base/memory/ref_counted.h"
#include "base/memory/weak_ptr.h"
#include "base/time/time.h"
#include "build/build_config.h"
#include "device/bluetooth/bluetooth_advertisement.h"
#include "device/bluetooth/bluetooth_device.h"
#include "device/bluetooth/bluetooth_export.h"

namespace device {

class BluetoothAdvertisement;
class BluetoothDiscoveryFilter;
class BluetoothDiscoverySession;
class BluetoothLocalGattService;
class BluetoothRemoteGattCharacteristic;
class BluetoothRemoteGattDescriptor;
class BluetoothRemoteGattService;
class BluetoothSocket;
class BluetoothUUID;
enum class UMABluetoothDiscoverySessionOutcome;

// BluetoothAdapter represents a local Bluetooth adapter which may be used to
// interact with remote Bluetooth devices. As well as providing support for
// determining whether an adapter is present and whether the radio is powered,
// this class also provides support for obtaining the list of remote devices
// known to the adapter, discovering new devices, and providing notification of
// updates to device information.
class DEVICE_BLUETOOTH_EXPORT BluetoothAdapter
    : public base::RefCounted<BluetoothAdapter> {
 public:
  // Interface for observing changes from bluetooth adapters.
  class DEVICE_BLUETOOTH_EXPORT Observer {
   public:
    virtual ~Observer() {}

    // Called when the presence of the adapter |adapter| changes. When |present|
    // is true the adapter is now present, false means the adapter has been
    // removed from the system.
    virtual void AdapterPresentChanged(BluetoothAdapter* adapter,
                                       bool present) {}

    // Called when the radio power state of the adapter |adapter| changes. When
    // |powered| is true the adapter radio is powered, false means the adapter
    // radio is off.
    virtual void AdapterPoweredChanged(BluetoothAdapter* adapter,
                                       bool powered) {}

    // Called when the discoverability state of the  adapter |adapter| changes.
    // When |discoverable| is true the adapter is discoverable by other devices,
    // false means the adapter is not discoverable.
    virtual void AdapterDiscoverableChanged(BluetoothAdapter* adapter,
                                           bool discoverable) {}

    // Called when the discovering state of the adapter |adapter| changes. When
    // |discovering| is true the adapter is seeking new devices, false means it
    // is not.
    virtual void AdapterDiscoveringChanged(BluetoothAdapter* adapter,
                                           bool discovering) {}

    // Called when a new device |device| is added to the adapter |adapter|,
    // either because it has been discovered or a connection made. |device|
    // should not be cached. Instead, copy its Bluetooth address.
    virtual void DeviceAdded(BluetoothAdapter* adapter,
                             BluetoothDevice* device) {}

    // Called when the result of one of the following methods of the device
    // |device| changes:
    //  * GetAddress()
    //  * GetAppearance()
    //  * GetBluetoothClass()
    //  * GetInquiryRSSI()
    //  * GetInquiryTxPower()
    //  * GetUUIDs()
    //  * GetServiceData()
    //  * GetServiceDataUUIDs()
    //  * GetServiceDataForUUID()
    //  * GetManufacturerData()
    //  * GetManufacturerDataIDs()
    //  * GetManufacturerDataForID()
    //  * GetAdvertisingDataFlags()
    //  * IsConnectable()
    //  * IsConnected()
    //  * IsConnecting()
    //  * IsGattConnected()
    //  * IsPaired()
    //
    // On Android and MacOS this method is called for each advertisement packet
    // received. On Chrome OS and Linux, we can't guarantee that this method
    // will be called for each Adv. Packet received but, because the RSSI is
    // always changing, it's very likely this method will be called for each
    // Adv. Packet.
    // |device| should not be cached. Instead, copy its Bluetooth address.
    virtual void DeviceChanged(BluetoothAdapter* adapter,
                               BluetoothDevice* device) {}

    // Called when address property of the device |device| known to the adapter
    // |adapter| change due to pairing.
    virtual void DeviceAddressChanged(BluetoothAdapter* adapter,
                                      BluetoothDevice* device,
                                      const std::string& old_address) {}

// TODO(crbug.com/732991): Update comment and fix redundant #ifs throughout.
#if defined(OS_CHROMEOS) || defined(OS_LINUX)
    // This function is implemented for ChromeOS only, and the support for
    // Android, MaxOS and Windows should be added on demand in the future.
    // Called when paired property of the device |device| known to the adapter
    // |adapter| changed.
    virtual void DevicePairedChanged(BluetoothAdapter* adapter,
                                     BluetoothDevice* device,
                                     bool new_paired_status) {}
#endif

    // Called when the device |device| is removed from the adapter |adapter|,
    // either as a result of a discovered device being lost between discovering
    // phases or pairing information deleted. |device| should not be
    // cached. Instead, copy its Bluetooth address.
    virtual void DeviceRemoved(BluetoothAdapter* adapter,
                               BluetoothDevice* device) {}

    // Deprecated GATT Added/Removed Events NOTE:
    //
    // The series of Observer methods for Service, Characteristic, & Descriptor
    // Added/Removed events should be removed.  They are rarely used and add
    // API & implementation complexity.  They are not reliable for cross
    // platform use, and devices that modify their attribute table have not been
    // tested or supported.
    //
    // New code should use Observer::GattServicesDiscovered and then call
    //   GetGattService(s)
    //   GetCharacteristic(s)
    //   GetDescriptor(s)
    //
    // TODO(710352): Remove Service, Characteristic, & Descriptor Added/Removed.

    // See "Deprecated GATT Added/Removed Events NOTE" above.
    //
    // Called when a new GATT service |service| is added to the device |device|,
    // as the service is received from the device. Don't cache |service|. Store
    // its identifier instead (i.e. BluetoothRemoteGattService::GetIdentifier).
    virtual void GattServiceAdded(BluetoothAdapter* adapter,
                                  BluetoothDevice* device,
                                  BluetoothRemoteGattService* service) {}

    // See "Deprecated GATT Added/Removed Events NOTE" above.
    //
    // Called when the GATT service |service| is removed from the device
    // |device|. This can happen if the attribute database of the remote device
    // changes or when |device| gets removed.
    virtual void GattServiceRemoved(BluetoothAdapter* adapter,
                                    BluetoothDevice* device,
                                    BluetoothRemoteGattService* service) {}

    // Called when the GATT discovery process has completed for all services,
    // characteristics, and descriptors in |device|.
    virtual void GattServicesDiscovered(BluetoothAdapter* adapter,
                                        BluetoothDevice* device) {}

    // TODO(782494): Deprecated & not functional on all platforms. Use
    // GattServicesDiscovered.
    //
    // Called when all characteristic and descriptor discovery procedures are
    // known to be completed for the GATT service |service|. This method will be
    // called after the initial discovery of a GATT service and will usually be
    // preceded by calls to GattCharacteristicAdded and GattDescriptorAdded.
    virtual void GattDiscoveryCompleteForService(
        BluetoothAdapter* adapter,
        BluetoothRemoteGattService* service) {}

    // See "Deprecated GATT Added/Removed Events NOTE" above.
    //
    // Called when properties of the remote GATT service |service| have changed.
    // This will get called for properties such as UUID, as well as for changes
    // to the list of known characteristics and included services. Observers
    // should read all GATT characteristic and descriptors objects and do any
    // necessary set up required for a changed service.
    virtual void GattServiceChanged(BluetoothAdapter* adapter,
                                    BluetoothRemoteGattService* service) {}

    // See "Deprecated GATT Added/Removed Events NOTE" above.
    //
    // Called when the remote GATT characteristic |characteristic| has been
    // discovered. Use this to issue any initial read/write requests to the
    // characteristic but don't cache the pointer as it may become invalid.
    // Instead, use the specially assigned identifier to obtain a characteristic
    // and cache that identifier as necessary, as it can be used to retrieve the
    // characteristic from its GATT service. The number of characteristics with
    // the same UUID belonging to a service depends on the particular profile
    // the remote device implements, hence the client of a GATT based profile
    // will usually operate on the whole set of characteristics and not just
    // one.
    virtual void GattCharacteristicAdded(
        BluetoothAdapter* adapter,
        BluetoothRemoteGattCharacteristic* characteristic) {}

    // See "Deprecated GATT Added/Removed Events NOTE" above.
    //
    // Called when a GATT characteristic |characteristic| has been removed from
    // the system.
    virtual void GattCharacteristicRemoved(
        BluetoothAdapter* adapter,
        BluetoothRemoteGattCharacteristic* characteristic) {}

    // See "Deprecated GATT Added/Removed Events NOTE" above.
    //
    // Called when the remote GATT characteristic descriptor |descriptor| has
    // been discovered. Don't cache the arguments as the pointers may become
    // invalid. Instead, use the specially assigned identifier to obtain a
    // descriptor and cache that identifier as necessary.
    virtual void GattDescriptorAdded(
        BluetoothAdapter* adapter,
        BluetoothRemoteGattDescriptor* descriptor) {}

    // See "Deprecated GATT Added/Removed Events NOTE" above.
    //
    // Called when a GATT characteristic descriptor |descriptor| has been
    // removed from the system.
    virtual void GattDescriptorRemoved(
        BluetoothAdapter* adapter,
        BluetoothRemoteGattDescriptor* descriptor) {}

    // Called when the value of a characteristic has changed. This might be a
    // result of a read/write request to, or a notification/indication from, a
    // remote GATT characteristic.
    virtual void GattCharacteristicValueChanged(
        BluetoothAdapter* adapter,
        BluetoothRemoteGattCharacteristic* characteristic,
        const std::vector<uint8_t>& value) {}

    // Called when the value of a characteristic descriptor has been updated.
    virtual void GattDescriptorValueChanged(
        BluetoothAdapter* adapter,
        BluetoothRemoteGattDescriptor* descriptor,
        const std::vector<uint8_t>& value) {}
  };

  // Used to configure a listening servie.
  struct DEVICE_BLUETOOTH_EXPORT ServiceOptions {
    ServiceOptions();
    ~ServiceOptions();

    std::unique_ptr<int> channel;
    std::unique_ptr<int> psm;
    std::unique_ptr<std::string> name;
  };

  // The ErrorCallback is used for methods that can fail in which case it is
  // called, in the success case the callback is simply not called.
  using ErrorCallback = base::Closure;

  // The InitCallback is used to trigger a callback after asynchronous
  // initialization, if initialization is asynchronous on the platform.
  using InitCallback = base::Callback<void()>;

  using DiscoverySessionCallback =
      base::Callback<void(std::unique_ptr<BluetoothDiscoverySession>)>;
  using DeviceList = std::vector<BluetoothDevice*>;
  using ConstDeviceList = std::vector<const BluetoothDevice*>;
  using UUIDList = std::vector<BluetoothUUID>;
  using CreateServiceCallback =
      base::Callback<void(scoped_refptr<BluetoothSocket>)>;
  using CreateServiceErrorCallback =
      base::Callback<void(const std::string& message)>;
  using CreateAdvertisementCallback =
      base::Callback<void(scoped_refptr<BluetoothAdvertisement>)>;
  using AdvertisementErrorCallback =
      base::Callback<void(BluetoothAdvertisement::ErrorCode)>;

  // Returns a weak pointer to a new adapter.  For platforms with asynchronous
  // initialization, the returned adapter will run the |init_callback| once
  // asynchronous initialization is complete.
  // Caution: The returned pointer also transfers ownership of the adapter.  The
  // caller is expected to call |AddRef()| on the returned pointer, typically by
  // storing it into a |scoped_refptr|.
  static base::WeakPtr<BluetoothAdapter> CreateAdapter(
      const InitCallback& init_callback);

  // Returns a weak pointer to an existing adapter for testing purposes only.
  base::WeakPtr<BluetoothAdapter> GetWeakPtrForTesting();

#if defined(OS_LINUX)
  // Shutdown the adapter: tear down and clean up all objects owned by
  // BluetoothAdapter. After this call, the BluetoothAdapter will behave as if
  // no Bluetooth controller exists in the local system. |IsPresent| will return
  // false.
  virtual void Shutdown();
#endif

  // Adds and removes observers for events on this bluetooth adapter. If
  // monitoring multiple adapters, check the |adapter| parameter of observer
  // methods to determine which adapter is issuing the event.
  virtual void AddObserver(BluetoothAdapter::Observer* observer);
  virtual void RemoveObserver(BluetoothAdapter::Observer* observer);
  virtual bool HasObserver(BluetoothAdapter::Observer* observer);

  // The address of this adapter. The address format is "XX:XX:XX:XX:XX:XX",
  // where each XX is a hexadecimal number.
  virtual std::string GetAddress() const = 0;

  // The name of the adapter.
  virtual std::string GetName() const = 0;

  // Set the human-readable name of the adapter to |name|. On success,
  // |callback| will be called. On failure, |error_callback| will be called.
  virtual void SetName(const std::string& name,
                       const base::Closure& callback,
                       const ErrorCallback& error_callback) = 0;

  // Indicates whether the adapter is initialized and ready to use.
  virtual bool IsInitialized() const = 0;

  // Indicates whether the adapter is actually present on the system. For the
  // default adapter, this indicates whether any adapter is present. An adapter
  // is only considered present if the address has been obtained.
  virtual bool IsPresent() const = 0;

  // Indicates whether the adapter radio is powered.
  virtual bool IsPowered() const = 0;

  // Requests a change to the adapter radio power. Setting |powered| to true
  // will turn on the radio and false will turn it off. On success, |callback|
  // will be called. On failure, |error_callback| will be called.
  // On macOS this is only supported if low energy is available.
  virtual void SetPowered(bool powered,
                          const base::Closure& callback,
                          const ErrorCallback& error_callback) = 0;

  // Indicates whether the adapter radio is discoverable.
  virtual bool IsDiscoverable() const = 0;

  // Requests that the adapter change its discoverability state. If
  // |discoverable| is true, then it will be discoverable by other Bluetooth
  // devices. On successfully changing the adapter's discoverability, |callback|
  // will be called. On failure, |error_callback| will be called.
  virtual void SetDiscoverable(bool discoverable,
                               const base::Closure& callback,
                               const ErrorCallback& error_callback) = 0;

  // Indicates whether the adapter is currently discovering new devices.
  virtual bool IsDiscovering() const = 0;

  // Inserts all the devices that are connected by the operating system, and not
  // being connected by Chromium, into |devices_|. This method is useful since
  // a discovery session cannot find devices that are already connected to the
  // computer.
  // TODO(crbug.com/653032): Needs to be implemented for Android, ChromeOS and
  // Windows.
  virtual std::unordered_map<BluetoothDevice*, BluetoothDevice::UUIDSet>
  RetrieveGattConnectedDevicesWithDiscoveryFilter(
      const BluetoothDiscoveryFilter& discovery_filter);

  // Requests the adapter to start a new discovery session. On success, a new
  // instance of BluetoothDiscoverySession will be returned to the caller via
  // |callback| and the adapter will be discovering nearby Bluetooth devices.
  // The returned BluetoothDiscoverySession is owned by the caller and it's the
  // owner's responsibility to properly clean it up and stop the session when
  // device discovery is no longer needed.
  //
  // If clients desire device discovery to run, they should always call this
  // method and never make it conditional on the value of IsDiscovering(), as
  // another client might cause discovery to stop unexpectedly. Hence, clients
  // should always obtain a BluetoothDiscoverySession and call
  // BluetoothDiscoverySession::Stop when done. When this method gets called,
  // device discovery may actually be in progress. Clients can call GetDevices()
  // and check for those with IsPaired() as false to obtain the list of devices
  // that have been discovered so far. Otherwise, clients can be notified of all
  // new and lost devices by implementing the Observer methods "DeviceAdded" and
  // "DeviceRemoved".
  virtual void StartDiscoverySession(const DiscoverySessionCallback& callback,
                                     const ErrorCallback& error_callback);
  virtual void StartDiscoverySessionWithFilter(
      std::unique_ptr<BluetoothDiscoveryFilter> discovery_filter,
      const DiscoverySessionCallback& callback,
      const ErrorCallback& error_callback);

  // Return all discovery filters assigned to this adapter merged together.
  std::unique_ptr<BluetoothDiscoveryFilter> GetMergedDiscoveryFilter() const;

  // Works like GetMergedDiscoveryFilter, but doesn't take |masked_filter| into
  // account. |masked_filter| is compared by pointer, and must be a member of
  // active session.
  std::unique_ptr<BluetoothDiscoveryFilter> GetMergedDiscoveryFilterMasked(
      BluetoothDiscoveryFilter* masked_filter) const;

  // Requests the list of devices from the adapter. All devices are returned,
  // including those currently connected, those paired and all devices returned
  // by RetrieveGattConnectedDevicesWithDiscoveryFilter() (from the previous
  // call). Use the returned device pointers to determine which they are.
  virtual DeviceList GetDevices();
  virtual ConstDeviceList GetDevices() const;

  // Returns a pointer to the device with the given address |address| or NULL if
  // no such device is known.
  virtual BluetoothDevice* GetDevice(const std::string& address);
  virtual const BluetoothDevice* GetDevice(const std::string& address) const;

  // Returns a list of UUIDs for services registered on this adapter.
  // This may include UUIDs from standard profiles (e.g. A2DP) as well
  // as those created by CreateRfcommService and CreateL2capService.
  virtual UUIDList GetUUIDs() const = 0;

  // Possible priorities for AddPairingDelegate(), low is intended for
  // permanent UI and high is intended for interactive UI or applications.
  enum PairingDelegatePriority {
    PAIRING_DELEGATE_PRIORITY_LOW,
    PAIRING_DELEGATE_PRIORITY_HIGH
  };

  // Adds a default pairing delegate with priority |priority|. Method calls
  // will be made on |pairing_delegate| for incoming pairing requests if the
  // priority is higher than any other registered; or for those of the same
  // priority, the first registered.
  //
  // |pairing_delegate| must not be freed without first calling
  // RemovePairingDelegate().
  virtual void AddPairingDelegate(
      BluetoothDevice::PairingDelegate* pairing_delegate,
      PairingDelegatePriority priority);

  // Removes a previously added pairing delegate.
  virtual void RemovePairingDelegate(
      BluetoothDevice::PairingDelegate* pairing_delegate);

  // Returns the first registered pairing delegate with the highest priority,
  // or NULL if no delegate is registered. Used to select the delegate for
  // incoming pairing requests.
  virtual BluetoothDevice::PairingDelegate* DefaultPairingDelegate();

  // Creates an RFCOMM service on this adapter advertised with UUID |uuid|,
  // listening on channel |options.channel|, which may be left null to
  // automatically allocate one. The service will be advertised with
  // |options.name| as the English name of the service. |callback| will be
  // called on success with a BluetoothSocket instance that is to be owned by
  // the received.  |error_callback| will be called on failure with a message
  // indicating the cause.
  virtual void CreateRfcommService(
      const BluetoothUUID& uuid,
      const ServiceOptions& options,
      const CreateServiceCallback& callback,
      const CreateServiceErrorCallback& error_callback) = 0;

  // Creates an L2CAP service on this adapter advertised with UUID |uuid|,
  // listening on PSM |options.psm|, which may be left null to automatically
  // allocate one. The service will be advertised with |options.name| as the
  // English name of the service. |callback| will be called on success with a
  // BluetoothSocket instance that is to be owned by the received.
  // |error_callback| will be called on failure with a message indicating the
  // cause.
  virtual void CreateL2capService(
      const BluetoothUUID& uuid,
      const ServiceOptions& options,
      const CreateServiceCallback& callback,
      const CreateServiceErrorCallback& error_callback) = 0;

  // Creates and registers an advertisement for broadcast over the LE channel.
  // The created advertisement will be returned via the success callback. An
  // advertisement can unregister itself at any time by calling its unregister
  // function.
  virtual void RegisterAdvertisement(
      std::unique_ptr<BluetoothAdvertisement::Data> advertisement_data,
      const CreateAdvertisementCallback& callback,
      const AdvertisementErrorCallback& error_callback) = 0;

#if defined(OS_LINUX)
  // Sets the interval between two consecutive advertisements. Valid ranges
  // for the interval are from 20ms to 10.24 seconds, with min <= max.
  // Note: This is a best effort. The actual interval may vary non-trivially
  // from the requested intervals. On some hardware, there is a minimum
  // interval of 100ms. The minimum and maximum values are specified by the
  // Core 4.2 Spec, Vol 2, Part E, Section 7.8.5.
  virtual void SetAdvertisingInterval(
      const base::TimeDelta& min,
      const base::TimeDelta& max,
      const base::Closure& callback,
      const AdvertisementErrorCallback& error_callback) = 0;

  // Resets advertising on this adapter. This will unregister all existing
  // advertisements and will stop advertising them.
  virtual void ResetAdvertising(
      const base::Closure& callback,
      const AdvertisementErrorCallback& error_callback) = 0;
#endif

  // Returns the local GATT services associated with this adapter with the
  // given identifier. Returns NULL if the service doesn't exist.
  virtual BluetoothLocalGattService* GetGattService(
      const std::string& identifier) const = 0;

  // The following methods are used to send various events to observers.
  void NotifyAdapterPoweredChanged(bool powered);
  void NotifyDeviceChanged(BluetoothDevice* device);

#if defined(OS_CHROMEOS) || defined(OS_LINUX)
  // This function is implemented for ChromeOS only, and the support on
  // Android, MaxOS and Windows should be added on demand in the future.
  void NotifyDevicePairedChanged(BluetoothDevice* device,
                                 bool new_paired_status);
#endif
  void NotifyGattServiceAdded(BluetoothRemoteGattService* service);
  void NotifyGattServiceRemoved(BluetoothRemoteGattService* service);
  void NotifyGattServiceChanged(BluetoothRemoteGattService* service);
  void NotifyGattServicesDiscovered(BluetoothDevice* device);
  void NotifyGattDiscoveryComplete(BluetoothRemoteGattService* service);
  void NotifyGattCharacteristicAdded(
      BluetoothRemoteGattCharacteristic* characteristic);
  void NotifyGattCharacteristicRemoved(
      BluetoothRemoteGattCharacteristic* characteristic);
  void NotifyGattDescriptorAdded(BluetoothRemoteGattDescriptor* descriptor);
  void NotifyGattDescriptorRemoved(BluetoothRemoteGattDescriptor* descriptor);
  void NotifyGattCharacteristicValueChanged(
      BluetoothRemoteGattCharacteristic* characteristic,
      const std::vector<uint8_t>& value);
  void NotifyGattDescriptorValueChanged(
      BluetoothRemoteGattDescriptor* descriptor,
      const std::vector<uint8_t>& value);

  // The timeout in seconds used by RemoveTimedOutDevices.
  static const base::TimeDelta timeoutSec;

 protected:
  friend class base::RefCounted<BluetoothAdapter>;
  friend class BluetoothDiscoverySession;
  friend class BluetoothTestBase;

  using DevicesMap =
      std::unordered_map<std::string, std::unique_ptr<BluetoothDevice>>;
  using PairingDelegatePair =
      std::pair<BluetoothDevice::PairingDelegate*, PairingDelegatePriority>;
  using DiscoverySessionErrorCallback =
      base::Callback<void(UMABluetoothDiscoverySessionOutcome)>;

  BluetoothAdapter();
  virtual ~BluetoothAdapter();

  // Internal methods for initiating and terminating device discovery sessions.
  // An implementation of BluetoothAdapter keeps an internal reference count to
  // make sure that the underlying controller is constantly searching for nearby
  // devices and retrieving information from them as long as there are clients
  // who have requested discovery. These methods behave in the following way:
  //
  // On a call to AddDiscoverySession:
  //    - If there is a pending request to the subsystem, queue this request to
  //      execute once the pending requests are done.
  //    - If the count is 0, issue a request to the subsystem to start
  //      device discovery. On success, increment the count to 1.
  //    - If the count is greater than 0, increment the count and return
  //      success.
  //    As long as the count is non-zero, the underlying controller will be
  //    discovering for devices. This means that Chrome will restart device
  //    scan and inquiry sessions if they ever end, unless these sessions
  //    terminate due to an unexpected reason.
  //
  // On a call to RemoveDiscoverySession:
  //    - If there is a pending request to the subsystem, queue this request to
  //      execute once the pending requests are done.
  //    - If the count is 0, return failure, as there is no active discovery
  //      session.
  //    - If the count is 1, issue a request to the subsystem to stop device
  //      discovery and decrement the count to 0 on success.
  //    - If the count is greater than 1, decrement the count and return
  //      success.
  //
  // |discovery_filter| passed to AddDiscoverySession and RemoveDiscoverySession
  // is owned by other objects and shall not be freed.  When the count is
  // greater than 0 and AddDiscoverySession or RemoveDiscoverySession is called
  // the filter being used by the underlying controller must be updated.
  //
  // These methods invoke |callback| for success and |error_callback| for
  // failures.
  virtual void AddDiscoverySession(
      BluetoothDiscoveryFilter* discovery_filter,
      const base::Closure& callback,
      const DiscoverySessionErrorCallback& error_callback) = 0;
  virtual void RemoveDiscoverySession(
      BluetoothDiscoveryFilter* discovery_filter,
      const base::Closure& callback,
      const DiscoverySessionErrorCallback& error_callback) = 0;

  // Used to set and update the discovery filter used by the underlying
  // Bluetooth controller.
  virtual void SetDiscoveryFilter(
      std::unique_ptr<BluetoothDiscoveryFilter> discovery_filter,
      const base::Closure& callback,
      const DiscoverySessionErrorCallback& error_callback) = 0;

  // Called by RemovePairingDelegate() in order to perform any class-specific
  // internal functionality necessary to remove the pairing delegate, such as
  // cleaning up ongoing pairings using it.
  virtual void RemovePairingDelegateInternal(
      BluetoothDevice::PairingDelegate* pairing_delegate) = 0;

  // Success callback passed to AddDiscoverySession by StartDiscoverySession.
  void OnStartDiscoverySession(
      std::unique_ptr<BluetoothDiscoveryFilter> discovery_filter,
      const DiscoverySessionCallback& callback);

  // Error callback passed to AddDiscoverySession by StartDiscoverySession.
  void OnStartDiscoverySessionError(
      const ErrorCallback& callback,
      UMABluetoothDiscoverySessionOutcome outcome);

  // Marks all known DiscoverySession instances as inactive. Called by
  // BluetoothAdapter in the event that the adapter unexpectedly stops
  // discovering. This should be called by all platform implementations.
  void MarkDiscoverySessionsAsInactive();

  // Removes |discovery_session| from |discovery_sessions_|, if its in there.
  // Called by DiscoverySession when an instance is destroyed or becomes
  // inactive.
  void DiscoverySessionBecameInactive(
      BluetoothDiscoverySession* discovery_session);

  void DeleteDeviceForTesting(const std::string& address);

  // Removes from |devices_| any previously paired, connected or seen
  // devices which are no longer present. Notifies observers. Note:
  // this is only used by platforms where there is no notification of
  // lost devices.
  void RemoveTimedOutDevices();

  // Observers of BluetoothAdapter, notified from implementation subclasses.
  base::ObserverList<device::BluetoothAdapter::Observer> observers_;

  // Devices paired with, connected to, discovered by, or visible to the
  // adapter. The key is the Bluetooth address of the device and the value is
  // the BluetoothDevice object whose lifetime is managed by the adapter
  // instance.
  DevicesMap devices_;

  // Default pairing delegates registered with the adapter.
  std::list<PairingDelegatePair> pairing_delegates_;

 private:
  // Histograms the result of StartDiscoverySession.
  static void RecordBluetoothDiscoverySessionStartOutcome(
      UMABluetoothDiscoverySessionOutcome outcome);

  // Histograms the result of BluetoothDiscoverySession::Stop.
  static void RecordBluetoothDiscoverySessionStopOutcome(
      UMABluetoothDiscoverySessionOutcome outcome);

  // Return all discovery filters assigned to this adapter merged together.
  // If |omit| is true, |discovery_filter| will not be processed.
  std::unique_ptr<BluetoothDiscoveryFilter> GetMergedDiscoveryFilterHelper(
      const BluetoothDiscoveryFilter* discovery_filter,
      bool omit) const;

  // List of active DiscoverySession objects. This is used to notify sessions to
  // become inactive in case of an unexpected change to the adapter discovery
  // state. We keep raw pointers, with the invariant that a DiscoverySession
  // will remove itself from this list when it gets destroyed or becomes
  // inactive by calling DiscoverySessionBecameInactive(), hence no pointers to
  // deallocated sessions are kept.
  std::set<BluetoothDiscoverySession*> discovery_sessions_;

  // Note: This should remain the last member so it'll be destroyed and
  // invalidate its weak pointers before any other members are destroyed.
  base::WeakPtrFactory<BluetoothAdapter> weak_ptr_factory_;
};

}  // namespace device

#endif  // DEVICE_BLUETOOTH_BLUETOOTH_ADAPTER_H_
