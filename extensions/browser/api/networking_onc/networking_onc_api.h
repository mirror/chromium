// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EXTENSIONS_BROWSER_API_NETWORKING_ONC_NETWORKING_ONC_API_H_
#define EXTENSIONS_BROWSER_API_NETWORKING_ONC_NETWORKING_ONC_API_H_

#include <memory>
#include <string>

#include "base/macros.h"
#include "base/values.h"
#include "extensions/browser/extension_function.h"

namespace extensions {

namespace networking_onc {

extern const char kErrorAccessToSharedConfig[];
extern const char kErrorInvalidArguments[];
extern const char kErrorInvalidNetworkGuid[];
extern const char kErrorInvalidNetworkOperation[];
extern const char kErrorNetworkUnavailable[];
extern const char kErrorNotReady[];
extern const char kErrorNotSupported[];
extern const char kErrorPolicyControlled[];
extern const char kErrorSimLocked[];

}  // namespace networking_onc

// Implements the chrome.networkingOnc.getProperties method.
class NetworkingOncGetPropertiesFunction : public UIThreadExtensionFunction {
 public:
  NetworkingOncGetPropertiesFunction() {}
  DECLARE_EXTENSION_FUNCTION("networkingOnc.getProperties",
                             NETWORKINGPRIVATE_GETPROPERTIES);

 protected:
  ~NetworkingOncGetPropertiesFunction() override;

  // ExtensionFunction:
  ResponseAction Run() override;

 private:
  void Success(std::unique_ptr<base::DictionaryValue> result);
  void Failure(const std::string& error_name);

  DISALLOW_COPY_AND_ASSIGN(NetworkingOncGetPropertiesFunction);
};

// Implements the chrome.networkingOnc.getManagedProperties method.
class NetworkingOncGetManagedPropertiesFunction
    : public UIThreadExtensionFunction {
 public:
  NetworkingOncGetManagedPropertiesFunction() {}
  DECLARE_EXTENSION_FUNCTION("networkingOnc.getManagedProperties",
                             NETWORKINGPRIVATE_GETMANAGEDPROPERTIES);

 protected:
  ~NetworkingOncGetManagedPropertiesFunction() override;

  // ExtensionFunction:
  ResponseAction Run() override;

 private:
  void Success(std::unique_ptr<base::DictionaryValue> result);
  void Failure(const std::string& error);

  DISALLOW_COPY_AND_ASSIGN(NetworkingOncGetManagedPropertiesFunction);
};

// Implements the chrome.networkingOnc.getState method.
class NetworkingOncGetStateFunction : public UIThreadExtensionFunction {
 public:
  NetworkingOncGetStateFunction() {}
  DECLARE_EXTENSION_FUNCTION("networkingOnc.getState",
                             NETWORKINGPRIVATE_GETSTATE);

 protected:
  ~NetworkingOncGetStateFunction() override;

  // ExtensionFunction:
  ResponseAction Run() override;

 private:
  void Success(std::unique_ptr<base::DictionaryValue> result);
  void Failure(const std::string& error);

  DISALLOW_COPY_AND_ASSIGN(NetworkingOncGetStateFunction);
};

// Implements the chrome.networkingOnc.setProperties method.
class NetworkingOncSetPropertiesFunction : public UIThreadExtensionFunction {
 public:
  NetworkingOncSetPropertiesFunction() {}
  DECLARE_EXTENSION_FUNCTION("networkingOnc.setProperties",
                             NETWORKINGPRIVATE_SETPROPERTIES);

 protected:
  ~NetworkingOncSetPropertiesFunction() override;

  // ExtensionFunction:
  ResponseAction Run() override;

 private:
  void Success();
  void Failure(const std::string& error);

  DISALLOW_COPY_AND_ASSIGN(NetworkingOncSetPropertiesFunction);
};

// Implements the chrome.networkingOnc.createNetwork method.
class NetworkingOncCreateNetworkFunction : public UIThreadExtensionFunction {
 public:
  NetworkingOncCreateNetworkFunction() {}
  DECLARE_EXTENSION_FUNCTION("networkingOnc.createNetwork",
                             NETWORKINGPRIVATE_CREATENETWORK);

 protected:
  ~NetworkingOncCreateNetworkFunction() override;

  // ExtensionFunction:
  ResponseAction Run() override;

 private:
  void Success(const std::string& guid);
  void Failure(const std::string& error);

  DISALLOW_COPY_AND_ASSIGN(NetworkingOncCreateNetworkFunction);
};

// Implements the chrome.networkingOnc.createNetwork method.
class NetworkingOncForgetNetworkFunction : public UIThreadExtensionFunction {
 public:
  NetworkingOncForgetNetworkFunction() {}
  DECLARE_EXTENSION_FUNCTION("networkingOnc.forgetNetwork",
                             NETWORKINGPRIVATE_FORGETNETWORK);

 protected:
  ~NetworkingOncForgetNetworkFunction() override;

  // ExtensionFunction:
  ResponseAction Run() override;

 private:
  void Success();
  void Failure(const std::string& error);

  DISALLOW_COPY_AND_ASSIGN(NetworkingOncForgetNetworkFunction);
};

// Implements the chrome.networkingOnc.getNetworks method.
class NetworkingOncGetNetworksFunction : public UIThreadExtensionFunction {
 public:
  NetworkingOncGetNetworksFunction() {}
  DECLARE_EXTENSION_FUNCTION("networkingOnc.getNetworks",
                             NETWORKINGPRIVATE_GETNETWORKS);

 protected:
  ~NetworkingOncGetNetworksFunction() override;

  // ExtensionFunction:
  ResponseAction Run() override;

 private:
  void Success(std::unique_ptr<base::ListValue> network_list);
  void Failure(const std::string& error);

  DISALLOW_COPY_AND_ASSIGN(NetworkingOncGetNetworksFunction);
};

// Implements the chrome.networkingOnc.getVisibleNetworks method.
class NetworkingOncGetVisibleNetworksFunction
    : public UIThreadExtensionFunction {
 public:
  NetworkingOncGetVisibleNetworksFunction() {}
  DECLARE_EXTENSION_FUNCTION("networkingOnc.getVisibleNetworks",
                             NETWORKINGPRIVATE_GETVISIBLENETWORKS);

 protected:
  ~NetworkingOncGetVisibleNetworksFunction() override;

  // ExtensionFunction:
  ResponseAction Run() override;

 private:
  void Success(std::unique_ptr<base::ListValue> network_list);
  void Failure(const std::string& error);

  DISALLOW_COPY_AND_ASSIGN(NetworkingOncGetVisibleNetworksFunction);
};

// Implements the chrome.networkingOnc.getEnabledNetworkTypes method.
class NetworkingOncGetEnabledNetworkTypesFunction
    : public UIThreadExtensionFunction {
 public:
  NetworkingOncGetEnabledNetworkTypesFunction() {}
  DECLARE_EXTENSION_FUNCTION("networkingOnc.getEnabledNetworkTypes",
                             NETWORKINGPRIVATE_GETENABLEDNETWORKTYPES);

 protected:
  ~NetworkingOncGetEnabledNetworkTypesFunction() override;

  // ExtensionFunction:
  ResponseAction Run() override;

 private:
  DISALLOW_COPY_AND_ASSIGN(NetworkingOncGetEnabledNetworkTypesFunction);
};

// Implements the chrome.networkingOnc.getDeviceStates method.
class NetworkingOncGetDeviceStatesFunction : public UIThreadExtensionFunction {
 public:
  NetworkingOncGetDeviceStatesFunction() {}
  DECLARE_EXTENSION_FUNCTION("networkingOnc.getDeviceStates",
                             NETWORKINGPRIVATE_GETDEVICESTATES);

 protected:
  ~NetworkingOncGetDeviceStatesFunction() override;

  // ExtensionFunction:
  ResponseAction Run() override;

 private:
  DISALLOW_COPY_AND_ASSIGN(NetworkingOncGetDeviceStatesFunction);
};

// Implements the chrome.networkingOnc.enableNetworkType method.
class NetworkingOncEnableNetworkTypeFunction
    : public UIThreadExtensionFunction {
 public:
  NetworkingOncEnableNetworkTypeFunction() {}
  DECLARE_EXTENSION_FUNCTION("networkingOnc.enableNetworkType",
                             NETWORKINGPRIVATE_ENABLENETWORKTYPE);

 protected:
  ~NetworkingOncEnableNetworkTypeFunction() override;

  // ExtensionFunction:
  ResponseAction Run() override;

 private:
  DISALLOW_COPY_AND_ASSIGN(NetworkingOncEnableNetworkTypeFunction);
};

// Implements the chrome.networkingOnc.disableNetworkType method.
class NetworkingOncDisableNetworkTypeFunction
    : public UIThreadExtensionFunction {
 public:
  NetworkingOncDisableNetworkTypeFunction() {}
  DECLARE_EXTENSION_FUNCTION("networkingOnc.disableNetworkType",
                             NETWORKINGPRIVATE_DISABLENETWORKTYPE);

 protected:
  ~NetworkingOncDisableNetworkTypeFunction() override;

  // ExtensionFunction:
  ResponseAction Run() override;

 private:
  DISALLOW_COPY_AND_ASSIGN(NetworkingOncDisableNetworkTypeFunction);
};

// Implements the chrome.networkingOnc.requestNetworkScan method.
class NetworkingOncRequestNetworkScanFunction
    : public UIThreadExtensionFunction {
 public:
  NetworkingOncRequestNetworkScanFunction() {}
  DECLARE_EXTENSION_FUNCTION("networkingOnc.requestNetworkScan",
                             NETWORKINGPRIVATE_REQUESTNETWORKSCAN);

 protected:
  ~NetworkingOncRequestNetworkScanFunction() override;

  // ExtensionFunction:
  ResponseAction Run() override;

 private:
  DISALLOW_COPY_AND_ASSIGN(NetworkingOncRequestNetworkScanFunction);
};

// Implements the chrome.networkingOnc.startConnect method.
class NetworkingOncStartConnectFunction : public UIThreadExtensionFunction {
 public:
  NetworkingOncStartConnectFunction() {}
  DECLARE_EXTENSION_FUNCTION("networkingOnc.startConnect",
                             NETWORKINGPRIVATE_STARTCONNECT);

 protected:
  ~NetworkingOncStartConnectFunction() override;

  // ExtensionFunction:
  ResponseAction Run() override;

 private:
  void Success();
  void Failure(const std::string& guid, const std::string& error);

  DISALLOW_COPY_AND_ASSIGN(NetworkingOncStartConnectFunction);
};

// Implements the chrome.networkingOnc.startDisconnect method.
class NetworkingOncStartDisconnectFunction : public UIThreadExtensionFunction {
 public:
  NetworkingOncStartDisconnectFunction() {}
  DECLARE_EXTENSION_FUNCTION("networkingOnc.startDisconnect",
                             NETWORKINGPRIVATE_STARTDISCONNECT);

 protected:
  ~NetworkingOncStartDisconnectFunction() override;

  // ExtensionFunction:
  ResponseAction Run() override;

 private:
  void Success();
  void Failure(const std::string& error);

  DISALLOW_COPY_AND_ASSIGN(NetworkingOncStartDisconnectFunction);
};

// Implements the chrome.networkingOnc.startActivate method.
class NetworkingOncStartActivateFunction : public UIThreadExtensionFunction {
 public:
  NetworkingOncStartActivateFunction() {}
  DECLARE_EXTENSION_FUNCTION("networkingOnc.startActivate",
                             NETWORKINGPRIVATE_STARTACTIVATE);

 protected:
  ~NetworkingOncStartActivateFunction() override;

  // ExtensionFunction:
  ResponseAction Run() override;

 private:
  void Success();
  void Failure(const std::string& error);

  DISALLOW_COPY_AND_ASSIGN(NetworkingOncStartActivateFunction);
};

// Implements the chrome.networkingOnc.verifyDestination method.
class NetworkingOncVerifyDestinationFunction
    : public UIThreadExtensionFunction {
 public:
  NetworkingOncVerifyDestinationFunction() {}
  DECLARE_EXTENSION_FUNCTION("networkingOnc.verifyDestination",
                             NETWORKINGPRIVATE_VERIFYDESTINATION);

 protected:
  ~NetworkingOncVerifyDestinationFunction() override;

  // ExtensionFunction:
  ResponseAction Run() override;

  void Success(bool result);
  void Failure(const std::string& error);

 private:
  DISALLOW_COPY_AND_ASSIGN(NetworkingOncVerifyDestinationFunction);
};

// Implements the chrome.networkingOnc.verifyAndEncryptCredentials method.
class NetworkingOncVerifyAndEncryptCredentialsFunction
    : public UIThreadExtensionFunction {
 public:
  NetworkingOncVerifyAndEncryptCredentialsFunction() {}
  DECLARE_EXTENSION_FUNCTION("networkingOnc.verifyAndEncryptCredentials",
                             NETWORKINGPRIVATE_VERIFYANDENCRYPTCREDENTIALS);

 protected:
  ~NetworkingOncVerifyAndEncryptCredentialsFunction() override;

  // ExtensionFunction:
  ResponseAction Run() override;

  void Success(const std::string& result);
  void Failure(const std::string& error);

 private:
  DISALLOW_COPY_AND_ASSIGN(NetworkingOncVerifyAndEncryptCredentialsFunction);
};

// Implements the chrome.networkingOnc.verifyAndEncryptData method.
class NetworkingOncVerifyAndEncryptDataFunction
    : public UIThreadExtensionFunction {
 public:
  NetworkingOncVerifyAndEncryptDataFunction() {}
  DECLARE_EXTENSION_FUNCTION("networkingOnc.verifyAndEncryptData",
                             NETWORKINGPRIVATE_VERIFYANDENCRYPTDATA);

 protected:
  ~NetworkingOncVerifyAndEncryptDataFunction() override;

  // ExtensionFunction:
  ResponseAction Run() override;

  void Success(const std::string& result);
  void Failure(const std::string& error);

 private:
  DISALLOW_COPY_AND_ASSIGN(NetworkingOncVerifyAndEncryptDataFunction);
};

// Implements the chrome.networkingOnc.setWifiTDLSEnabledState method.
class NetworkingOncSetWifiTDLSEnabledStateFunction
    : public UIThreadExtensionFunction {
 public:
  NetworkingOncSetWifiTDLSEnabledStateFunction() {}
  DECLARE_EXTENSION_FUNCTION("networkingOnc.setWifiTDLSEnabledState",
                             NETWORKINGPRIVATE_SETWIFITDLSENABLEDSTATE);

 protected:
  ~NetworkingOncSetWifiTDLSEnabledStateFunction() override;

  // ExtensionFunction:
  ResponseAction Run() override;

  void Success(const std::string& result);
  void Failure(const std::string& error);

 private:
  DISALLOW_COPY_AND_ASSIGN(NetworkingOncSetWifiTDLSEnabledStateFunction);
};

// Implements the chrome.networkingOnc.getWifiTDLSStatus method.
class NetworkingOncGetWifiTDLSStatusFunction
    : public UIThreadExtensionFunction {
 public:
  NetworkingOncGetWifiTDLSStatusFunction() {}
  DECLARE_EXTENSION_FUNCTION("networkingOnc.getWifiTDLSStatus",
                             NETWORKINGPRIVATE_GETWIFITDLSSTATUS);

 protected:
  ~NetworkingOncGetWifiTDLSStatusFunction() override;

  // ExtensionFunction:
  ResponseAction Run() override;

  void Success(const std::string& result);
  void Failure(const std::string& error);

 private:
  DISALLOW_COPY_AND_ASSIGN(NetworkingOncGetWifiTDLSStatusFunction);
};

class NetworkingOncGetCaptivePortalStatusFunction
    : public UIThreadExtensionFunction {
 public:
  NetworkingOncGetCaptivePortalStatusFunction() {}
  DECLARE_EXTENSION_FUNCTION("networkingOnc.getCaptivePortalStatus",
                             NETWORKINGPRIVATE_GETCAPTIVEPORTALSTATUS);

  // ExtensionFunction:
  ResponseAction Run() override;

 protected:
  ~NetworkingOncGetCaptivePortalStatusFunction() override;

 private:
  void Success(const std::string& result);
  void Failure(const std::string& error);

  DISALLOW_COPY_AND_ASSIGN(NetworkingOncGetCaptivePortalStatusFunction);
};

class NetworkingOncUnlockCellularSimFunction
    : public UIThreadExtensionFunction {
 public:
  NetworkingOncUnlockCellularSimFunction() {}
  DECLARE_EXTENSION_FUNCTION("networkingOnc.unlockCellularSim",
                             NETWORKINGPRIVATE_UNLOCKCELLULARSIM);

  // ExtensionFunction:
  ResponseAction Run() override;

 protected:
  ~NetworkingOncUnlockCellularSimFunction() override;

 private:
  void Success();
  void Failure(const std::string& error);

  DISALLOW_COPY_AND_ASSIGN(NetworkingOncUnlockCellularSimFunction);
};

class NetworkingOncSetCellularSimStateFunction
    : public UIThreadExtensionFunction {
 public:
  NetworkingOncSetCellularSimStateFunction() {}
  DECLARE_EXTENSION_FUNCTION("networkingOnc.setCellularSimState",
                             NETWORKINGPRIVATE_SETCELLULARSIMSTATE);

  // ExtensionFunction:
  ResponseAction Run() override;

 protected:
  ~NetworkingOncSetCellularSimStateFunction() override;

 private:
  void Success();
  void Failure(const std::string& error);

  DISALLOW_COPY_AND_ASSIGN(NetworkingOncSetCellularSimStateFunction);
};

class NetworkingOncGetGlobalPolicyFunction : public UIThreadExtensionFunction {
 public:
  NetworkingOncGetGlobalPolicyFunction() {}
  DECLARE_EXTENSION_FUNCTION("networkingOnc.getGlobalPolicy",
                             NETWORKINGPRIVATE_GETGLOBALPOLICY);

 protected:
  ~NetworkingOncGetGlobalPolicyFunction() override;

  // ExtensionFunction:
  ResponseAction Run() override;

 private:
  DISALLOW_COPY_AND_ASSIGN(NetworkingOncGetGlobalPolicyFunction);
};

class NetworkingOncGetCertificateListsFunction
    : public UIThreadExtensionFunction {
 public:
  NetworkingOncGetCertificateListsFunction() {}
  DECLARE_EXTENSION_FUNCTION("networkingOnc.getCertificateLists",
                             NETWORKINGPRIVATE_GETCERTIFICATELISTS);

 protected:
  ~NetworkingOncGetCertificateListsFunction() override;

  // ExtensionFunction:
  ResponseAction Run() override;

 private:
  DISALLOW_COPY_AND_ASSIGN(NetworkingOncGetCertificateListsFunction);
};

}  // namespace extensions

#endif  // EXTENSIONS_BROWSER_API_NETWORKING_ONC_NETWORKING_ONC_API_H_
