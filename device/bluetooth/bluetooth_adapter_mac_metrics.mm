// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "device/bluetooth/bluetooth_adapter_mac_metrics.h"

#import <CoreBluetooth/CoreBluetooth.h>
#import <Foundation/Foundation.h>

#include "base/metrics/histogram_macros.h"

namespace {

#if !defined(MAC_OS_X_VERSION_10_11)
// Available with macOS 10.11 SDK or after.
const NSInteger CBErrorMaxConnection = 11;
#endif  // !defined(MAC_OS_X_VERSION_10_11)

WebBluetoothMacOSErrors GetWebBluetoothMacOSErrorsFromNSError(NSError* error) {
  NSString* error_domain = [error domain];
  NSInteger error_code = [error code];
  if ([error_domain isEqualToString:CBErrorDomain]) {
    CBError cb_error_code = static_cast<CBError>(error_code);
    switch (cb_error_code) {
      case CBErrorUnknown:
        return WebBluetoothMacOSErrors::CBERROR_UNKNOWN;
      case CBErrorInvalidParameters:
        return WebBluetoothMacOSErrors::CBERROR_INVALID_PARAMETERS;
      case CBErrorInvalidHandle:
        return WebBluetoothMacOSErrors::CBERROR_INVALID_HANDLE;
      case CBErrorNotConnected:
        return WebBluetoothMacOSErrors::CBERROR_NOT_CONNECTED;
      case CBErrorOutOfSpace:
        return WebBluetoothMacOSErrors::CBERROR_OUT_OF_SPACE;
      case CBErrorOperationCancelled:
        return WebBluetoothMacOSErrors::CBERROR_OPERATION_CANCELLED;
      case CBErrorConnectionTimeout:
        return WebBluetoothMacOSErrors::CBERROR_CONNECTION_TIMEOUT;
      case CBErrorPeripheralDisconnected:
        return WebBluetoothMacOSErrors::CBERROR_PERIPHERAL_DISCONNECTED;
      case CBErrorUUIDNotAllowed:
        return WebBluetoothMacOSErrors::CBERROR_UUID_NOT_ALLOWED;
      case CBErrorAlreadyAdvertising:
        return WebBluetoothMacOSErrors::CBERROR_ALREADY_ADVERTISING;
#if defined(MAC_OS_X_VERSION_10_11)
      case CBErrorMaxConnection:
        return WebBluetoothMacOSErrors::CBERROR_MAX_CONNECTION;
#endif  // defined(MAC_OS_X_VERSION_10_11)
    }
#if !defined(MAC_OS_X_VERSION_10_11)
    if (error_code == CBErrorMaxConnection) {
      return WebBluetoothMacOSErrors::CBERROR_MAX_CONNECTION;
    }
#endif  // !defined(MAC_OS_X_VERSION_10_11)
    return WebBluetoothMacOSErrors::CBERROR_MAX;
  } else if ([error_domain isEqualToString:CBATTErrorDomain]) {
    switch (static_cast<CBATTError>(error_code)) {
      case CBATTErrorSuccess:
        return WebBluetoothMacOSErrors::CBATT_ERROR_SUCCESS;
      case CBATTErrorInvalidHandle:
        return WebBluetoothMacOSErrors::CBATT_ERROR_INVALID_HANDLE;
      case CBATTErrorReadNotPermitted:
        return WebBluetoothMacOSErrors::CBATT_ERROR_READ_NOT_PERMITTED;
      case CBATTErrorWriteNotPermitted:
        return WebBluetoothMacOSErrors::CBATT_ERROR_WRITE_NOT_PERMITTED;
      case CBATTErrorInvalidPdu:
        return WebBluetoothMacOSErrors::CBATT_ERROR_INVALID_PDU;
      case CBATTErrorInsufficientAuthentication:
        return WebBluetoothMacOSErrors::CBATT_ERROR_INSUFFICIENT_AUTHENTICATION;
      case CBATTErrorRequestNotSupported:
        return WebBluetoothMacOSErrors::CBATT_ERROR_REQUEST_NOT_SUPPORTED;
      case CBATTErrorInvalidOffset:
        return WebBluetoothMacOSErrors::CBATT_ERROR_INVALID_OFFSET;
      case CBATTErrorInsufficientAuthorization:
        return WebBluetoothMacOSErrors::CBATT_ERROR_INSUFFICIENT_AUTHORIZATION;
      case CBATTErrorPrepareQueueFull:
        return WebBluetoothMacOSErrors::CBATT_ERROR_PREPARE_QUEUE_FULL;
      case CBATTErrorAttributeNotFound:
        return WebBluetoothMacOSErrors::CBATT_ERROR_ATTRIBUTE_NOT_FOUND;
      case CBATTErrorAttributeNotLong:
        return WebBluetoothMacOSErrors::CBATT_ERROR_ATTRIBUTE_NOT_LONG;
      case CBATTErrorInsufficientEncryptionKeySize:
        return WebBluetoothMacOSErrors::
            CBATT_ERROR_INSUFFICIENT_ENCRYPTION_KEY_SIZE;
      case CBATTErrorInvalidAttributeValueLength:
        return WebBluetoothMacOSErrors::
            CBATT_ERROR_INVALID_ATTRIBUTE_VALUE_LENGTH;
      case CBATTErrorUnlikelyError:
        return WebBluetoothMacOSErrors::CBATT_ERROR_UNLIKELY_ERROR;
      case CBATTErrorInsufficientEncryption:
        return WebBluetoothMacOSErrors::CBATT_ERROR_INSUFFICIENT_ENCRYPTION;
      case CBATTErrorUnsupportedGroupType:
        return WebBluetoothMacOSErrors::CBATT_ERROR_UNSUPPORTED_GROUP_TYPE;
      case CBATTErrorInsufficientResources:
        return WebBluetoothMacOSErrors::CBATT_ERROR_INSUFFICIENT_RESOURCES;
    }
    return WebBluetoothMacOSErrors::CBATT_ERROR_MAX;
  }
  return WebBluetoothMacOSErrors::UNKNOWN_ERROR_DOMAIN;
}

}  // namespace

void LogDidFailToConnectPeripheralErrorToHistogram(NSError* error) {
  if (!error)
    return;
  WebBluetoothMacOSErrors histogram_macos_error =
      GetMacOSOperationResultFromNSError(error);
  UMA_HISTOGRAM_ENUMERATION(
      "Bluetooth.Web.MacOS.Errors.DidFailToConnectToPeripheral",
      static_cast<int>(histogram_macos_error),
      static_cast<int>(WebBluetoothMacOSErrors::MAX));
}

void LogDidDisconnectPeripheralErrorToHistogram(NSError* error) {
  if (!error)
    return;
  WebBluetoothMacOSErrors histogram_macos_error =
      GetMacOSOperationResultFromNSError(error);
  UMA_HISTOGRAM_ENUMERATION(
      "Bluetooth.Web.MacOS.Errors.DidDisconnectPeripheral",
      static_cast<int>(histogram_macos_error),
      static_cast<int>(WebBluetoothMacOSErrors::MAX));
}

void LogDidDiscoverPrimaryServicesErrorToHistogram(NSError* error) {
  if (!error)
    return;
  WebBluetoothMacOSErrors histogram_macos_error =
      GetMacOSOperationResultFromNSError(error);
  UMA_HISTOGRAM_ENUMERATION(
      "Bluetooth.Web.MacOS.Errors.DidDiscoverPrimaryServices",
      static_cast<int>(histogram_macos_error),
      static_cast<int>(WebBluetoothMacOSErrors::MAX));
}

void LogDidDiscoverCharacteristicsErrorToHistogram(NSError* error) {
  if (!error)
    return;
  WebBluetoothMacOSErrors histogram_macos_error =
      GetMacOSOperationResultFromNSError(error);
  UMA_HISTOGRAM_ENUMERATION(
      "Bluetooth.Web.MacOS.Errors.DidDiscoverCharacteristics",
      static_cast<int>(histogram_macos_error),
      static_cast<int>(WebBluetoothMacOSErrors::MAX));
}

void LogDidUpdateValueErrorToHistogram(NSError* error) {
  if (!error)
    return;
  WebBluetoothMacOSErrors histogram_macos_error =
      GetMacOSOperationResultFromNSError(error);
  UMA_HISTOGRAM_ENUMERATION("Bluetooth.Web.MacOS.Errors.DidUpdateValue",
                            static_cast<int>(histogram_macos_error),
                            static_cast<int>(WebBluetoothMacOSErrors::MAX));
}

void LogDidWriteValueErrorToHistogram(NSError* error) {
  if (!error)
    return;
  WebBluetoothMacOSErrors histogram_macos_error =
      GetMacOSOperationResultFromNSError(error);
  UMA_HISTOGRAM_ENUMERATION("Bluetooth.Web.MacOS.Errors.DidWriteValue",
                            static_cast<int>(histogram_macos_error),
                            static_cast<int>(WebBluetoothMacOSErrors::MAX));
}

void LogDidUpdateNotificationStateErrorToHistogram(NSError* error) {
  if (!error)
    return;
  WebBluetoothMacOSErrors histogram_macos_error =
      GetMacOSOperationResultFromNSError(error);
  UMA_HISTOGRAM_ENUMERATION(
      "Bluetooth.Web.MacOS.Errors.DidUpdateNotificationState",
      static_cast<int>(histogram_macos_error),
      static_cast<int>(WebBluetoothMacOSErrors::MAX));
}

void LogDidDiscoverDescriptorsErrorToHistogram(NSError* error) {
  if (!error)
    return;
  WebBluetoothMacOSErrors histogram_macos_error =
      GetMacOSOperationResultFromNSError(error);
  UMA_HISTOGRAM_ENUMERATION("Bluetooth.Web.MacOS.Errors.DidDiscoverDescriptors",
                            static_cast<int>(histogram_macos_error),
                            static_cast<int>(WebBluetoothMacOSErrors::MAX));
}

void LogDidUpdateValueForDescriptorErrorToHistogram(NSError* error) {
  if (!error)
    return;
  WebBluetoothMacOSErrors histogram_macos_error =
      GetMacOSOperationResultFromNSError(error);
  UMA_HISTOGRAM_ENUMERATION(
      "Bluetooth.Web.MacOS.Errors.DidUpdateValueForDescriptor",
      static_cast<int>(histogram_macos_error),
      static_cast<int>(WebBluetoothMacOSErrors::MAX));
}

void LogDidWriteValueForDescriptorErrorToHistogram(NSError* error) {
  if (!error)
    return;
  WebBluetoothMacOSErrors histogram_macos_error =
      GetMacOSOperationResultFromNSError(error);
  UMA_HISTOGRAM_ENUMERATION(
      "Bluetooth.Web.MacOS.Errors.DidWriteValueForDescriptor",
      static_cast<int>(histogram_macos_error),
      static_cast<int>(WebBluetoothMacOSErrors::MAX));
}
