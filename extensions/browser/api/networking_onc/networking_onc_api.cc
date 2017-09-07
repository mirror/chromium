// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "extensions/browser/api/networking_onc/networking_onc_api.h"

#include <utility>
#include <vector>

#include "base/bind.h"
#include "base/bind_helpers.h"
#include "base/callback.h"
#include "base/memory/ptr_util.h"
#include "base/strings/string_util.h"
#include "components/onc/onc_constants.h"
#include "extensions/browser/api/extensions_api_client.h"
#include "extensions/browser/api/networking_onc/networking_cast_private_delegate.h"
#include "extensions/browser/api/networking_onc/networking_onc_delegate.h"
#include "extensions/browser/api/networking_onc/networking_onc_delegate_factory.h"
#include "extensions/browser/extension_function_registry.h"
#include "extensions/common/api/networking_onc.h"
#include "extensions/common/extension_api.h"
#include "extensions/common/features/feature_provider.h"

#if defined(OS_CHROMEOS)
#include "chromeos/network/network_connect.h"
#endif

namespace extensions {

namespace {

const int kDefaultNetworkListLimit = 1000;

const char kPrivateOnlyError[] = "Requires networkingPrivate API access.";

const char* const kPrivatePropertiesForSet[] = {
    "Cellular.APN", "ProxySettings", "StaticIPConfig", "VPN.Host",
    "VPN.IPsec",    "VPN.L2TP",      "VPN.OpenVPN",    "VPN.ThirdPartyVPN",
};

const char* const kPrivatePropertiesForGet[] = {
    "Cellular.APN",  "Cellular.APNList", "Cellular.LastGoodAPN",
    "Cellular.ESN",  "Cellular.ICCID",   "Cellular.IMEI",
    "Cellular.IMSI", "Cellular.MDN",     "Cellular.MEID",
    "Cellular.MIN",  "Ethernet.EAP",     "VPN.IPsec",
    "VPN.L2TP",      "VPN.OpenVPN",      "WiFi.EAP",
    "WiMax.EAP",
};

NetworkingOncDelegate* GetDelegate(content::BrowserContext* browser_context) {
  return NetworkingOncDelegateFactory::GetForBrowserContext(browser_context);
}

bool HasPrivateNetworkingAccess(const Extension* extension,
                                Feature::Context context,
                                const GURL& source_url) {
  return ExtensionAPI::GetSharedInstance()
      ->IsAvailable("networkingPrivate", extension, context, source_url,
                    CheckAliasStatus::NOT_ALLOWED)
      .is_available();
}

// Indicates which filter should be used - filter for properties allowed to
// be returned by getProperties methods, or the filter for properties settable
// by setProperties/createNetwork methods.
enum class PropertiesType { GET, SET };

// Filters out all properties that are not allowed for the extension in the
// provided context.
// Returns list of removed keys.
std::vector<std::string> FilterProperties(base::DictionaryValue* properties,
                                          PropertiesType type,
                                          const Extension* extension,
                                          Feature::Context context,
                                          const GURL& source_url) {
  if (HasPrivateNetworkingAccess(extension, context, source_url))
    return std::vector<std::string>();

  const char* const* filter = nullptr;
  size_t filter_size = 0;
  if (type == PropertiesType::GET) {
    filter = kPrivatePropertiesForGet;
    filter_size = arraysize(kPrivatePropertiesForGet);
  } else {
    filter = kPrivatePropertiesForSet;
    filter_size = arraysize(kPrivatePropertiesForSet);
  }

  std::vector<std::string> removed_properties;
  for (size_t i = 0; i < filter_size; ++i) {
    base::Value property;
    if (properties->Remove(filter[i], nullptr)) {
      removed_properties.push_back(filter[i]);
    }
  }
  return removed_properties;
}

bool CanChangeSharedConfig(const Extension* extension,
                           Feature::Context context) {
#if defined(OS_CHROMEOS)
  return context == Feature::WEBUI_CONTEXT;
#else
  return true;
#endif
}

std::unique_ptr<NetworkingCastPrivateDelegate::Credentials> AsCastCredentials(
    const api::networking_onc::VerificationProperties& properties) {
  return std::make_unique<NetworkingCastPrivateDelegate::Credentials>(
      properties.certificate,
      properties.intermediate_certificates
          ? *properties.intermediate_certificates
          : std::vector<std::string>(),
      properties.signed_data, properties.device_ssid, properties.device_serial,
      properties.device_bssid, properties.public_key, properties.nonce);
}

std::string InvalidPropertiesError(const std::vector<std::string>& properties) {
  DCHECK(!properties.empty());
  return "Error.PropertiesNotAllowed: [" + base::JoinString(properties, ", ") +
         "]";
}

}  // namespace

namespace networking_onc {

// static
const char kErrorAccessToSharedConfig[] = "Error.CannotChangeSharedConfig";
const char kErrorInvalidArguments[] = "Error.InvalidArguments";
const char kErrorInvalidNetworkGuid[] = "Error.InvalidNetworkGuid";
const char kErrorInvalidNetworkOperation[] = "Error.InvalidNetworkOperation";
const char kErrorNetworkUnavailable[] = "Error.NetworkUnavailable";
const char kErrorNotReady[] = "Error.NotReady";
const char kErrorNotSupported[] = "Error.NotSupported";
const char kErrorPolicyControlled[] = "Error.PolicyControlled";
const char kErrorSimLocked[] = "Error.SimLocked";

}  // namespace networking_onc

////////////////////////////////////////////////////////////////////////////////
// NetworkingOncGetPropertiesFunction

NetworkingOncGetPropertiesFunction::~NetworkingOncGetPropertiesFunction() {}

ExtensionFunction::ResponseAction NetworkingOncGetPropertiesFunction::Run() {
  std::unique_ptr<api::networking_onc::GetProperties::Params> params =
      api::networking_onc::GetProperties::Params::Create(*args_);
  EXTENSION_FUNCTION_VALIDATE(params);

  GetDelegate(browser_context())
      ->GetProperties(
          params->network_guid,
          base::Bind(&NetworkingOncGetPropertiesFunction::Success, this),
          base::Bind(&NetworkingOncGetPropertiesFunction::Failure, this));
  // Success() or Failure() might have been called synchronously at this point.
  // In that case this function has already called Respond(). Return
  // AlreadyResponded() in that case.
  return did_respond() ? AlreadyResponded() : RespondLater();
}

void NetworkingOncGetPropertiesFunction::Success(
    std::unique_ptr<base::DictionaryValue> result) {
  FilterProperties(result.get(), PropertiesType::GET, extension(),
                   source_context_type(), source_url());
  Respond(OneArgument(std::move(result)));
}

void NetworkingOncGetPropertiesFunction::Failure(const std::string& error) {
  Respond(Error(error));
}

////////////////////////////////////////////////////////////////////////////////
// NetworkingOncGetManagedPropertiesFunction

NetworkingOncGetManagedPropertiesFunction::
    ~NetworkingOncGetManagedPropertiesFunction() {}

ExtensionFunction::ResponseAction
NetworkingOncGetManagedPropertiesFunction::Run() {
  std::unique_ptr<api::networking_onc::GetManagedProperties::Params> params =
      api::networking_onc::GetManagedProperties::Params::Create(*args_);
  EXTENSION_FUNCTION_VALIDATE(params);

  GetDelegate(browser_context())
      ->GetManagedProperties(
          params->network_guid,
          base::Bind(&NetworkingOncGetManagedPropertiesFunction::Success, this),
          base::Bind(&NetworkingOncGetManagedPropertiesFunction::Failure,
                     this));
  // Success() or Failure() might have been called synchronously at this point.
  // In that case this function has already called Respond(). Return
  // AlreadyResponded() in that case.
  return did_respond() ? AlreadyResponded() : RespondLater();
}

void NetworkingOncGetManagedPropertiesFunction::Success(
    std::unique_ptr<base::DictionaryValue> result) {
  FilterProperties(result.get(), PropertiesType::GET, extension(),
                   source_context_type(), source_url());
  Respond(OneArgument(std::move(result)));
}

void NetworkingOncGetManagedPropertiesFunction::Failure(
    const std::string& error) {
  Respond(Error(error));
}

////////////////////////////////////////////////////////////////////////////////
// NetworkingOncGetStateFunction

NetworkingOncGetStateFunction::~NetworkingOncGetStateFunction() {}

ExtensionFunction::ResponseAction NetworkingOncGetStateFunction::Run() {
  std::unique_ptr<api::networking_onc::GetState::Params> params =
      api::networking_onc::GetState::Params::Create(*args_);
  EXTENSION_FUNCTION_VALIDATE(params);

  GetDelegate(browser_context())
      ->GetState(params->network_guid,
                 base::Bind(&NetworkingOncGetStateFunction::Success, this),
                 base::Bind(&NetworkingOncGetStateFunction::Failure, this));
  // Success() or Failure() might have been called synchronously at this point.
  // In that case this function has already called Respond(). Return
  // AlreadyResponded() in that case.
  return did_respond() ? AlreadyResponded() : RespondLater();
}

void NetworkingOncGetStateFunction::Success(
    std::unique_ptr<base::DictionaryValue> result) {
  FilterProperties(result.get(), PropertiesType::GET, extension(),
                   source_context_type(), source_url());
  Respond(OneArgument(std::move(result)));
}

void NetworkingOncGetStateFunction::Failure(const std::string& error) {
  Respond(Error(error));
}

////////////////////////////////////////////////////////////////////////////////
// NetworkingOncSetPropertiesFunction

NetworkingOncSetPropertiesFunction::~NetworkingOncSetPropertiesFunction() {}

ExtensionFunction::ResponseAction NetworkingOncSetPropertiesFunction::Run() {
  std::unique_ptr<api::networking_onc::SetProperties::Params> params =
      api::networking_onc::SetProperties::Params::Create(*args_);
  EXTENSION_FUNCTION_VALIDATE(params);

  std::unique_ptr<base::DictionaryValue> properties_dict(
      params->properties.ToValue());

  std::vector<std::string> not_allowed_properties =
      FilterProperties(properties_dict.get(), PropertiesType::SET, extension(),
                       source_context_type(), source_url());
  if (!not_allowed_properties.empty())
    return RespondNow(Error(InvalidPropertiesError(not_allowed_properties)));

  GetDelegate(browser_context())
      ->SetProperties(
          params->network_guid, std::move(properties_dict),
          CanChangeSharedConfig(extension(), source_context_type()),
          base::Bind(&NetworkingOncSetPropertiesFunction::Success, this),
          base::Bind(&NetworkingOncSetPropertiesFunction::Failure, this));
  // Success() or Failure() might have been called synchronously at this point.
  // In that case this function has already called Respond(). Return
  // AlreadyResponded() in that case.
  return did_respond() ? AlreadyResponded() : RespondLater();
}

void NetworkingOncSetPropertiesFunction::Success() {
  Respond(NoArguments());
}

void NetworkingOncSetPropertiesFunction::Failure(const std::string& error) {
  Respond(Error(error));
}

////////////////////////////////////////////////////////////////////////////////
// NetworkingOncCreateNetworkFunction

NetworkingOncCreateNetworkFunction::~NetworkingOncCreateNetworkFunction() {}

ExtensionFunction::ResponseAction NetworkingOncCreateNetworkFunction::Run() {
  std::unique_ptr<api::networking_onc::CreateNetwork::Params> params =
      api::networking_onc::CreateNetwork::Params::Create(*args_);
  EXTENSION_FUNCTION_VALIDATE(params);

  if (params->shared &&
      !CanChangeSharedConfig(extension(), source_context_type())) {
    return RespondNow(Error(networking_onc::kErrorAccessToSharedConfig));
  }

  std::unique_ptr<base::DictionaryValue> properties_dict(
      params->properties.ToValue());

  std::vector<std::string> not_allowed_properties =
      FilterProperties(properties_dict.get(), PropertiesType::SET, extension(),
                       source_context_type(), source_url());
  if (!not_allowed_properties.empty())
    return RespondNow(Error(InvalidPropertiesError(not_allowed_properties)));

  GetDelegate(browser_context())
      ->CreateNetwork(
          params->shared, std::move(properties_dict),
          base::Bind(&NetworkingOncCreateNetworkFunction::Success, this),
          base::Bind(&NetworkingOncCreateNetworkFunction::Failure, this));
  // Success() or Failure() might have been called synchronously at this point.
  // In that case this function has already called Respond(). Return
  // AlreadyResponded() in that case.
  return did_respond() ? AlreadyResponded() : RespondLater();
}

void NetworkingOncCreateNetworkFunction::Success(const std::string& guid) {
  Respond(
      ArgumentList(api::networking_onc::CreateNetwork::Results::Create(guid)));
}

void NetworkingOncCreateNetworkFunction::Failure(const std::string& error) {
  Respond(Error(error));
}

////////////////////////////////////////////////////////////////////////////////
// NetworkingOncForgetNetworkFunction

NetworkingOncForgetNetworkFunction::~NetworkingOncForgetNetworkFunction() {}

ExtensionFunction::ResponseAction NetworkingOncForgetNetworkFunction::Run() {
  std::unique_ptr<api::networking_onc::ForgetNetwork::Params> params =
      api::networking_onc::ForgetNetwork::Params::Create(*args_);
  EXTENSION_FUNCTION_VALIDATE(params);

  GetDelegate(browser_context())
      ->ForgetNetwork(
          params->network_guid,
          CanChangeSharedConfig(extension(), source_context_type()),
          base::Bind(&NetworkingOncForgetNetworkFunction::Success, this),
          base::Bind(&NetworkingOncForgetNetworkFunction::Failure, this));
  // Success() or Failure() might have been called synchronously at this point.
  // In that case this function has already called Respond(). Return
  // AlreadyResponded() in that case.
  return did_respond() ? AlreadyResponded() : RespondLater();
}

void NetworkingOncForgetNetworkFunction::Success() {
  Respond(NoArguments());
}

void NetworkingOncForgetNetworkFunction::Failure(const std::string& error) {
  Respond(Error(error));
}

////////////////////////////////////////////////////////////////////////////////
// NetworkingOncGetNetworksFunction

NetworkingOncGetNetworksFunction::~NetworkingOncGetNetworksFunction() {}

ExtensionFunction::ResponseAction NetworkingOncGetNetworksFunction::Run() {
  std::unique_ptr<api::networking_onc::GetNetworks::Params> params =
      api::networking_onc::GetNetworks::Params::Create(*args_);
  EXTENSION_FUNCTION_VALIDATE(params);

  std::string network_type =
      api::networking_onc::ToString(params->filter.network_type);
  const bool configured_only =
      params->filter.configured ? *params->filter.configured : false;
  const bool visible_only =
      params->filter.visible ? *params->filter.visible : false;
  const int limit =
      params->filter.limit ? *params->filter.limit : kDefaultNetworkListLimit;

  GetDelegate(browser_context())
      ->GetNetworks(
          network_type, configured_only, visible_only, limit,
          base::Bind(&NetworkingOncGetNetworksFunction::Success, this),
          base::Bind(&NetworkingOncGetNetworksFunction::Failure, this));
  // Success() or Failure() might have been called synchronously at this point.
  // In that case this function has already called Respond(). Return
  // AlreadyResponded() in that case.
  return did_respond() ? AlreadyResponded() : RespondLater();
}

void NetworkingOncGetNetworksFunction::Success(
    std::unique_ptr<base::ListValue> network_list) {
  return Respond(OneArgument(std::move(network_list)));
}

void NetworkingOncGetNetworksFunction::Failure(const std::string& error) {
  Respond(Error(error));
}

////////////////////////////////////////////////////////////////////////////////
// NetworkingOncGetVisibleNetworksFunction

NetworkingOncGetVisibleNetworksFunction::
    ~NetworkingOncGetVisibleNetworksFunction() {}

ExtensionFunction::ResponseAction
NetworkingOncGetVisibleNetworksFunction::Run() {
  std::unique_ptr<api::networking_onc::GetVisibleNetworks::Params> params =
      api::networking_onc::GetVisibleNetworks::Params::Create(*args_);
  EXTENSION_FUNCTION_VALIDATE(params);

  // getVisibleNetworks is deprecated - allow it only for apps with
  // networkingPrivate permissions, i.e. apps that might have started using it
  // before its deprecation.
  if (!HasPrivateNetworkingAccess(extension(), source_context_type(),
                                  source_url())) {
    return RespondNow(Error(kPrivateOnlyError));
  }

  std::string network_type =
      api::networking_onc::ToString(params->network_type);
  const bool configured_only = false;
  const bool visible_only = true;

  GetDelegate(browser_context())
      ->GetNetworks(
          network_type, configured_only, visible_only, kDefaultNetworkListLimit,
          base::Bind(&NetworkingOncGetVisibleNetworksFunction::Success, this),
          base::Bind(&NetworkingOncGetVisibleNetworksFunction::Failure, this));
  // Success() or Failure() might have been called synchronously at this point.
  // In that case this function has already called Respond(). Return
  // AlreadyResponded() in that case.
  return did_respond() ? AlreadyResponded() : RespondLater();
}

void NetworkingOncGetVisibleNetworksFunction::Success(
    std::unique_ptr<base::ListValue> network_properties_list) {
  Respond(OneArgument(std::move(network_properties_list)));
}

void NetworkingOncGetVisibleNetworksFunction::Failure(
    const std::string& error) {
  Respond(Error(error));
}

////////////////////////////////////////////////////////////////////////////////
// NetworkingOncGetEnabledNetworkTypesFunction

NetworkingOncGetEnabledNetworkTypesFunction::
    ~NetworkingOncGetEnabledNetworkTypesFunction() {}

ExtensionFunction::ResponseAction
NetworkingOncGetEnabledNetworkTypesFunction::Run() {
  // getEnabledNetworkTypes is deprecated - allow it only for apps with
  // networkingPrivate permissions, i.e. apps that might have started using it
  // before its deprecation.
  if (!HasPrivateNetworkingAccess(extension(), source_context_type(),
                                  source_url())) {
    return RespondNow(Error(kPrivateOnlyError));
  }

  std::unique_ptr<base::ListValue> enabled_networks_onc_types(
      GetDelegate(browser_context())->GetEnabledNetworkTypes());
  if (!enabled_networks_onc_types)
    return RespondNow(Error(networking_onc::kErrorNotSupported));
  std::unique_ptr<base::ListValue> enabled_networks_list(new base::ListValue);
  for (base::ListValue::iterator iter = enabled_networks_onc_types->begin();
       iter != enabled_networks_onc_types->end(); ++iter) {
    std::string type;
    if (!iter->GetAsString(&type))
      NOTREACHED();
    if (type == ::onc::network_type::kEthernet) {
      enabled_networks_list->AppendString(api::networking_onc::ToString(
          api::networking_onc::NETWORK_TYPE_ETHERNET));
    } else if (type == ::onc::network_type::kWiFi) {
      enabled_networks_list->AppendString(api::networking_onc::ToString(
          api::networking_onc::NETWORK_TYPE_WIFI));
    } else if (type == ::onc::network_type::kWimax) {
      enabled_networks_list->AppendString(api::networking_onc::ToString(
          api::networking_onc::NETWORK_TYPE_WIMAX));
    } else if (type == ::onc::network_type::kCellular) {
      enabled_networks_list->AppendString(api::networking_onc::ToString(
          api::networking_onc::NETWORK_TYPE_CELLULAR));
    } else {
      LOG(ERROR) << "networkingOnc: Unexpected type: " << type;
    }
  }
  return RespondNow(OneArgument(std::move(enabled_networks_list)));
}

////////////////////////////////////////////////////////////////////////////////
// NetworkingOncGetDeviceStatesFunction

NetworkingOncGetDeviceStatesFunction::~NetworkingOncGetDeviceStatesFunction() {}

ExtensionFunction::ResponseAction NetworkingOncGetDeviceStatesFunction::Run() {
  std::unique_ptr<NetworkingOncDelegate::DeviceStateList> device_states(
      GetDelegate(browser_context())->GetDeviceStateList());
  if (!device_states)
    return RespondNow(Error(networking_onc::kErrorNotSupported));

  std::unique_ptr<base::ListValue> device_state_list(new base::ListValue);
  for (const auto& properties : *device_states)
    device_state_list->Append(properties->ToValue());
  return RespondNow(OneArgument(std::move(device_state_list)));
}

////////////////////////////////////////////////////////////////////////////////
// NetworkingOncEnableNetworkTypeFunction

NetworkingOncEnableNetworkTypeFunction::
    ~NetworkingOncEnableNetworkTypeFunction() {}

ExtensionFunction::ResponseAction
NetworkingOncEnableNetworkTypeFunction::Run() {
  std::unique_ptr<api::networking_onc::EnableNetworkType::Params> params =
      api::networking_onc::EnableNetworkType::Params::Create(*args_);
  EXTENSION_FUNCTION_VALIDATE(params);

  if (!GetDelegate(browser_context())
           ->EnableNetworkType(
               api::networking_onc::ToString(params->network_type))) {
    return RespondNow(Error(networking_onc::kErrorNotSupported));
  }
  return RespondNow(NoArguments());
}

////////////////////////////////////////////////////////////////////////////////
// NetworkingOncDisableNetworkTypeFunction

NetworkingOncDisableNetworkTypeFunction::
    ~NetworkingOncDisableNetworkTypeFunction() {}

ExtensionFunction::ResponseAction
NetworkingOncDisableNetworkTypeFunction::Run() {
  std::unique_ptr<api::networking_onc::DisableNetworkType::Params> params =
      api::networking_onc::DisableNetworkType::Params::Create(*args_);

  if (!GetDelegate(browser_context())
           ->DisableNetworkType(
               api::networking_onc::ToString(params->network_type))) {
    return RespondNow(Error(networking_onc::kErrorNotSupported));
  }
  return RespondNow(NoArguments());
}

////////////////////////////////////////////////////////////////////////////////
// NetworkingOncRequestNetworkScanFunction

NetworkingOncRequestNetworkScanFunction::
    ~NetworkingOncRequestNetworkScanFunction() {}

ExtensionFunction::ResponseAction
NetworkingOncRequestNetworkScanFunction::Run() {
  if (!GetDelegate(browser_context())->RequestScan())
    return RespondNow(Error(networking_onc::kErrorNotSupported));
  return RespondNow(NoArguments());
}

////////////////////////////////////////////////////////////////////////////////
// NetworkingOncStartConnectFunction

NetworkingOncStartConnectFunction::~NetworkingOncStartConnectFunction() {}

ExtensionFunction::ResponseAction NetworkingOncStartConnectFunction::Run() {
  std::unique_ptr<api::networking_onc::StartConnect::Params> params =
      api::networking_onc::StartConnect::Params::Create(*args_);
  EXTENSION_FUNCTION_VALIDATE(params);

  GetDelegate(browser_context())
      ->StartConnect(
          params->network_guid,
          base::Bind(&NetworkingOncStartConnectFunction::Success, this),
          base::Bind(&NetworkingOncStartConnectFunction::Failure, this,
                     params->network_guid));
  // Success() or Failure() might have been called synchronously at this point.
  // In that case this function has already called Respond(). Return
  // AlreadyResponded() in that case.
  return did_respond() ? AlreadyResponded() : RespondLater();
}

void NetworkingOncStartConnectFunction::Success() {
  Respond(NoArguments());
}

void NetworkingOncStartConnectFunction::Failure(const std::string& guid,
                                                const std::string& error) {
// TODO(stevenjb): Temporary workaround to show the configuration UI.
// Eventually the caller (e.g. Settings) should handle any failures and
// show its own configuration UI. crbug.com/380937.
#if defined(OS_CHROMEOS)
  if (source_context_type() == Feature::WEBUI_CONTEXT &&
      chromeos::NetworkConnect::Get()->MaybeShowConfigureUI(guid, error)) {
    Success();
    return;
  }
#endif

  Respond(Error(error));
}

////////////////////////////////////////////////////////////////////////////////
// NetworkingOncStartDisconnectFunction

NetworkingOncStartDisconnectFunction::~NetworkingOncStartDisconnectFunction() {}

ExtensionFunction::ResponseAction NetworkingOncStartDisconnectFunction::Run() {
  std::unique_ptr<api::networking_onc::StartDisconnect::Params> params =
      api::networking_onc::StartDisconnect::Params::Create(*args_);
  EXTENSION_FUNCTION_VALIDATE(params);

  GetDelegate(browser_context())
      ->StartDisconnect(
          params->network_guid,
          base::Bind(&NetworkingOncStartDisconnectFunction::Success, this),
          base::Bind(&NetworkingOncStartDisconnectFunction::Failure, this));
  // Success() or Failure() might have been called synchronously at this point.
  // In that case this function has already called Respond(). Return
  // AlreadyResponded() in that case.
  return did_respond() ? AlreadyResponded() : RespondLater();
}

void NetworkingOncStartDisconnectFunction::Success() {
  Respond(NoArguments());
}

void NetworkingOncStartDisconnectFunction::Failure(const std::string& error) {
  Respond(Error(error));
}

////////////////////////////////////////////////////////////////////////////////
// NetworkingOncStartActivateFunction

NetworkingOncStartActivateFunction::~NetworkingOncStartActivateFunction() {}

ExtensionFunction::ResponseAction NetworkingOncStartActivateFunction::Run() {
  if (!HasPrivateNetworkingAccess(extension(), source_context_type(),
                                  source_url())) {
    return RespondNow(Error(kPrivateOnlyError));
  }

  std::unique_ptr<api::networking_onc::StartActivate::Params> params =
      api::networking_onc::StartActivate::Params::Create(*args_);
  EXTENSION_FUNCTION_VALIDATE(params);

  GetDelegate(browser_context())
      ->StartActivate(
          params->network_guid, params->carrier ? *params->carrier : "",
          base::Bind(&NetworkingOncStartActivateFunction::Success, this),
          base::Bind(&NetworkingOncStartActivateFunction::Failure, this));
  // Success() or Failure() might have been called synchronously at this point.
  // In that case this function has already called Respond(). Return
  // AlreadyResponded() in that case.
  return did_respond() ? AlreadyResponded() : RespondLater();
}

void NetworkingOncStartActivateFunction::Success() {
  Respond(NoArguments());
}

void NetworkingOncStartActivateFunction::Failure(const std::string& error) {
  Respond(Error(error));
}

////////////////////////////////////////////////////////////////////////////////
// NetworkingOncVerifyDestinationFunction

NetworkingOncVerifyDestinationFunction::
    ~NetworkingOncVerifyDestinationFunction() {}

ExtensionFunction::ResponseAction
NetworkingOncVerifyDestinationFunction::Run() {
  // This method is private - as such, it should not be exposed through public
  // networking.onc API.
  // TODO(tbarzic): Consider exposing this via separate API.
  // http://crbug.com/678737
  if (!HasPrivateNetworkingAccess(extension(), source_context_type(),
                                  source_url())) {
    return RespondNow(Error(kPrivateOnlyError));
  }

  std::unique_ptr<api::networking_onc::VerifyDestination::Params> params =
      api::networking_onc::VerifyDestination::Params::Create(*args_);
  EXTENSION_FUNCTION_VALIDATE(params);

  NetworkingCastPrivateDelegate* delegate =
      ExtensionsAPIClient::Get()->GetNetworkingCastPrivateDelegate();
  if (!delegate)
    return RespondNow(Error("Not supported."));

  delegate->VerifyDestination(
      AsCastCredentials(params->properties),
      base::Bind(&NetworkingOncVerifyDestinationFunction::Success, this),
      base::Bind(&NetworkingOncVerifyDestinationFunction::Failure, this));
  // Success() or Failure() might have been called synchronously at this point.
  // In that case this function has already called Respond(). Return
  // AlreadyResponded() in that case.
  return did_respond() ? AlreadyResponded() : RespondLater();
}

void NetworkingOncVerifyDestinationFunction::Success(bool result) {
  Respond(ArgumentList(
      api::networking_onc::VerifyDestination::Results::Create(result)));
}

void NetworkingOncVerifyDestinationFunction::Failure(const std::string& error) {
  Respond(Error(error));
}

////////////////////////////////////////////////////////////////////////////////
// NetworkingOncVerifyAndEncryptCredentialsFunction

NetworkingOncVerifyAndEncryptCredentialsFunction::
    ~NetworkingOncVerifyAndEncryptCredentialsFunction() {}

ExtensionFunction::ResponseAction
NetworkingOncVerifyAndEncryptCredentialsFunction::Run() {
  // This method is private - as such, it should not be exposed through public
  // networking.onc API.
  // TODO(tbarzic): Consider exposing this via separate API.
  // http://crbug.com/678737
  if (!HasPrivateNetworkingAccess(extension(), source_context_type(),
                                  source_url())) {
    return RespondNow(Error(kPrivateOnlyError));
  }
  std::unique_ptr<api::networking_onc::VerifyAndEncryptCredentials::Params>
      params = api::networking_onc::VerifyAndEncryptCredentials::Params::Create(
          *args_);
  EXTENSION_FUNCTION_VALIDATE(params);

  NetworkingCastPrivateDelegate* delegate =
      ExtensionsAPIClient::Get()->GetNetworkingCastPrivateDelegate();
  if (!delegate)
    return RespondNow(Error("Not supported."));

  delegate->VerifyAndEncryptCredentials(
      params->network_guid, AsCastCredentials(params->properties),
      base::Bind(&NetworkingOncVerifyAndEncryptCredentialsFunction::Success,
                 this),
      base::Bind(&NetworkingOncVerifyAndEncryptCredentialsFunction::Failure,
                 this));
  // Success() or Failure() might have been called synchronously at this point.
  // In that case this function has already called Respond(). Return
  // AlreadyResponded() in that case.
  return did_respond() ? AlreadyResponded() : RespondLater();
}

void NetworkingOncVerifyAndEncryptCredentialsFunction::Success(
    const std::string& result) {
  Respond(ArgumentList(
      api::networking_onc::VerifyAndEncryptCredentials::Results::Create(
          result)));
}

void NetworkingOncVerifyAndEncryptCredentialsFunction::Failure(
    const std::string& error) {
  Respond(Error(error));
}

////////////////////////////////////////////////////////////////////////////////
// NetworkingOncVerifyAndEncryptDataFunction

NetworkingOncVerifyAndEncryptDataFunction::
    ~NetworkingOncVerifyAndEncryptDataFunction() {}

ExtensionFunction::ResponseAction
NetworkingOncVerifyAndEncryptDataFunction::Run() {
  // This method is private - as such, it should not be exposed through public
  // networking.onc API.
  // TODO(tbarzic): Consider exposing this via separate API.
  // http://crbug.com/678737
  if (!HasPrivateNetworkingAccess(extension(), source_context_type(),
                                  source_url())) {
    return RespondNow(Error(kPrivateOnlyError));
  }
  std::unique_ptr<api::networking_onc::VerifyAndEncryptData::Params> params =
      api::networking_onc::VerifyAndEncryptData::Params::Create(*args_);
  EXTENSION_FUNCTION_VALIDATE(params);

  NetworkingCastPrivateDelegate* delegate =
      ExtensionsAPIClient::Get()->GetNetworkingCastPrivateDelegate();
  if (!delegate)
    return RespondNow(Error("Not supported."));

  delegate->VerifyAndEncryptData(
      params->data, AsCastCredentials(params->properties),
      base::Bind(&NetworkingOncVerifyAndEncryptDataFunction::Success, this),
      base::Bind(&NetworkingOncVerifyAndEncryptDataFunction::Failure, this));
  // Success() or Failure() might have been called synchronously at this point.
  // In that case this function has already called Respond(). Return
  // AlreadyResponded() in that case.
  return did_respond() ? AlreadyResponded() : RespondLater();
}

void NetworkingOncVerifyAndEncryptDataFunction::Success(
    const std::string& result) {
  Respond(ArgumentList(
      api::networking_onc::VerifyAndEncryptData::Results::Create(result)));
}

void NetworkingOncVerifyAndEncryptDataFunction::Failure(
    const std::string& error) {
  Respond(Error(error));
}

////////////////////////////////////////////////////////////////////////////////
// NetworkingOncSetWifiTDLSEnabledStateFunction

NetworkingOncSetWifiTDLSEnabledStateFunction::
    ~NetworkingOncSetWifiTDLSEnabledStateFunction() {}

ExtensionFunction::ResponseAction
NetworkingOncSetWifiTDLSEnabledStateFunction::Run() {
  // This method is private - as such, it should not be exposed through public
  // networking.onc API.
  // TODO(tbarzic): Consider exposing this via separate API.
  // http://crbug.com/678737
  if (!HasPrivateNetworkingAccess(extension(), source_context_type(),
                                  source_url())) {
    return RespondNow(Error(kPrivateOnlyError));
  }
  std::unique_ptr<api::networking_onc::SetWifiTDLSEnabledState::Params> params =
      api::networking_onc::SetWifiTDLSEnabledState::Params::Create(*args_);
  EXTENSION_FUNCTION_VALIDATE(params);

  GetDelegate(browser_context())
      ->SetWifiTDLSEnabledState(
          params->ip_or_mac_address, params->enabled,
          base::Bind(&NetworkingOncSetWifiTDLSEnabledStateFunction::Success,
                     this),
          base::Bind(&NetworkingOncSetWifiTDLSEnabledStateFunction::Failure,
                     this));
  // Success() or Failure() might have been called synchronously at this point.
  // In that case this function has already called Respond(). Return
  // AlreadyResponded() in that case.
  return did_respond() ? AlreadyResponded() : RespondLater();
}

void NetworkingOncSetWifiTDLSEnabledStateFunction::Success(
    const std::string& result) {
  Respond(ArgumentList(
      api::networking_onc::SetWifiTDLSEnabledState::Results::Create(result)));
}

void NetworkingOncSetWifiTDLSEnabledStateFunction::Failure(
    const std::string& error) {
  Respond(Error(error));
}

////////////////////////////////////////////////////////////////////////////////
// NetworkingOncGetWifiTDLSStatusFunction

NetworkingOncGetWifiTDLSStatusFunction::
    ~NetworkingOncGetWifiTDLSStatusFunction() {}

ExtensionFunction::ResponseAction
NetworkingOncGetWifiTDLSStatusFunction::Run() {
  // This method is private - as such, it should not be exposed through public
  // networking.onc API.
  // TODO(tbarzic): Consider exposing this via separate API.
  // http://crbug.com/678737
  if (!HasPrivateNetworkingAccess(extension(), source_context_type(),
                                  source_url())) {
    return RespondNow(Error(kPrivateOnlyError));
  }
  std::unique_ptr<api::networking_onc::GetWifiTDLSStatus::Params> params =
      api::networking_onc::GetWifiTDLSStatus::Params::Create(*args_);
  EXTENSION_FUNCTION_VALIDATE(params);

  GetDelegate(browser_context())
      ->GetWifiTDLSStatus(
          params->ip_or_mac_address,
          base::Bind(&NetworkingOncGetWifiTDLSStatusFunction::Success, this),
          base::Bind(&NetworkingOncGetWifiTDLSStatusFunction::Failure, this));
  // Success() or Failure() might have been called synchronously at this point.
  // In that case this function has already called Respond(). Return
  // AlreadyResponded() in that case.
  return did_respond() ? AlreadyResponded() : RespondLater();
}

void NetworkingOncGetWifiTDLSStatusFunction::Success(
    const std::string& result) {
  Respond(ArgumentList(
      api::networking_onc::GetWifiTDLSStatus::Results::Create(result)));
}

void NetworkingOncGetWifiTDLSStatusFunction::Failure(const std::string& error) {
  Respond(Error(error));
}

////////////////////////////////////////////////////////////////////////////////
// NetworkingOncGetCaptivePortalStatusFunction

NetworkingOncGetCaptivePortalStatusFunction::
    ~NetworkingOncGetCaptivePortalStatusFunction() {}

ExtensionFunction::ResponseAction
NetworkingOncGetCaptivePortalStatusFunction::Run() {
  std::unique_ptr<api::networking_onc::GetCaptivePortalStatus::Params> params =
      api::networking_onc::GetCaptivePortalStatus::Params::Create(*args_);
  EXTENSION_FUNCTION_VALIDATE(params);

  GetDelegate(browser_context())
      ->GetCaptivePortalStatus(
          params->network_guid,
          base::Bind(&NetworkingOncGetCaptivePortalStatusFunction::Success,
                     this),
          base::Bind(&NetworkingOncGetCaptivePortalStatusFunction::Failure,
                     this));
  // Success() or Failure() might have been called synchronously at this point.
  // In that case this function has already called Respond(). Return
  // AlreadyResponded() in that case.
  return did_respond() ? AlreadyResponded() : RespondLater();
}

void NetworkingOncGetCaptivePortalStatusFunction::Success(
    const std::string& result) {
  Respond(
      ArgumentList(api::networking_onc::GetCaptivePortalStatus::Results::Create(
          api::networking_onc::ParseCaptivePortalStatus(result))));
}

void NetworkingOncGetCaptivePortalStatusFunction::Failure(
    const std::string& error) {
  Respond(Error(error));
}

////////////////////////////////////////////////////////////////////////////////
// NetworkingOncUnlockCellularSimFunction

NetworkingOncUnlockCellularSimFunction::
    ~NetworkingOncUnlockCellularSimFunction() {}

ExtensionFunction::ResponseAction
NetworkingOncUnlockCellularSimFunction::Run() {
  if (!HasPrivateNetworkingAccess(extension(), source_context_type(),
                                  source_url())) {
    return RespondNow(Error(kPrivateOnlyError));
  }

  std::unique_ptr<api::networking_onc::UnlockCellularSim::Params> params =
      api::networking_onc::UnlockCellularSim::Params::Create(*args_);
  EXTENSION_FUNCTION_VALIDATE(params);

  GetDelegate(browser_context())
      ->UnlockCellularSim(
          params->network_guid, params->pin, params->puk ? *params->puk : "",
          base::Bind(&NetworkingOncUnlockCellularSimFunction::Success, this),
          base::Bind(&NetworkingOncUnlockCellularSimFunction::Failure, this));
  // Success() or Failure() might have been called synchronously at this point.
  // In that case this function has already called Respond(). Return
  // AlreadyResponded() in that case.
  return did_respond() ? AlreadyResponded() : RespondLater();
}

void NetworkingOncUnlockCellularSimFunction::Success() {
  Respond(NoArguments());
}

void NetworkingOncUnlockCellularSimFunction::Failure(const std::string& error) {
  Respond(Error(error));
}

////////////////////////////////////////////////////////////////////////////////
// NetworkingOncSetCellularSimStateFunction

NetworkingOncSetCellularSimStateFunction::
    ~NetworkingOncSetCellularSimStateFunction() {}

ExtensionFunction::ResponseAction
NetworkingOncSetCellularSimStateFunction::Run() {
  if (!HasPrivateNetworkingAccess(extension(), source_context_type(),
                                  source_url())) {
    return RespondNow(Error(kPrivateOnlyError));
  }

  std::unique_ptr<api::networking_onc::SetCellularSimState::Params> params =
      api::networking_onc::SetCellularSimState::Params::Create(*args_);
  EXTENSION_FUNCTION_VALIDATE(params);

  GetDelegate(browser_context())
      ->SetCellularSimState(
          params->network_guid, params->sim_state.require_pin,
          params->sim_state.current_pin,
          params->sim_state.new_pin ? *params->sim_state.new_pin : "",
          base::Bind(&NetworkingOncSetCellularSimStateFunction::Success, this),
          base::Bind(&NetworkingOncSetCellularSimStateFunction::Failure, this));
  // Success() or Failure() might have been called synchronously at this point.
  // In that case this function has already called Respond(). Return
  // AlreadyResponded() in that case.
  return did_respond() ? AlreadyResponded() : RespondLater();
}

void NetworkingOncSetCellularSimStateFunction::Success() {
  Respond(NoArguments());
}

void NetworkingOncSetCellularSimStateFunction::Failure(
    const std::string& error) {
  Respond(Error(error));
}

////////////////////////////////////////////////////////////////////////////////
// NetworkingOncGetGlobalPolicyFunction

NetworkingOncGetGlobalPolicyFunction::~NetworkingOncGetGlobalPolicyFunction() {}

ExtensionFunction::ResponseAction NetworkingOncGetGlobalPolicyFunction::Run() {
  std::unique_ptr<base::DictionaryValue> policy_dict(
      GetDelegate(browser_context())->GetGlobalPolicy());
  DCHECK(policy_dict);
  // api::networking_onc::GlobalPolicy is a subset of the global policy
  // dictionary (by definition), so use the api setter/getter to generate the
  // subset.
  std::unique_ptr<api::networking_onc::GlobalPolicy> policy(
      api::networking_onc::GlobalPolicy::FromValue(*policy_dict));
  DCHECK(policy);
  return RespondNow(ArgumentList(
      api::networking_onc::GetGlobalPolicy::Results::Create(*policy)));
}

////////////////////////////////////////////////////////////////////////////////
// NetworkingOncGetCertificateListsFunction

NetworkingOncGetCertificateListsFunction::
    ~NetworkingOncGetCertificateListsFunction() {}

ExtensionFunction::ResponseAction
NetworkingOncGetCertificateListsFunction::Run() {
  if (!HasPrivateNetworkingAccess(extension(), source_context_type(),
                                  source_url())) {
    return RespondNow(Error(kPrivateOnlyError));
  }

  std::unique_ptr<base::DictionaryValue> certificate_lists(
      GetDelegate(browser_context())->GetCertificateLists());
  DCHECK(certificate_lists);
  return RespondNow(OneArgument(std::move(certificate_lists)));
}

}  // namespace extensions
