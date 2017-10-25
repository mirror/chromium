// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/component_updater/downloadable_strings_component_installer.h"

#include <algorithm>
#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "base/bind.h"
#include "base/feature_list.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/lazy_instance.h"
#include "base/logging.h"
#include "base/macros.h"
#include "base/memory/ptr_util.h"
#include "base/metrics/histogram_macros.h"
#include "base/path_service.h"
#include "base/strings/string_number_conversions.h"
#include "base/task_scheduler/post_task.h"
#include "base/version.h"
#include "build/build_config.h"
#include "components/component_updater/component_updater_paths.h"
#include "components/crx_file/id_util.h"
#include "components/startup_metric_utils/browser/startup_metric_utils.h"
#include "components/update_client/update_client_errors.h"

#if defined(OS_ANDROID)
#include "base/android/locale_utils.h"
#include "net/android/network_library.h"
#endif

using component_updater::ComponentUpdateService;

namespace {

// An enum class listing how/when the downloaded binary test file should be
// deleted. Valid values are:
//
//   NONE:
//     The test file is never deleted, so only one download per Chrome
//     install will be performed.
//
//   AFTER_INSTALL:
//     The file is deleted after it is installed. This will force a new
//     download on the next Chrome startup (and thus the collection of more
//     metrics). However, this could interfere with differential updates if a
//     new version of the payload is available. This setting is only valid if
//     it is possible to disable differential downloads on the server side
//     (which is ok for the experiment).
//
//  DELETE_BEFORE_REGISTRATION:
//     The file is deleted in a background task first, then component
//     registration will happen. This tends to push registration to a much
//     later time in the Chrome startup sequence, and also to increase the
//     install time dramatically (e.g. from 600 ms to 2500 ms!).
//
//     This number still corresponds to the time between registration and
//     installation, the only explanation for now is that this pushes the
//     update check to a later part of the Chrome startup phase, which is much
//     busier, and thus leaves less CPU cycles to the update tasks.
//
//  RENAME_BEFORE_REGISTRATION
//     The file/directory is renamed in a background task first, then
//     the component registration will happen. Then a new background task
//     is posted to delete the now-obsolete renamed directory. Since
//     renaming is much faster than deletion, this reduces the variance
//     introduced by delaying the registration.
//
enum class DeletionPolicy {
  NONE,
  AFTER_INSTALL,
  DELETE_BEFORE_REGISTRATION,
  RENAME_BEFORE_REGISTRATION,
};

// kDeletionPolicy is the deletion policy to adopt here.
// WARNING: See DeletionPolicy comments above before changing this value.
static constexpr DeletionPolicy kDeletionPolicy =
    DeletionPolicy::RENAME_BEFORE_REGISTRATION;

// Finch experiment configuration
const base::Feature kDownloadableStringsExperimentFeature = {
    "DownloadableStringsExperiment", base::FEATURE_DISABLED_BY_DEFAULT,
};

// Name of the binary test file
const base::FilePath::CharType kDownloadableStringsBinaryTestFileName[] =
    FILE_PATH_LITERAL("dummy-locale.pak");

// The SHA256 of the SubjectPublicKeyInfo used to sign the extension.
const uint8_t kPublicKeySHA256[32] = {
    0x5e, 0xea, 0xe8, 0x4b, 0xba, 0xe2, 0x2b, 0x8f, 0xeb, 0x60, 0x24,
    0x01, 0x09, 0x49, 0x92, 0xdf, 0x7b, 0xe9, 0x41, 0x50, 0xba, 0x97,
    0x39, 0x7e, 0x72, 0xdd, 0x7b, 0xc0, 0xd3, 0xb7, 0x04, 0xe6,
};

// Relative installation directory for this component.
const base::FilePath::CharType kRelativeInstallDir[] =
    FILE_PATH_LITERAL("DownloadableStrings");

const base::FilePath::CharType kRelativeInstallRenamedDir[] =
    FILE_PATH_LITERAL("DownloadableStrings.tmp");

// Used to indicate to the OnUpdateResult() function iff the component
// was properly installed. Unfortunately, this is necessary because in
// certain cases (e.g. corrupted CRX), the component updater will call the
// function with an error code 0, while no file was unpacked from the CRX.
bool g_component_ready = false;

// A structure used to record metrics associated with this component.
class Metrics {
 public:
  // Must be called when component registration starts, and before
  // forcing the on-demand update/download of the component.
  void RecordComponentRegistrationTime() {
    component_registration_ticks_ = base::TimeTicks::Now();
  }

  // Must be called once the install/update completes. |error_code| must be
  // the update_client::Error value passed to the OnDemandUpdate
  // callback.
  void RecordComponentInstallationTime(update_client::Error error) {
    component_installation_ticks_ = base::TimeTicks::Now();
    RecordUmaMetrics(error);
  }

 private:
  // Stable enum values sent to UMA for the DownloadableStrings experiment.
  // These are persisted to logs, unlike update_client::Error codes which
  // they are supposed to match. See ErrorCodeToUmaError() below, and the
  // DownloadableStringStatus definition in tools/histograms/enums.xml.
  enum class UmaError {
    NONE = 0,
    UPDATE_IN_PROGRESS = 1,
    UPDATE_CANCELED = 2,
    RETRY_LATER = 3,
    SERVICE_ERROR = 4,
    UPDATE_CHECK_ERROR = 5,

    UNKNOWN = 15,  // Not part of update_client::Error so far.
    COUNT          // DO NOT REMOVE
  };

// The list of locales supported by Chrome. Extracted from
// build/config/locales.gni.IMPORTANT: Because this is used to define
// a log-persistent enum class for UMA, ALWAYS APPEND NEW VALUES TO THE
// END OF THIS LIST, and never insert new ones.
//
// The |X| parameter must be a macro that will be invoked once per locale
// in the list.
#define CHROME_LOCALES_LIST(X)                                            \
  X(am) X(ar) X(bg) X(bn) X(ca) X(cs) X(da) X(de) X(el) X(en_GB) X(en_US) \
  X(es) X(es_419) X(et) X(fa) X(fi) X(fil) X(fr) X(gu) X(he) X(hi) X(hr)  \
  X(hu) X(id) X(it) X(ja) X(kn) X(ko) X(lt) X(lv) X(ml) X(mr) X(ms) X(nb) \
  X(nl) X(pl) X(pt_BR) X(pt_PT) X(ro) X(ru) X(sk) X(sl) X(sr) X(sv) X(sw) \
  X(ta) X(te) X(th) X(tr) X(uk) X(vi) X(zh_CN) X(zh_TW)

// An enum class assigning a numerical value to each known locale.
// The COUNT value returns the number of valid locale values, while
// the special UNKNOWN value is used to indicate the locale could not
// be determined for this device.
#define DECLARE_ENUM_VALUE(name) name,
  enum class UmaLocale {
    CHROME_LOCALES_LIST(DECLARE_ENUM_VALUE)
    COUNT,  // DO NOT REMOVE
    UNKNOWN = 500,
  };
#undef DECLARE_ENUM_VALUE

  // Sanity check.
  static_assert(static_cast<int>(UmaLocale::UNKNOWN) >
                    static_cast<int>(UmaLocale::COUNT),
                "Invalid value for UmaLocale::UNKNOWN");

  static constexpr int kLocaleCount = static_cast<int>(UmaLocale::COUNT);

  static const char* const kLocaleNames[kLocaleCount];

  static constexpr int kMaxLocaleScore = 100;

  // Compare the device's current |locale|, with the list of Chrome
  // supported locales. Valid formats for |locale| are '<country>', or
  // '<country>_<region>'. This returns a score (lower is better), where
  // 0 corresponds to an exact match. Partial matches happen when the
  // locale contains a region that is not supported by Chrome directly
  // (e.g. 'fr_FR' is matched to 'fr' with a score of 1, or 'en_IN'is
  // matched to 'en_GB' and 'en_US' with a score of 2).
  static int ScoreLocaleMatch(base::StringPiece locale,
                              base::StringPiece chrome_locale) {
    if (locale == chrome_locale) {
      // Direct match returns the best (lowest) score.
      return 0;
    }
    size_t locale_underscore_pos = locale.find('_');
    base::StringPiece locale_country =
        (locale_underscore_pos != std::string::npos)
            ? base::StringPiece(locale.data(), locale_underscore_pos)
            : locale;

    // If the input locale has a region, but matches the chrome locale
    // This helps match 'fr-FR' to 'fr', for example.
    if (locale_country == chrome_locale) {
      return 1;
    }

    size_t chrome_underscore_pos = chrome_locale.find('_');
    if (chrome_underscore_pos != base::StringPiece::npos) {
      base::StringPiece chrome_country =
          base::StringPiece(chrome_locale.data(), chrome_underscore_pos);

      if (locale_country == chrome_country) {
        return 2;
      }
    }
    return kMaxLocaleScore;
  }

  // Convert a locale name into the corresponding UmaLocale value, or
  // return UmaLocale::UNKNOWN if it is unknown. Accepts the following
  // input formats: "<lang>", "<lang>-<region>" and "<lang>_<region>".
  static UmaLocale LocaleNameToUmaLocale(const base::StringPiece locale_name) {
    std::string name(locale_name);
    std::replace(name.begin(), name.end(), '-', '_');

    // Try to
    int best_match = -1;
    int best_score = kMaxLocaleScore;
    for (int n = 0; n < kLocaleCount; ++n) {
      // First, perform a direct comparison.
      int score = ScoreLocaleMatch(name, kLocaleNames[n]);
      // NOTE: Use <= for the comparison instead of < to ensure that
      //       en_IN matches en_US instead of en_GB. Also works for 'pt'.
      if (score <= best_score) {
        best_match = n;
        best_score = score;
      }
    }
    if (best_match >= 0) {
      return static_cast<UmaLocale>(best_match);
    } else {
      return UmaLocale::UNKNOWN;
    }
  }

  static std::string UmaLocaleToString(UmaLocale locale) {
    const int locale_index = static_cast<int>(locale);
    if (locale_index >= 0 && locale_index <= kLocaleCount) {
      return kLocaleNames[locale_index];
    }
    return "unknown";
  }

  // Function used to convert an error code received by
  // RecordDownloadableStringsComponentInstallComplete() into a UmaError value.
  static UmaError ErrorCodeToUmaError(update_client::Error error) {
    using update_client::Error;

    switch (error) {
      case Error::NONE:
        return UmaError::NONE;
      case Error::UPDATE_IN_PROGRESS:
        return UmaError::UPDATE_IN_PROGRESS;
      case Error::UPDATE_CANCELED:
        return UmaError::UPDATE_CANCELED;
      case Error::RETRY_LATER:
        return UmaError::RETRY_LATER;
      case Error::SERVICE_ERROR:
        return UmaError::SERVICE_ERROR;
      case Error::UPDATE_CHECK_ERROR:
        return UmaError::UPDATE_CHECK_ERROR;
      default:
        return UmaError::UNKNOWN;
    }
  }

  void RecordRegistrationTime(base::TimeTicks ticks) {
    component_registration_ticks_ = ticks;
  }

  void RecordUmaMetrics(update_client::Error error) {
    UMA_HISTOGRAM_ENUMERATION("DownloadableStrings.Status",
                              ErrorCodeToUmaError(error), UmaError::COUNT);

    // Register times for the three following errors:
    //   NONE:                Succesful download + install of the test file.
    //   RETRY_LATER:         Timeout value after failure to download the file.
    //   SERVICE_ERROR:       Timeout value after a connectivity issue.
    //   UPDATE_CHECK_ERROR:  Timeout value after an update check failure
    //                        (before the actual download).
    //
    // Other error codes (UPDATE_IN_PROGRESS and UPDATE_CANCELED) are not
    // related to network conditions, but to misuse of the update_client
    // API that should not apply during this experiment.

    const base::TimeDelta time_delta =
        component_installation_ticks_ - component_registration_ticks_;

    switch (error) {
      case update_client::Error::NONE:
        UMA_HISTOGRAM_TIMES("DownloadableStrings.InstallTime", time_delta);
        break;

      case update_client::Error::RETRY_LATER:
        UMA_HISTOGRAM_TIMES("DownloadableStrings.RetryLaterTimeout",
                            time_delta);
        break;

      case update_client::Error::SERVICE_ERROR:
        UMA_HISTOGRAM_TIMES("DownloadableStrings.ServiceErrorTimeout",
                            time_delta);
        break;

      case update_client::Error::UPDATE_CHECK_ERROR:
        UMA_HISTOGRAM_TIMES("DownloadableStrings.UpdateCheckTimeout",
                            time_delta);
        break;

      default:
        break;
    }

#if defined(OS_ANDROID)
    // The value returned by GetTelephonySimOperator() is normally a string
    // containing 5 or 6 decimal digits. The first 3 digits are the
    // Mobile Country Code (MCC), and the remaining 2 or 3 digits are the
    // Mobile Network Code (MNC), which is not useful here. However, the
    // Android documentation for the corresponding API [1] states that the
    // result is unreliable for CDMA networks (since there are no SIMs in
    // CDMA devices). Perform a little sanity check on the value to be
    // sure.
    //
    // [1] TelephonyManager.getSimOperator().
    int mobile_country_code = 0;
    const std::string sim_mcc_mnc = net::android::GetTelephonySimOperator();
    if (sim_mcc_mnc.size() < 5 || sim_mcc_mnc.size() > 6 ||
        !std::all_of(sim_mcc_mnc.begin(), sim_mcc_mnc.end(), ::isdigit) ||
        !base::StringToInt(base::StringPiece(&sim_mcc_mnc[0], 3U),
                           &mobile_country_code)) {
      LOG(ERROR) << "Invalid MCC+MNC string [" << sim_mcc_mnc << "]";
      mobile_country_code = 0;
    }

    // Real-world mobile country codes start at 200 and end at 999,
    // see https://en.wikipedia.org/wiki/Mobile_country_code
    // The value 0 means "undetermined" in this histogram.
    if (mobile_country_code != 0 &&
        (mobile_country_code < 200 || mobile_country_code > 999)) {
      LOG(ERROR) << "Mobile country code is out of range "
                 << mobile_country_code << "(should be in [200..999])";
      mobile_country_code = 0;
    }

    const std::string default_locale = base::android::GetDefaultLocaleString();
    const UmaLocale uma_locale = LocaleNameToUmaLocale(default_locale);
    if (uma_locale == UmaLocale::UNKNOWN) {
      LOG(ERROR) << "Unknown device locale [" << default_locale << "]";
    }
#else   // !OS_ANDROID
    const int mobile_country_code = 0;
    const UmaLocale uma_locale = UmaLocale::UNKNOWN;
#endif  // !OS_ANDROID

    UMA_HISTOGRAM_SPARSE_SLOWLY("DownloadableStrings.SimCountryCode",
                                mobile_country_code);

    UMA_HISTOGRAM_SPARSE_SLOWLY("DownloadableStrings.LocaleIndex",
                                static_cast<int>(uma_locale));

    const base::TimeTicks startup_ticks =
        startup_metric_utils::MainEntryPointTicks();

    const base::TimeDelta registration_time =
        component_registration_ticks_ - startup_ticks;

    const base::TimeDelta installation_time =
        component_installation_ticks_ - startup_ticks;

    VLOG(1) << "DownloadableStrings metrics: registration @"
            << registration_time.InMillisecondsF() << " ms, install @"
            << installation_time.InMillisecondsF() << " ms (duration "
            << time_delta.InMillisecondsF()
            << " ms) country_code=" << mobile_country_code
            << " locale=" << UmaLocaleToString(uma_locale) << " (index "
            << static_cast<int>(uma_locale) << ")";
  }

  base::TimeTicks component_registration_ticks_;
  base::TimeTicks component_installation_ticks_;
};

#define DECLARE_STRING_VALUE(name) #name,
// static
const char* const Metrics::kLocaleNames[] = {
    CHROME_LOCALES_LIST(DECLARE_STRING_VALUE)};
#undef DECLARE_STRING_VALUE

base::LazyInstance<Metrics>::Leaky g_metrics = LAZY_INSTANCE_INITIALIZER;

// Delete the installed test files for this experiment.
void DeleteExperimentTestFiles(const base::FilePath::CharType* relative_path) {
  const base::TimeTicks deletion_start = base::TimeTicks::Now();
  base::FilePath component_root;
  if (!base::PathService::Get(component_updater::DIR_COMPONENT_USER,
                              &component_root)) {
    LOG(ERROR) << "Could not find root directory for components.";
    return;
  }
  base::FilePath install_dir = component_root.Append(relative_path);
  ignore_result(base::DeleteFile(install_dir, true /* recursive */));
  const base::TimeDelta deletion_time = base::TimeTicks::Now() - deletion_start;
  VLOG(1) << "Deletion time " << deletion_time.InMillisecondsF() << " ms";
}

void RenameComponentDirectory() {
  const base::TimeTicks rename_start = base::TimeTicks::Now();
  base::FilePath component_root;
  if (!base::PathService::Get(component_updater::DIR_COMPONENT_USER,
                              &component_root)) {
    LOG(ERROR) << "Could not find root directory for components.";
    return;
  }
  base::FilePath src_dir = component_root.Append(kRelativeInstallDir);
  base::FilePath dst_dir = component_root.Append(kRelativeInstallRenamedDir);
  if (!base::Move(src_dir, dst_dir)) {
    // Not an error because this can fail if the directory does not exist yet.
    VLOG(1) << "Could not rename component directory";
  }
  const base::TimeDelta rename_time = base::TimeTicks::Now() - rename_start;
  VLOG(1) << "Rename time " << rename_time.InMillisecondsF() << " ms";
}

}  // namespace

namespace component_updater {

bool DownloadableStringsComponentInstallerPolicy::
    SupportsGroupPolicyEnabledComponentUpdates() const {
  return false;
}

bool DownloadableStringsComponentInstallerPolicy::RequiresNetworkEncryption()
    const {
  return false;
}

update_client::CrxInstaller::Result
DownloadableStringsComponentInstallerPolicy::OnCustomInstall(
    const base::DictionaryValue& manifest,
    const base::FilePath& install_dir) {
  return update_client::CrxInstaller::Result(0);  // Nothing custom here.
}

base::FilePath DownloadableStringsComponentInstallerPolicy::GetInstalledPath(
    const base::FilePath& base) {
  return base.Append(kDownloadableStringsBinaryTestFileName);
}

void DownloadableStringsComponentInstallerPolicy::ComponentReady(
    const base::Version& version,
    const base::FilePath& install_dir,
    std::unique_ptr<base::DictionaryValue> manifest) {
  VLOG(1) << "Component ready, version " << version.GetString() << " in "
          << install_dir.value();

  g_component_ready = true;

  g_metrics.Pointer()->RecordComponentInstallationTime(
      update_client::Error::NONE);

  if (kDeletionPolicy == DeletionPolicy::AFTER_INSTALL) {
    // Delete the file in a background task after installation.
    base::PostTaskWithTraits(
        FROM_HERE, {base::TaskPriority::BACKGROUND, base::MayBlock()},
        base::BindOnce(&DeleteExperimentTestFiles,
                       base::Unretained(kRelativeInstallDir)));
  }
  if (kDeletionPolicy == DeletionPolicy::RENAME_BEFORE_REGISTRATION) {
    base::PostTaskWithTraits(
        FROM_HERE, {base::TaskPriority::BACKGROUND, base::MayBlock()},
        base::BindOnce(&DeleteExperimentTestFiles, kRelativeInstallRenamedDir));
  }
}

// Called during startup and installation before ComponentReady().
bool DownloadableStringsComponentInstallerPolicy::VerifyInstallation(
    const base::DictionaryValue& manifest,
    const base::FilePath& install_dir) const {
  // No need to actually validate the file here.
  return base::PathExists(GetInstalledPath(install_dir));
}

base::FilePath
DownloadableStringsComponentInstallerPolicy::GetRelativeInstallDir() const {
  return base::FilePath(kRelativeInstallDir);
}

void DownloadableStringsComponentInstallerPolicy::GetHash(
    std::vector<uint8_t>* hash) const {
  hash->assign(kPublicKeySHA256,
               kPublicKeySHA256 + arraysize(kPublicKeySHA256));
}

std::string DownloadableStringsComponentInstallerPolicy::GetName() const {
  return "Downloadable Strings";
}

update_client::InstallerAttributes
DownloadableStringsComponentInstallerPolicy::GetInstallerAttributes() const {
  return update_client::InstallerAttributes();
}

std::vector<std::string>
DownloadableStringsComponentInstallerPolicy::GetMimeTypes() const {
  return {};
}

static void OnUpdateResult(update_client::Error error) {
  if (error == update_client::Error::NONE && !g_component_ready) {
    // The component was not downloaded, even though no error was reported.
    // This can happen under certain circumstances (e.g. corrupted CRX file).
    error = update_client::Error::UPDATE_CHECK_ERROR;
  }
  if (error != update_client::Error::NONE) {
    LOG(ERROR) << "Error: " << static_cast<int>(error);
    g_metrics.Pointer()->RecordComponentInstallationTime(error);

    if (kDeletionPolicy == DeletionPolicy::RENAME_BEFORE_REGISTRATION) {
      base::PostTaskWithTraits(
          FROM_HERE, {base::TaskPriority::BACKGROUND, base::MayBlock()},
          base::BindOnce(&DeleteExperimentTestFiles, kRelativeInstallRenamedDir));
    }
  }
}

// static
void DownloadableStringsComponentInstallerPolicy::TriggerComponentUpdate(
    OnDemandUpdater* updater) {
  const std::string crx_id = crx_file::id_util::GenerateIdFromHash(
      kPublicKeySHA256, sizeof(kPublicKeySHA256));
  DCHECK_EQ(crx_id, "fookoiellkocclipolgaceabajejjcnp");
  VLOG(1) << "Triggering component update";
  updater->OnDemandUpdate(crx_id, base::Bind(&OnUpdateResult));
}

// static
void DownloadableStringsComponentInstallerPolicy::DoRegistration(
    ComponentUpdateService* cus) {
  g_metrics.Pointer()->RecordComponentRegistrationTime();
  VLOG(1) << "Registering Downloadable Strings component.";

  auto policy = base::MakeUnique<DownloadableStringsComponentInstallerPolicy>();
  // |cus| will take nership of |installer| during installer->Register(cus).
  ComponentInstaller* installer = new ComponentInstaller(std::move(policy));
  installer->Register(
      cus,
      base::Bind(
          &DownloadableStringsComponentInstallerPolicy::TriggerComponentUpdate,
          &cus->GetOnDemandUpdater()));
}

void RegisterDownloadableStringsComponent(ComponentUpdateService* cus) {
  // Return immediately if the feature is not enabled. Otherwise, register
  // the component and immediately trigger an update.
  if (!base::FeatureList::IsEnabled(kDownloadableStringsExperimentFeature)) {
    return;
  }

  // Delete the file in a background task before registering the component,
  // to always force a download. This is better than doing the delete in
  // ::ComponentReady(), because the latter could interfere with differential
  // updates. This delays the actual component registration to later, but this
  // is not important for this experiment.
  //
  // TODO(digit): Once the experiment is removed, and the full feature
  // implemented, call DoRegistration() directly (or inline its content
  // directly here).

  if (kDeletionPolicy == DeletionPolicy::RENAME_BEFORE_REGISTRATION) {
    base::PostTaskWithTraitsAndReply(
        FROM_HERE, {base::TaskPriority::BACKGROUND, base::MayBlock()},
        base::BindOnce(&RenameComponentDirectory),
        base::BindOnce(
            &DownloadableStringsComponentInstallerPolicy::DoRegistration,
            base::Unretained(cus)));
    return;
  }

  if (kDeletionPolicy == DeletionPolicy::DELETE_BEFORE_REGISTRATION) {
    // Delete in a background task, then perform component registration.
    base::PostTaskWithTraitsAndReply(
        FROM_HERE, {base::TaskPriority::BACKGROUND, base::MayBlock()},
        base::BindOnce(&DeleteExperimentTestFiles,
                       base::Unretained(kRelativeInstallDir)),
        base::BindOnce(
            &DownloadableStringsComponentInstallerPolicy::DoRegistration,
            base::Unretained(cus)));
    return;
  }

  DownloadableStringsComponentInstallerPolicy::DoRegistration(cus);
}

}  // namespace component_updater
