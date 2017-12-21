// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/renderer/media/chrome_key_systems.h"

#include <stddef.h>

#include <string>
#include <vector>

#include "base/logging.h"
#include "base/strings/string16.h"
#include "base/strings/string_split.h"
#include "base/strings/utf_string_conversions.h"
#include "build/build_config.h"
#include "chrome/renderer/chrome_render_thread_observer.h"
#include "components/cdm/renderer/external_clear_key_key_system_properties.h"
#include "components/cdm/renderer/widevine_key_system_properties.h"
#include "content/public/renderer/render_thread.h"
#include "media/base/eme_constants.h"
#include "media/base/key_system_properties.h"
#include "media/media_features.h"

#if defined(OS_ANDROID)
#include "components/cdm/renderer/android_key_systems.h"
#endif

#if BUILDFLAG(ENABLE_LIBRARY_CDMS)
#include "base/feature_list.h"
#include "base/macros.h"
#include "content/public/common/service_names.mojom.h"
#include "media/base/media_switches.h"
#include "media/mojo/interfaces/key_system_info.mojom.h"
#include "services/service_manager/public/cpp/connector.h"
#endif

#include "widevine_cdm_version.h" // In SHARED_INTERMEDIATE_DIR.

// The following must be after widevine_cdm_version.h.

#if defined(WIDEVINE_CDM_AVAILABLE) && defined(WIDEVINE_CDM_MIN_GLIBC_VERSION)
#include <gnu/libc-version.h>
#include "base/version.h"
#endif

using media::EmeFeatureSupport;
using media::EmeSessionTypeSupport;
using media::KeySystemProperties;
using media::SupportedCodecs;

#if BUILDFLAG(ENABLE_LIBRARY_CDMS)

// Obtain a reference to the KeySystemInfo host.
static media::mojom::KeySystemInfoPtr& GetKeySystemInfoHost() {
  struct KeySystemInfoHostHolder {
    KeySystemInfoHostHolder() {
      content::RenderThread::Get()->GetConnector()->BindInterface(
          content::mojom::kBrowserServiceName,
          mojo::MakeRequest(&key_system_info_host));
    }
    ~KeySystemInfoHostHolder() {}
    media::mojom::KeySystemInfoPtr key_system_info_host;
  };

  CR_DEFINE_STATIC_LOCAL(KeySystemInfoHostHolder, holder, ());
  return holder.key_system_info_host;
}

// Determine if |key_system| is available by calling into the browser.
// If it is available, return true and |supported_codecs| and
// |supports_persistent_license| are updated to match what |key_system|
// supports. If not available, false is returned.
static bool IsCdmAvailable(const std::string& key_system,
                           std::vector<std::string>* supported_codecs,
                           bool* supports_persistent_license) {
  DVLOG(3) << __func__ << " key_system: " << key_system;
  bool is_available = false;

  GetKeySystemInfoHost()->IsKeySystemAvailable(
      key_system, &is_available, supported_codecs, supports_persistent_license);
  return is_available;
}

// External Clear Key (used for testing).
static void AddExternalClearKey(
    std::vector<std::unique_ptr<KeySystemProperties>>* concrete_key_systems) {
  // TODO(xhwang): Move these into an array so we can use a for loop to add
  // supported key systems below.
  static const char kExternalClearKeyKeySystem[] =
      "org.chromium.externalclearkey";
  static const char kExternalClearKeyDecryptOnlyKeySystem[] =
      "org.chromium.externalclearkey.decryptonly";
  static const char kExternalClearKeyRenewalKeySystem[] =
      "org.chromium.externalclearkey.renewal";
  static const char kExternalClearKeyFileIOTestKeySystem[] =
      "org.chromium.externalclearkey.fileiotest";
  static const char kExternalClearKeyOutputProtectionTestKeySystem[] =
      "org.chromium.externalclearkey.outputprotectiontest";
  static const char kExternalClearKeyPlatformVerificationTestKeySystem[] =
      "org.chromium.externalclearkey.platformverificationtest";
  static const char kExternalClearKeyInitializeFailKeySystem[] =
      "org.chromium.externalclearkey.initializefail";
  static const char kExternalClearKeyCrashKeySystem[] =
      "org.chromium.externalclearkey.crash";
  static const char kExternalClearKeyVerifyCdmHostTestKeySystem[] =
      "org.chromium.externalclearkey.verifycdmhosttest";
  static const char kExternalClearKeyStorageIdTestKeySystem[] =
      "org.chromium.externalclearkey.storageidtest";
  static const char kExternalClearKeyDifferentGuidTestKeySystem[] =
      "org.chromium.externalclearkey.differentguid";

  std::vector<std::string> supported_codecs;
  bool supports_persistent_license;
  if (!IsCdmAvailable(kExternalClearKeyKeySystem, &supported_codecs,
                      &supports_persistent_license)) {
    return;
  }

  concrete_key_systems->emplace_back(
      new cdm::ExternalClearKeyProperties(kExternalClearKeyKeySystem));

  // Add support of decrypt-only mode in ClearKeyCdm.
  concrete_key_systems->emplace_back(new cdm::ExternalClearKeyProperties(
      kExternalClearKeyDecryptOnlyKeySystem));

  // A key system that triggers renewal message in ClearKeyCdm.
  concrete_key_systems->emplace_back(
      new cdm::ExternalClearKeyProperties(kExternalClearKeyRenewalKeySystem));

  // A key system that triggers the FileIO test in ClearKeyCdm.
  concrete_key_systems->emplace_back(new cdm::ExternalClearKeyProperties(
      kExternalClearKeyFileIOTestKeySystem));

  // A key system that triggers the output protection test in ClearKeyCdm.
  concrete_key_systems->emplace_back(new cdm::ExternalClearKeyProperties(
      kExternalClearKeyOutputProtectionTestKeySystem));

  // A key system that triggers the platform verification test in ClearKeyCdm.
  concrete_key_systems->emplace_back(new cdm::ExternalClearKeyProperties(
      kExternalClearKeyPlatformVerificationTestKeySystem));

  // A key system that Chrome thinks is supported by ClearKeyCdm, but actually
  // will be refused by ClearKeyCdm. This is to test the CDM initialization
  // failure case.
  concrete_key_systems->emplace_back(new cdm::ExternalClearKeyProperties(
      kExternalClearKeyInitializeFailKeySystem));

  // A key system that triggers a crash in ClearKeyCdm.
  concrete_key_systems->emplace_back(
      new cdm::ExternalClearKeyProperties(kExternalClearKeyCrashKeySystem));

  // A key system that triggers the verify host files test in ClearKeyCdm.
  concrete_key_systems->emplace_back(new cdm::ExternalClearKeyProperties(
      kExternalClearKeyVerifyCdmHostTestKeySystem));

  // A key system that fetches the Storage ID in ClearKeyCdm.
  concrete_key_systems->emplace_back(new cdm::ExternalClearKeyProperties(
      kExternalClearKeyStorageIdTestKeySystem));

  // A key system that is registered with a different CDM GUID.
  concrete_key_systems->emplace_back(new cdm::ExternalClearKeyProperties(
      kExternalClearKeyDifferentGuidTestKeySystem));
}

#if defined(WIDEVINE_CDM_AVAILABLE)
// Returns persistent-license session support.
EmeSessionTypeSupport GetPersistentLicenseSupport(bool supported_by_the_cdm) {
  // Do not support persistent-license if the process cannot persist data.
  // TODO(crbug.com/457487): Have a better plan on this. See bug for details.
  if (ChromeRenderThreadObserver::is_incognito_process()) {
    DVLOG(2) << __func__ << ": Not supported in incognito process.";
    return EmeSessionTypeSupport::NOT_SUPPORTED;
  }

  if (!supported_by_the_cdm) {
    DVLOG(2) << __func__ << ": Not supported by the CDM.";
    return EmeSessionTypeSupport::NOT_SUPPORTED;
  }

// On ChromeOS, platform verification is similar to CDM host verification.
#if BUILDFLAG(ENABLE_CDM_HOST_VERIFICATION) || defined(OS_CHROMEOS)
  bool cdm_host_verification_potentially_supported = true;
#else
  bool cdm_host_verification_potentially_supported = false;
#endif

  // If we are sure CDM host verification is NOT supported, we should not
  // support persistent-license.
  if (!cdm_host_verification_potentially_supported) {
    DVLOG(2) << __func__ << ": Not supported without CDM host verification.";
    return EmeSessionTypeSupport::NOT_SUPPORTED;
  }

#if defined(OS_CHROMEOS)
  // On ChromeOS, platform verification (similar to CDM host verification)
  // requires identifier to be allowed.
  // TODO(jrummell): Currently the ChromeOS CDM does not require storage ID
  // to support persistent license. Update this logic when the new CDM requires
  // storage ID.
  return EmeSessionTypeSupport::SUPPORTED_WITH_IDENTIFIER;
#elif BUILDFLAG(ENABLE_CDM_STORAGE_ID)
  // On other platforms, we require storage ID to support persistent license.
  return EmeSessionTypeSupport::SUPPORTED;
#else
  // Storage ID not implemented, so no support for persistent license.
  DVLOG(2) << __func__ << ": Not supported without CDM storage ID.";
  return EmeSessionTypeSupport::NOT_SUPPORTED;
#endif  // defined(OS_CHROMEOS)
}

static void AddPepperBasedWidevine(
    std::vector<std::unique_ptr<KeySystemProperties>>* concrete_key_systems) {
#if defined(WIDEVINE_CDM_MIN_GLIBC_VERSION)
  base::Version glibc_version(gnu_get_libc_version());
  DCHECK(glibc_version.IsValid());
  if (glibc_version < base::Version(WIDEVINE_CDM_MIN_GLIBC_VERSION))
    return;
#endif  // defined(WIDEVINE_CDM_MIN_GLIBC_VERSION)

  std::vector<std::string> codecs;
  bool supports_persistent_license = false;
  if (!IsCdmAvailable(kWidevineKeySystem, &codecs,
                      &supports_persistent_license)) {
    DVLOG(1) << "Widevine CDM is not currently available.";
    return;
  }

  SupportedCodecs supported_codecs = media::EME_CODEC_NONE;

  // Audio codecs are always supported.
  // TODO(sandersd): Distinguish these from those that are directly supported,
  // as those may offer a higher level of protection.
  supported_codecs |= media::EME_CODEC_WEBM_OPUS;
  supported_codecs |= media::EME_CODEC_WEBM_VORBIS;
#if BUILDFLAG(USE_PROPRIETARY_CODECS)
  supported_codecs |= media::EME_CODEC_MP4_AAC;
#endif  // BUILDFLAG(USE_PROPRIETARY_CODECS)

  // Video codecs are determined by what was registered for the CDM.
  for (const auto& codec : codecs) {
    if (codec == kCdmSupportedCodecVp8) {
      supported_codecs |= media::EME_CODEC_WEBM_VP8;
    }
    if (codec == kCdmSupportedCodecVp9) {
      supported_codecs |= media::EME_CODEC_WEBM_VP9;
      supported_codecs |= media::EME_CODEC_COMMON_VP9;
    }
#if BUILDFLAG(USE_PROPRIETARY_CODECS)
    if (codec == kCdmSupportedCodecAvc1) {
      supported_codecs |= media::EME_CODEC_MP4_AVC1;
    }
#endif  // BUILDFLAG(USE_PROPRIETARY_CODECS)
  }

  EmeSessionTypeSupport persistent_license_support =
      GetPersistentLicenseSupport(supports_persistent_license);

  using Robustness = cdm::WidevineKeySystemProperties::Robustness;

  concrete_key_systems->emplace_back(new cdm::WidevineKeySystemProperties(
      supported_codecs,
#if defined(OS_CHROMEOS)
      Robustness::HW_SECURE_ALL,             // Maximum audio robustness.
      Robustness::HW_SECURE_ALL,             // Maximum video robustness.
      persistent_license_support,            // Persistent-license.
      EmeSessionTypeSupport::NOT_SUPPORTED,  // Persistent-release-message.
      EmeFeatureSupport::REQUESTABLE,        // Persistent state.
      EmeFeatureSupport::REQUESTABLE));      // Distinctive identifier.
#else                                        // Desktop
      Robustness::SW_SECURE_CRYPTO,          // Maximum audio robustness.
      Robustness::SW_SECURE_DECODE,          // Maximum video robustness.
      persistent_license_support,            // persistent-license.
      EmeSessionTypeSupport::NOT_SUPPORTED,  // persistent-release-message.
      EmeFeatureSupport::REQUESTABLE,        // Persistent state.
      EmeFeatureSupport::NOT_SUPPORTED));    // Distinctive identifier.
#endif                                       // defined(OS_CHROMEOS)
}
#endif  // defined(WIDEVINE_CDM_AVAILABLE)
#endif  // BUILDFLAG(ENABLE_LIBRARY_CDMS)

void AddChromeKeySystems(
    std::vector<std::unique_ptr<KeySystemProperties>>* key_systems_properties) {
#if BUILDFLAG(ENABLE_LIBRARY_CDMS)
  if (base::FeatureList::IsEnabled(media::kExternalClearKeyForTesting))
    AddExternalClearKey(key_systems_properties);

#if defined(WIDEVINE_CDM_AVAILABLE)
  AddPepperBasedWidevine(key_systems_properties);
#endif  // defined(WIDEVINE_CDM_AVAILABLE)

#endif  // BUILDFLAG(ENABLE_LIBRARY_CDMS)

#if defined(OS_ANDROID)
  cdm::AddAndroidWidevine(key_systems_properties);
#endif  // defined(OS_ANDROID)
}
