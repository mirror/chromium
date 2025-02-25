// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "extensions/browser/sandboxed_unpacker.h"

#include <stddef.h>
#include <stdint.h>

#include <set>
#include <tuple>
#include <utility>
#include <vector>

#include "base/bind.h"
#include "base/command_line.h"
#include "base/files/file_enumerator.h"
#include "base/files/file_util.h"
#include "base/i18n/rtl.h"
#include "base/json/json_string_value_serializer.h"
#include "base/metrics/histogram_macros.h"
#include "base/path_service.h"
#include "base/sequenced_task_runner.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/utf_string_conversions.h"
#include "build/build_config.h"
#include "components/crx_file/crx_verifier.h"
#include "content/public/browser/browser_thread.h"
#include "extensions/browser/api/declarative_net_request/utils.h"
#include "extensions/browser/extension_file_task_runner.h"
#include "extensions/common/api/declarative_net_request/dnr_manifest_data.h"
#include "extensions/common/constants.h"
#include "extensions/common/extension.h"
#include "extensions/common/extension_l10n_util.h"
#include "extensions/common/extension_resource_path_normalizer.h"
#include "extensions/common/extension_unpacker.mojom.h"
#include "extensions/common/extension_utility_types.h"
#include "extensions/common/extensions_client.h"
#include "extensions/common/features/feature_channel.h"
#include "extensions/common/features/feature_session_type.h"
#include "extensions/common/file_util.h"
#include "extensions/common/manifest_constants.h"
#include "extensions/common/manifest_handlers/default_locale_handler.h"
#include "extensions/common/manifest_handlers/icons_handler.h"
#include "extensions/common/switches.h"
#include "extensions/strings/grit/extensions_strings.h"
#include "services/data_decoder/public/interfaces/constants.mojom.h"
#include "services/service_manager/public/cpp/connector.h"
#include "third_party/skia/include/core/SkBitmap.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/gfx/codec/png_codec.h"

using base::ASCIIToUTF16;
using content::BrowserThread;

// The following macro makes histograms that record the length of paths
// in this file much easier to read.
// Windows has a short max path length. If the path length to a
// file being unpacked from a CRX exceeds the max length, we might
// fail to install. To see if this is happening, see how long the
// path to the temp unpack directory is. See crbug.com/69693 .
#define PATH_LENGTH_HISTOGRAM(name, path) \
  UMA_HISTOGRAM_CUSTOM_COUNTS(name, path.value().length(), 1, 500, 100)

// Record a rate (kB per second) at which extensions are unpacked.
// Range from 1kB/s to 100mB/s.
#define UNPACK_RATE_HISTOGRAM(name, rate) \
  UMA_HISTOGRAM_CUSTOM_COUNTS(name, rate, 1, 100000, 100);

namespace extensions {
namespace {

void RecordSuccessfulUnpackTimeHistograms(const base::FilePath& crx_path,
                                          const base::TimeDelta unpack_time) {
  const int64_t kBytesPerKb = 1024;
  const int64_t kBytesPerMb = 1024 * 1024;

  UMA_HISTOGRAM_TIMES("Extensions.SandboxUnpackSuccessTime", unpack_time);

  // To get a sense of how CRX size impacts unpack time, record unpack
  // time for several increments of CRX size.
  int64_t crx_file_size;
  if (!base::GetFileSize(crx_path, &crx_file_size)) {
    UMA_HISTOGRAM_COUNTS("Extensions.SandboxUnpackSuccessCantGetCrxSize", 1);
    return;
  }

  // Cast is safe as long as the number of bytes in the CRX is less than
  // 2^31 * 2^10.
  int crx_file_size_kb = static_cast<int>(crx_file_size / kBytesPerKb);
  UMA_HISTOGRAM_COUNTS("Extensions.SandboxUnpackSuccessCrxSize",
                       crx_file_size_kb);

  // We have time in seconds and file size in bytes.  We want the rate bytes are
  // unpacked in kB/s.
  double file_size_kb =
      static_cast<double>(crx_file_size) / static_cast<double>(kBytesPerKb);
  int unpack_rate_kb_per_s =
      static_cast<int>(file_size_kb / unpack_time.InSecondsF());
  UNPACK_RATE_HISTOGRAM("Extensions.SandboxUnpackRate", unpack_rate_kb_per_s);

  if (crx_file_size < 50.0 * kBytesPerKb) {
    UNPACK_RATE_HISTOGRAM("Extensions.SandboxUnpackRateUnder50kB",
                          unpack_rate_kb_per_s);

  } else if (crx_file_size < 1 * kBytesPerMb) {
    UNPACK_RATE_HISTOGRAM("Extensions.SandboxUnpackRate50kBTo1mB",
                          unpack_rate_kb_per_s);

  } else if (crx_file_size < 2 * kBytesPerMb) {
    UNPACK_RATE_HISTOGRAM("Extensions.SandboxUnpackRate1To2mB",
                          unpack_rate_kb_per_s);

  } else if (crx_file_size < 5 * kBytesPerMb) {
    UNPACK_RATE_HISTOGRAM("Extensions.SandboxUnpackRate2To5mB",
                          unpack_rate_kb_per_s);

  } else if (crx_file_size < 10 * kBytesPerMb) {
    UNPACK_RATE_HISTOGRAM("Extensions.SandboxUnpackRate5To10mB",
                          unpack_rate_kb_per_s);

  } else {
    UNPACK_RATE_HISTOGRAM("Extensions.SandboxUnpackRateOver10mB",
                          unpack_rate_kb_per_s);
  }
}

// Work horse for FindWritableTempLocation. Creates a temp file in the folder
// and uses NormalizeFilePath to check if the path is junction free.
bool VerifyJunctionFreeLocation(base::FilePath* temp_dir) {
  if (temp_dir->empty())
    return false;

  base::FilePath temp_file;
  if (!base::CreateTemporaryFileInDir(*temp_dir, &temp_file)) {
    LOG(ERROR) << temp_dir->value() << " is not writable";
    return false;
  }

  // NormalizeFilePath requires a non-empty file, so write some data.
  // If you change the exit points of this function please make sure all
  // exit points delete this temp file!
  if (base::WriteFile(temp_file, ".", 1) != 1) {
    base::DeleteFile(temp_file, false);
    return false;
  }

  base::FilePath normalized_temp_file;
  bool normalized = base::NormalizeFilePath(temp_file, &normalized_temp_file);
  if (!normalized) {
    // If |temp_file| contains a link, the sandbox will block all file
    // system operations, and the install will fail.
    LOG(ERROR) << temp_dir->value() << " seem to be on remote drive.";
  } else {
    *temp_dir = normalized_temp_file.DirName();
  }

  // Clean up the temp file.
  base::DeleteFile(temp_file, false);

  return normalized;
}

// This function tries to find a location for unpacking the extension archive
// that is writable and does not lie on a shared drive so that the sandboxed
// unpacking process can write there. If no such location exists we can not
// proceed and should fail.
// The result will be written to |temp_dir|. The function will write to this
// parameter even if it returns false.
bool FindWritableTempLocation(const base::FilePath& extensions_dir,
                              base::FilePath* temp_dir) {
// On ChromeOS, we will only attempt to unpack extension in cryptohome (profile)
// directory to provide additional security/privacy and speed up the rest of
// the extension install process.
#if !defined(OS_CHROMEOS)
  PathService::Get(base::DIR_TEMP, temp_dir);
  if (VerifyJunctionFreeLocation(temp_dir))
    return true;
#endif

  *temp_dir = file_util::GetInstallTempDir(extensions_dir);
  if (VerifyJunctionFreeLocation(temp_dir))
    return true;
  // Neither paths is link free chances are good installation will fail.
  LOG(ERROR) << "Both the %TEMP% folder and the profile seem to be on "
             << "remote drives or read-only. Installation can not complete!";
  return false;
}

std::set<base::FilePath> GetMessageCatalogPathsToBeSanitized(
    const base::FilePath& locales_path) {
  // Not all folders under _locales have to be valid locales.
  base::FileEnumerator locales(locales_path, /*recursive=*/false,
                               base::FileEnumerator::DIRECTORIES);

  std::set<base::FilePath> message_catalog_paths;
  std::set<std::string> all_locales;
  extension_l10n_util::GetAllLocales(&all_locales);
  base::FilePath locale_path;
  while (!(locale_path = locales.Next()).empty()) {
    if (!extension_l10n_util::ShouldSkipValidation(locales_path, locale_path,
                                                   all_locales)) {
      message_catalog_paths.insert(locale_path.Append(kMessagesFilename));
    }
  }
  return message_catalog_paths;
}

}  // namespace

SandboxedUnpackerClient::SandboxedUnpackerClient()
    : RefCountedDeleteOnSequence<SandboxedUnpackerClient>(
          content::BrowserThread::GetTaskRunnerForThread(
              content::BrowserThread::UI)) {
  DCHECK_CURRENTLY_ON(BrowserThread::UI);
}

SandboxedUnpacker::SandboxedUnpacker(
    std::unique_ptr<service_manager::Connector> connector,
    Manifest::Location location,
    int creation_flags,
    const base::FilePath& extensions_dir,
    const scoped_refptr<base::SequencedTaskRunner>& unpacker_io_task_runner,
    SandboxedUnpackerClient* client)
    : connector_(std::move(connector)),
      client_(client),
      extensions_dir_(extensions_dir),
      location_(location),
      creation_flags_(creation_flags),
      unpacker_io_task_runner_(unpacker_io_task_runner) {
  // Tracking for crbug.com/692069. The location must be valid. If it's invalid,
  // the utility process kills itself for a bad IPC.
  CHECK_GT(location, Manifest::INVALID_LOCATION);
  CHECK_LT(location, Manifest::NUM_LOCATIONS);

  // Use a random instance ID to guarantee the connection is to a new data
  // decoder service (running in its own process).
  data_decoder_identity_ = service_manager::Identity(
      data_decoder::mojom::kServiceName, service_manager::mojom::kInheritUserID,
      base::UnguessableToken::Create().ToString());
}

bool SandboxedUnpacker::CreateTempDirectory() {
  DCHECK(unpacker_io_task_runner_->RunsTasksInCurrentSequence());

  base::FilePath temp_dir;
  if (!FindWritableTempLocation(extensions_dir_, &temp_dir)) {
    ReportFailure(COULD_NOT_GET_TEMP_DIRECTORY,
                  l10n_util::GetStringFUTF16(
                      IDS_EXTENSION_PACKAGE_INSTALL_ERROR,
                      ASCIIToUTF16("COULD_NOT_GET_TEMP_DIRECTORY")));
    return false;
  }

  if (!temp_dir_.CreateUniqueTempDirUnderPath(temp_dir)) {
    ReportFailure(COULD_NOT_CREATE_TEMP_DIRECTORY,
                  l10n_util::GetStringFUTF16(
                      IDS_EXTENSION_PACKAGE_INSTALL_ERROR,
                      ASCIIToUTF16("COULD_NOT_CREATE_TEMP_DIRECTORY")));
    return false;
  }

  return true;
}

void SandboxedUnpacker::StartWithCrx(const CRXFileInfo& crx_info) {
  // We assume that we are started on the thread that the client wants us
  // to do file IO on.
  DCHECK(unpacker_io_task_runner_->RunsTasksInCurrentSequence());

  crx_unpack_start_time_ = base::TimeTicks::Now();
  std::string expected_hash;
  if (!crx_info.expected_hash.empty() &&
      base::CommandLine::ForCurrentProcess()->HasSwitch(
          extensions::switches::kEnableCrxHashCheck)) {
    expected_hash = base::ToLowerASCII(crx_info.expected_hash);
  }

  PATH_LENGTH_HISTOGRAM("Extensions.SandboxUnpackInitialCrxPathLength",
                        crx_info.path);
  if (!CreateTempDirectory())
    return;  // ReportFailure() already called.

  // Initialize the path that will eventually contain the unpacked extension.
  extension_root_ = temp_dir_.GetPath().AppendASCII(kTempExtensionName);
  PATH_LENGTH_HISTOGRAM("Extensions.SandboxUnpackUnpackedCrxPathLength",
                        extension_root_);

  // Extract the public key and validate the package.
  if (!ValidateSignature(crx_info.path, expected_hash))
    return;  // ValidateSignature() already reported the error.

  // Copy the crx file into our working directory.
  base::FilePath temp_crx_path =
      temp_dir_.GetPath().Append(crx_info.path.BaseName());
  PATH_LENGTH_HISTOGRAM("Extensions.SandboxUnpackTempCrxPathLength",
                        temp_crx_path);

  if (!base::CopyFile(crx_info.path, temp_crx_path)) {
    // Failed to copy extension file to temporary directory.
    ReportFailure(
        FAILED_TO_COPY_EXTENSION_FILE_TO_TEMP_DIRECTORY,
        l10n_util::GetStringFUTF16(
            IDS_EXTENSION_PACKAGE_INSTALL_ERROR,
            ASCIIToUTF16("FAILED_TO_COPY_EXTENSION_FILE_TO_TEMP_DIRECTORY")));
    return;
  }

  // The utility process will have access to the directory passed to
  // SandboxedUnpacker.  That directory should not contain a symlink or NTFS
  // reparse point.  When the path is used, following the link/reparse point
  // will cause file system access outside the sandbox path, and the sandbox
  // will deny the operation.
  base::FilePath link_free_crx_path;
  if (!base::NormalizeFilePath(temp_crx_path, &link_free_crx_path)) {
    LOG(ERROR) << "Could not get the normalized path of "
               << temp_crx_path.value();
    ReportFailure(COULD_NOT_GET_SANDBOX_FRIENDLY_PATH,
                  l10n_util::GetStringUTF16(IDS_EXTENSION_UNPACK_FAILED));
    return;
  }

  PATH_LENGTH_HISTOGRAM("Extensions.SandboxUnpackLinkFreeCrxPathLength",
                        link_free_crx_path);

  BrowserThread::PostTask(
      BrowserThread::IO, FROM_HERE,
      base::BindOnce(&SandboxedUnpacker::Unzip, this, link_free_crx_path));
}

void SandboxedUnpacker::StartWithDirectory(const std::string& extension_id,
                                           const std::string& public_key,
                                           const base::FilePath& directory) {
  extension_id_ = extension_id;
  public_key_ = public_key;
  if (!CreateTempDirectory())
    return;  // ReportFailure() already called.

  extension_root_ = temp_dir_.GetPath().AppendASCII(kTempExtensionName);

  if (!base::Move(directory, extension_root_)) {
    LOG(ERROR) << "Could not move " << directory.value() << " to "
               << extension_root_.value();
    ReportFailure(
        DIRECTORY_MOVE_FAILED,
        l10n_util::GetStringFUTF16(IDS_EXTENSION_PACKAGE_INSTALL_ERROR,
                                   ASCIIToUTF16("DIRECTORY_MOVE_FAILED")));
    return;
  }

  BrowserThread::PostTask(
      BrowserThread::IO, FROM_HERE,
      base::BindOnce(&SandboxedUnpacker::Unpack, this, extension_root_));
}

SandboxedUnpacker::~SandboxedUnpacker() {
  // To avoid blocking shutdown, don't delete temporary directory here if it
  // hasn't been cleaned up or passed on to another owner yet.
  // This is OK because ExtensionGarbageCollector will take care of the leaked
  // |temp_dir_| eventually.
  temp_dir_.Take();

  // Make sure that members get deleted on the thread they were created.
  if (image_sanitizer_) {
    unpacker_io_task_runner_->DeleteSoon(FROM_HERE,
                                         std::move(image_sanitizer_));
  }
  if (json_file_sanitizer_) {
    unpacker_io_task_runner_->DeleteSoon(FROM_HERE,
                                         std::move(json_file_sanitizer_));
  }
  if (connector_)
    unpacker_io_task_runner_->DeleteSoon(FROM_HERE, std::move(connector_));
}

void SandboxedUnpacker::StartUtilityProcessIfNeeded() {
  DCHECK_CURRENTLY_ON(content::BrowserThread::IO);

  if (utility_process_mojo_client_)
    return;

  utility_process_mojo_client_ = std::make_unique<
      content::UtilityProcessMojoClient<mojom::ExtensionUnpacker>>(
      l10n_util::GetStringUTF16(IDS_UTILITY_PROCESS_EXTENSION_UNPACKER_NAME));
  utility_process_mojo_client_->set_error_callback(
      base::Bind(&SandboxedUnpacker::UtilityProcessCrashed, this));

  utility_process_mojo_client_->set_exposed_directory(temp_dir_.GetPath());

  utility_process_mojo_client_->Start();
}

void SandboxedUnpacker::UtilityProcessCrashed() {
  DCHECK_CURRENTLY_ON(content::BrowserThread::IO);

  utility_process_mojo_client_.reset();

  unpacker_io_task_runner_->PostTask(
      FROM_HERE,
      base::BindOnce(
          &SandboxedUnpacker::ReportFailure, this,
          UTILITY_PROCESS_CRASHED_WHILE_TRYING_TO_INSTALL,
          l10n_util::GetStringFUTF16(
              IDS_EXTENSION_PACKAGE_INSTALL_ERROR,
              ASCIIToUTF16("UTILITY_PROCESS_CRASHED_WHILE_TRYING_TO_INSTALL")) +
              ASCIIToUTF16(". ") +
              l10n_util::GetStringUTF16(
                  IDS_EXTENSION_INSTALL_PROCESS_CRASHED)));
}

void SandboxedUnpacker::Unzip(const base::FilePath& crx_path) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);

  StartUtilityProcessIfNeeded();

  DCHECK(crx_path.DirName() == temp_dir_.GetPath());
  base::FilePath unzipped_dir =
      crx_path.DirName().AppendASCII(kTempExtensionName);

  utility_process_mojo_client_->service()->Unzip(
      crx_path, unzipped_dir,
      base::BindOnce(&SandboxedUnpacker::UnzipDone, this, unzipped_dir));
}

void SandboxedUnpacker::UnzipDone(const base::FilePath& directory,
                                  bool success) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);

  utility_process_mojo_client_.reset();

  if (!success) {
    utility_process_mojo_client_.reset();
    unpacker_io_task_runner_->PostTask(
        FROM_HERE,
        base::BindOnce(
            &SandboxedUnpacker::ReportFailure, this, UNZIP_FAILED,
            l10n_util::GetStringUTF16(IDS_EXTENSION_PACKAGE_UNZIP_ERROR)));
    return;
  }

  BrowserThread::PostTask(
      BrowserThread::IO, FROM_HERE,
      base::BindOnce(&SandboxedUnpacker::Unpack, this, directory));
}

void SandboxedUnpacker::Unpack(const base::FilePath& directory) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);

  DCHECK(directory.DirName() == temp_dir_.GetPath());

  base::FilePath manifest_path = extension_root_.Append(kManifestFilename);
  unpacker_io_task_runner_->PostTask(
      FROM_HERE,
      base::BindOnce(
          &SandboxedUnpacker::ParseJsonFile, this, manifest_path,
          base::BindOnce(&SandboxedUnpacker::ReadManifestDone, this)));
}

void SandboxedUnpacker::ReadManifestDone(
    std::unique_ptr<base::Value> manifest,
    const base::Optional<std::string>& error) {
  DCHECK(unpacker_io_task_runner_->RunsTasksInCurrentSequence());
  if (error) {
    ReportUnpackingError(*error);
    return;
  }
  if (!manifest || !manifest->is_dict()) {
    ReportUnpackingError(manifest_errors::kInvalidManifest);
    return;
  }

  std::unique_ptr<base::DictionaryValue> manifest_dict =
      base::DictionaryValue::From(std::move(manifest));

  std::string error_msg;
  scoped_refptr<Extension> extension(
      Extension::Create(extension_root_, location_, *manifest_dict,
                        creation_flags_, extension_id_, &error_msg));
  if (!extension) {
    ReportUnpackingError(error_msg);
    return;
  }

  std::vector<InstallWarning> warnings;
  if (!file_util::ValidateExtension(extension.get(), &error_msg, &warnings)) {
    ReportUnpackingError(error_msg);
    return;
  }
  extension->AddInstallWarnings(warnings);

  UnpackExtensionSucceeded(std::move(manifest_dict));
}

void SandboxedUnpacker::UnpackExtensionSucceeded(
    std::unique_ptr<base::DictionaryValue> manifest) {
  DCHECK(unpacker_io_task_runner_->RunsTasksInCurrentSequence());

  std::unique_ptr<base::DictionaryValue> final_manifest(
      RewriteManifestFile(*manifest));
  if (!final_manifest)
    return;

  // Create an extension object that refers to the temporary location the
  // extension was unpacked to. We use this until the extension is finally
  // installed. For example, the install UI shows images from inside the
  // extension.

  // Localize manifest now, so confirm UI gets correct extension name.

  // TODO(rdevlin.cronin): Continue removing std::string errors and replacing
  // with base::string16
  std::string utf8_error;
  if (!extension_l10n_util::LocalizeExtension(
          extension_root_, final_manifest.get(), &utf8_error)) {
    ReportFailure(
        COULD_NOT_LOCALIZE_EXTENSION,
        l10n_util::GetStringFUTF16(IDS_EXTENSION_PACKAGE_ERROR_MESSAGE,
                                   base::UTF8ToUTF16(utf8_error)));
    return;
  }

  extension_ =
      Extension::Create(extension_root_, location_, *final_manifest,
                        Extension::REQUIRE_KEY | creation_flags_, &utf8_error);

  if (!extension_.get()) {
    ReportFailure(INVALID_MANIFEST,
                  ASCIIToUTF16("Manifest is invalid: " + utf8_error));
    return;
  }

  // The install icon path may be empty, which is OK, but if it is not it should
  // be normalized successfully.
  const std::string& original_install_icon_path =
      IconsInfo::GetIcons(extension_.get())
          .Get(extension_misc::EXTENSION_ICON_LARGE,
               ExtensionIconSet::MATCH_BIGGER);
  if (!original_install_icon_path.empty() &&
      !NormalizeExtensionResourcePath(
          base::FilePath::FromUTF8Unsafe(original_install_icon_path),
          &install_icon_path_)) {
    // Invalid path for browser image.
    ReportFailure(INVALID_PATH_FOR_BROWSER_IMAGE,
                  l10n_util::GetStringFUTF16(
                      IDS_EXTENSION_PACKAGE_INSTALL_ERROR,
                      ASCIIToUTF16("INVALID_PATH_FOR_BROWSER_IMAGE")));
    return;
  }

  DCHECK(!image_sanitizer_);
  std::set<base::FilePath> image_paths =
      ExtensionsClient::Get()->GetBrowserImagePaths(extension_.get());
  image_sanitizer_ = ImageSanitizer::CreateAndStart(
      connector_.get(), data_decoder_identity_, extension_root_, image_paths,
      base::BindRepeating(&SandboxedUnpacker::ImageSanitizerDecodedImage, this),
      base::BindOnce(&SandboxedUnpacker::ImageSanitizationDone, this,
                     std::move(manifest)));
}

void SandboxedUnpacker::ImageSanitizerDecodedImage(const base::FilePath& path,
                                                   SkBitmap image) {
  if (path == install_icon_path_)
    install_icon_ = image;
}

void SandboxedUnpacker::ImageSanitizationDone(
    std::unique_ptr<base::DictionaryValue> manifest,
    ImageSanitizer::Status status,
    const base::FilePath& file_path_for_error) {
  if (status == ImageSanitizer::Status::kSuccess) {
    // Next step is to sanitize the message catalogs.
    ReadMessageCatalogs(std::move(manifest));
    return;
  }

  FailureReason failure_reason = UNPACKER_CLIENT_FAILED;
  base::string16 error;
  switch (status) {
    case ImageSanitizer::Status::kImagePathError:
      failure_reason = INVALID_PATH_FOR_BROWSER_IMAGE;
      error = l10n_util::GetStringFUTF16(
          IDS_EXTENSION_PACKAGE_INSTALL_ERROR,
          ASCIIToUTF16("INVALID_PATH_FOR_BROWSER_IMAGE"));
      break;
    case ImageSanitizer::Status::kFileReadError:
    case ImageSanitizer::Status::kDecodingError:
      error = l10n_util::GetStringFUTF16(
          IDS_EXTENSION_PACKAGE_IMAGE_ERROR,
          base::i18n::GetDisplayStringInLTRDirectionality(
              file_path_for_error.BaseName().LossyDisplayName()));
      break;
    case ImageSanitizer::Status::kFileDeleteError:
      failure_reason = ERROR_REMOVING_OLD_IMAGE_FILE;
      error = l10n_util::GetStringFUTF16(
          IDS_EXTENSION_PACKAGE_INSTALL_ERROR,
          ASCIIToUTF16("ERROR_REMOVING_OLD_IMAGE_FILE"));
      break;
    case ImageSanitizer::Status::kEncodingError:
      failure_reason = ERROR_RE_ENCODING_THEME_IMAGE;
      error = l10n_util::GetStringFUTF16(
          IDS_EXTENSION_PACKAGE_INSTALL_ERROR,
          ASCIIToUTF16("ERROR_RE_ENCODING_THEME_IMAGE"));
      break;
    case ImageSanitizer::Status::kFileWriteError:
      failure_reason = ERROR_SAVING_THEME_IMAGE;
      error =
          l10n_util::GetStringFUTF16(IDS_EXTENSION_PACKAGE_INSTALL_ERROR,
                                     ASCIIToUTF16("ERROR_SAVING_THEME_IMAGE"));
      break;
    default:
      NOTREACHED();
      break;
  }

  ReportFailure(failure_reason, error);
}

void SandboxedUnpacker::ReadMessageCatalogs(
    std::unique_ptr<base::DictionaryValue> manifest) {
  DCHECK(unpacker_io_task_runner_->RunsTasksInCurrentSequence());
  if (LocaleInfo::GetDefaultLocale(extension_.get()).empty()) {
    MessageCatalogsSanitized(std::move(manifest),
                             JsonFileSanitizer::Status::kSuccess,
                             std::string());
    return;
  }

  // Get the paths to the message catalogs we should sanitize on the file task
  // runner.
  base::FilePath locales_path = extension_root_.Append(kLocaleFolder);

  base::PostTaskAndReplyWithResult(
      extensions::GetExtensionFileTaskRunner().get(), FROM_HERE,
      base::BindOnce(&GetMessageCatalogPathsToBeSanitized, locales_path),
      base::BindOnce(&SandboxedUnpacker::SanitizeMessageCatalogs, this,
                     std::move(manifest)));
}

void SandboxedUnpacker::SanitizeMessageCatalogs(
    std::unique_ptr<base::DictionaryValue> manifest,
    const std::set<base::FilePath>& message_catalog_paths) {
  DCHECK(unpacker_io_task_runner_->RunsTasksInCurrentSequence());
  json_file_sanitizer_ = JsonFileSanitizer::CreateAndStart(
      connector_.get(), data_decoder_identity_, message_catalog_paths,
      base::BindOnce(&SandboxedUnpacker::MessageCatalogsSanitized, this,
                     std::move(manifest)));
}

void SandboxedUnpacker::MessageCatalogsSanitized(
    std::unique_ptr<base::DictionaryValue> manifest,
    JsonFileSanitizer::Status status,
    const std::string& error_msg) {
  DCHECK(unpacker_io_task_runner_->RunsTasksInCurrentSequence());
  if (status == JsonFileSanitizer::Status::kSuccess) {
    ReadJSONRulesetIfNeeded(std::move(manifest));
    return;
  }

  FailureReason failure_reason = UNPACKER_CLIENT_FAILED;
  base::string16 error;
  switch (status) {
    case JsonFileSanitizer::Status::kFileReadError:
    case JsonFileSanitizer::Status::kDecodingError:
      failure_reason = INVALID_CATALOG_DATA;
      error = l10n_util::GetStringFUTF16(IDS_EXTENSION_PACKAGE_INSTALL_ERROR,
                                         ASCIIToUTF16("INVALID_CATALOG_DATA"));
      break;
    case JsonFileSanitizer::Status::kSerializingError:
      failure_reason = ERROR_SERIALIZING_CATALOG;
      error =
          l10n_util::GetStringFUTF16(IDS_EXTENSION_PACKAGE_INSTALL_ERROR,
                                     ASCIIToUTF16("ERROR_SERIALIZING_CATALOG"));
      break;
    case JsonFileSanitizer::Status::kFileDeleteError:
    case JsonFileSanitizer::Status::kFileWriteError:
      failure_reason = ERROR_SAVING_CATALOG;
      error = l10n_util::GetStringFUTF16(IDS_EXTENSION_PACKAGE_INSTALL_ERROR,
                                         ASCIIToUTF16("ERROR_SAVING_CATALOG"));
      break;
    default:
      NOTREACHED();
      break;
  }

  ReportFailure(failure_reason, error);
}

void SandboxedUnpacker::ReadJSONRulesetIfNeeded(
    std::unique_ptr<base::DictionaryValue> manifest) {
  DCHECK(unpacker_io_task_runner_->RunsTasksInCurrentSequence());

  const ExtensionResource* resource =
      declarative_net_request::DNRManifestData::GetRulesetResource(
          extension_.get());
  if (!resource) {
    ReadJSONRulesetDone(std::move(manifest),
                        /*json_ruleset=*/nullptr, /*error=*/base::nullopt);
    return;
  }

  ParseJsonFile(resource->GetFilePath(),
                base::BindOnce(&SandboxedUnpacker::ReadJSONRulesetDone, this,
                               std::move(manifest)));
}

void SandboxedUnpacker::ReadJSONRulesetDone(
    std::unique_ptr<base::DictionaryValue> manifest,
    std::unique_ptr<base::Value> json_ruleset,
    const base::Optional<std::string>& error) {
  DCHECK(unpacker_io_task_runner_->RunsTasksInCurrentSequence());

  if (error) {
    ReportUnpackingError(*error);
    return;
  }

  if (json_ruleset && !json_ruleset->is_list()) {
    ReportUnpackingError(manifest_errors::kDeclarativeNetRequestListNotPassed);
    return;
  }

  // Index and persist ruleset for the Declarative Net Request API.
  base::Optional<int> dnr_ruleset_checksum;
  if (!IndexAndPersistRulesIfNeeded(
          base::ListValue::From(std::move(json_ruleset)),
          &dnr_ruleset_checksum)) {
    return;  // Failure was already reported.
  }

  ReportSuccess(std::move(manifest), dnr_ruleset_checksum);
}

bool SandboxedUnpacker::IndexAndPersistRulesIfNeeded(
    std::unique_ptr<base::ListValue> json_ruleset,
    base::Optional<int>* dnr_ruleset_checksum) {
  DCHECK(extension_);
  DCHECK(dnr_ruleset_checksum);

  // Delete extension provided indexed ruleset file/folder, since it's a
  // reserved file name. This helps ensure that we only use one generated by the
  // Extension system.
  base::DeleteFile(file_util::GetIndexedRulesetPath(extension_->path()),
                   true /*recursive*/);

  if (!json_ruleset)
    return true;

  std::string error;
  std::vector<InstallWarning> warnings;
  int ruleset_checksum;
  if (!declarative_net_request::IndexAndPersistRules(
          *json_ruleset, *extension_, &error, &warnings, &ruleset_checksum)) {
    ReportFailure(
        ERROR_INDEXING_DNR_RULESET,
        l10n_util::GetStringFUTF16(IDS_EXTENSION_PACKAGE_ERROR_MESSAGE,
                                   base::UTF8ToUTF16(error)));
    return false;
  }

  *dnr_ruleset_checksum = ruleset_checksum;
  extension_->AddInstallWarnings(warnings);
  return true;
}

data_decoder::mojom::JsonParser* SandboxedUnpacker::GetJsonParserPtr() {
  DCHECK(unpacker_io_task_runner_->RunsTasksInCurrentSequence());
  if (!json_parser_ptr_) {
    connector_->BindInterface(data_decoder_identity_, &json_parser_ptr_);
    json_parser_ptr_.set_connection_error_handler(base::BindOnce(
        &SandboxedUnpacker::ReportFailure, this,
        UTILITY_PROCESS_CRASHED_WHILE_TRYING_TO_INSTALL,
        l10n_util::GetStringFUTF16(
            IDS_EXTENSION_PACKAGE_INSTALL_ERROR,
            ASCIIToUTF16("UTILITY_PROCESS_CRASHED_WHILE_TRYING_TO_INSTALL")) +
            ASCIIToUTF16(". ") +
            l10n_util::GetStringUTF16(IDS_EXTENSION_INSTALL_PROCESS_CRASHED)));
  }
  return json_parser_ptr_.get();
}

void SandboxedUnpacker::ReportUnpackingError(base::StringPiece error) {
  unpacker_io_task_runner_->PostTask(
      FROM_HERE, base::BindOnce(&SandboxedUnpacker::UnpackExtensionFailed, this,
                                base::UTF8ToUTF16(error)));
}

void SandboxedUnpacker::UnpackExtensionFailed(const base::string16& error) {
  DCHECK(unpacker_io_task_runner_->RunsTasksInCurrentSequence());

  ReportFailure(
      UNPACKER_CLIENT_FAILED,
      l10n_util::GetStringFUTF16(IDS_EXTENSION_PACKAGE_ERROR_MESSAGE, error));
}

base::string16 SandboxedUnpacker::FailureReasonToString16(
    FailureReason reason) {
  switch (reason) {
    case COULD_NOT_GET_TEMP_DIRECTORY:
      return ASCIIToUTF16("COULD_NOT_GET_TEMP_DIRECTORY");
    case COULD_NOT_CREATE_TEMP_DIRECTORY:
      return ASCIIToUTF16("COULD_NOT_CREATE_TEMP_DIRECTORY");
    case FAILED_TO_COPY_EXTENSION_FILE_TO_TEMP_DIRECTORY:
      return ASCIIToUTF16("FAILED_TO_COPY_EXTENSION_FILE_TO_TEMP_DIRECTORY");
    case COULD_NOT_GET_SANDBOX_FRIENDLY_PATH:
      return ASCIIToUTF16("COULD_NOT_GET_SANDBOX_FRIENDLY_PATH");
    case COULD_NOT_LOCALIZE_EXTENSION:
      return ASCIIToUTF16("COULD_NOT_LOCALIZE_EXTENSION");
    case INVALID_MANIFEST:
      return ASCIIToUTF16("INVALID_MANIFEST");
    case UNPACKER_CLIENT_FAILED:
      return ASCIIToUTF16("UNPACKER_CLIENT_FAILED");
    case UTILITY_PROCESS_CRASHED_WHILE_TRYING_TO_INSTALL:
      return ASCIIToUTF16("UTILITY_PROCESS_CRASHED_WHILE_TRYING_TO_INSTALL");

    case CRX_FILE_NOT_READABLE:
      return ASCIIToUTF16("CRX_FILE_NOT_READABLE");
    case CRX_HEADER_INVALID:
      return ASCIIToUTF16("CRX_HEADER_INVALID");
    case CRX_MAGIC_NUMBER_INVALID:
      return ASCIIToUTF16("CRX_MAGIC_NUMBER_INVALID");
    case CRX_VERSION_NUMBER_INVALID:
      return ASCIIToUTF16("CRX_VERSION_NUMBER_INVALID");
    case CRX_EXCESSIVELY_LARGE_KEY_OR_SIGNATURE:
      return ASCIIToUTF16("CRX_EXCESSIVELY_LARGE_KEY_OR_SIGNATURE");
    case CRX_ZERO_KEY_LENGTH:
      return ASCIIToUTF16("CRX_ZERO_KEY_LENGTH");
    case CRX_ZERO_SIGNATURE_LENGTH:
      return ASCIIToUTF16("CRX_ZERO_SIGNATURE_LENGTH");
    case CRX_PUBLIC_KEY_INVALID:
      return ASCIIToUTF16("CRX_PUBLIC_KEY_INVALID");
    case CRX_SIGNATURE_INVALID:
      return ASCIIToUTF16("CRX_SIGNATURE_INVALID");
    case CRX_SIGNATURE_VERIFICATION_INITIALIZATION_FAILED:
      return ASCIIToUTF16("CRX_SIGNATURE_VERIFICATION_INITIALIZATION_FAILED");
    case CRX_SIGNATURE_VERIFICATION_FAILED:
      return ASCIIToUTF16("CRX_SIGNATURE_VERIFICATION_FAILED");
    case CRX_FILE_IS_DELTA_UPDATE:
      return ASCIIToUTF16("CRX_FILE_IS_DELTA_UPDATE");
    case CRX_EXPECTED_HASH_INVALID:
      return ASCIIToUTF16("CRX_EXPECTED_HASH_INVALID");

    case ERROR_SERIALIZING_MANIFEST_JSON:
      return ASCIIToUTF16("ERROR_SERIALIZING_MANIFEST_JSON");
    case ERROR_SAVING_MANIFEST_JSON:
      return ASCIIToUTF16("ERROR_SAVING_MANIFEST_JSON");

    case INVALID_PATH_FOR_BROWSER_IMAGE:
      return ASCIIToUTF16("INVALID_PATH_FOR_BROWSER_IMAGE");
    case ERROR_REMOVING_OLD_IMAGE_FILE:
      return ASCIIToUTF16("ERROR_REMOVING_OLD_IMAGE_FILE");
    case INVALID_PATH_FOR_BITMAP_IMAGE:
      return ASCIIToUTF16("INVALID_PATH_FOR_BITMAP_IMAGE");
    case ERROR_RE_ENCODING_THEME_IMAGE:
      return ASCIIToUTF16("ERROR_RE_ENCODING_THEME_IMAGE");
    case ERROR_SAVING_THEME_IMAGE:
      return ASCIIToUTF16("ERROR_SAVING_THEME_IMAGE");

    case INVALID_CATALOG_DATA:
      return ASCIIToUTF16("INVALID_CATALOG_DATA");
    case ERROR_SERIALIZING_CATALOG:
      return ASCIIToUTF16("ERROR_SERIALIZING_CATALOG");
    case ERROR_SAVING_CATALOG:
      return ASCIIToUTF16("ERROR_SAVING_CATALOG");

    case CRX_HASH_VERIFICATION_FAILED:
      return ASCIIToUTF16("CRX_HASH_VERIFICATION_FAILED");

    case UNZIP_FAILED:
      return ASCIIToUTF16("UNZIP_FAILED");
    case DIRECTORY_MOVE_FAILED:
      return ASCIIToUTF16("DIRECTORY_MOVE_FAILED");

    case ERROR_PARSING_DNR_RULESET:
      return ASCIIToUTF16("ERROR_PARSING_DNR_RULESET");
    case ERROR_INDEXING_DNR_RULESET:
      return ASCIIToUTF16("ERROR_INDEXING_DNR_RULESET");

    case DEPRECATED_ABORTED_DUE_TO_SHUTDOWN:
    case NUM_FAILURE_REASONS:
    default:
      NOTREACHED();
      return base::string16();
  }
}

void SandboxedUnpacker::FailWithPackageError(FailureReason reason) {
  ReportFailure(reason,
                l10n_util::GetStringFUTF16(IDS_EXTENSION_PACKAGE_ERROR_CODE,
                                           FailureReasonToString16(reason)));
}

bool SandboxedUnpacker::ValidateSignature(const base::FilePath& crx_path,
                                          const std::string& expected_hash) {
  std::vector<uint8_t> hash;
  if (!expected_hash.empty()) {
    if (!base::HexStringToBytes(expected_hash, &hash)) {
      FailWithPackageError(CRX_EXPECTED_HASH_INVALID);
      return false;
    }
  }
  const crx_file::VerifierResult result = crx_file::Verify(
      crx_path, crx_file::VerifierFormat::CRX2_OR_CRX3,
      std::vector<std::vector<uint8_t>>(), hash, &public_key_, &extension_id_);

  switch (result) {
    case crx_file::VerifierResult::OK_FULL: {
      if (!expected_hash.empty())
        UMA_HISTOGRAM_BOOLEAN("Extensions.SandboxUnpackHashCheck", true);
      return true;
    }
    case crx_file::VerifierResult::OK_DELTA:
      FailWithPackageError(CRX_FILE_IS_DELTA_UPDATE);
      break;
    case crx_file::VerifierResult::ERROR_FILE_NOT_READABLE:
      FailWithPackageError(CRX_FILE_NOT_READABLE);
      break;
    case crx_file::VerifierResult::ERROR_HEADER_INVALID:
      FailWithPackageError(CRX_HEADER_INVALID);
      break;
    case crx_file::VerifierResult::ERROR_SIGNATURE_INITIALIZATION_FAILED:
      FailWithPackageError(CRX_SIGNATURE_VERIFICATION_INITIALIZATION_FAILED);
      break;
    case crx_file::VerifierResult::ERROR_SIGNATURE_VERIFICATION_FAILED:
      FailWithPackageError(CRX_SIGNATURE_VERIFICATION_FAILED);
      break;
    case crx_file::VerifierResult::ERROR_EXPECTED_HASH_INVALID:
      FailWithPackageError(CRX_EXPECTED_HASH_INVALID);
      break;
    case crx_file::VerifierResult::ERROR_REQUIRED_PROOF_MISSING:
      // We should never get this result, as we do not call
      // verifier.RequireKeyProof.
      NOTREACHED();
      break;
    case crx_file::VerifierResult::ERROR_FILE_HASH_FAILED:
      // We should never get this result unless we had specifically asked for
      // verification of the crx file's hash.
      CHECK(!expected_hash.empty());
      UMA_HISTOGRAM_BOOLEAN("Extensions.SandboxUnpackHashCheck", false);
      FailWithPackageError(CRX_HASH_VERIFICATION_FAILED);
      break;
  }

  return false;
}

void SandboxedUnpacker::ReportFailure(FailureReason reason,
                                      const base::string16& error) {
  DCHECK(unpacker_io_task_runner_->RunsTasksInCurrentSequence());

  UMA_HISTOGRAM_ENUMERATION("Extensions.SandboxUnpackFailureReason", reason,
                            NUM_FAILURE_REASONS);
  if (!crx_unpack_start_time_.is_null())
    UMA_HISTOGRAM_TIMES("Extensions.SandboxUnpackFailureTime",
                        base::TimeTicks::Now() - crx_unpack_start_time_);
  Cleanup();

  CrxInstallError error_info(reason == CRX_HASH_VERIFICATION_FAILED
                                 ? CrxInstallError::ERROR_HASH_MISMATCH
                                 : CrxInstallError::ERROR_OTHER,
                             error);

  client_->OnUnpackFailure(error_info);
}

void SandboxedUnpacker::ReportSuccess(
    std::unique_ptr<base::DictionaryValue> original_manifest,
    const base::Optional<int>& dnr_ruleset_checksum) {
  DCHECK(unpacker_io_task_runner_->RunsTasksInCurrentSequence());

  UMA_HISTOGRAM_COUNTS("Extensions.SandboxUnpackSuccess", 1);

  if (!crx_unpack_start_time_.is_null())
    RecordSuccessfulUnpackTimeHistograms(
        crx_path_for_histograms_,
        base::TimeTicks::Now() - crx_unpack_start_time_);
  DCHECK(!temp_dir_.GetPath().empty());

  // Client takes ownership of temporary directory, manifest, and extension.
  client_->OnUnpackSuccess(temp_dir_.Take(), extension_root_,
                           std::move(original_manifest), extension_.get(),
                           install_icon_, dnr_ruleset_checksum);
  extension_ = NULL;

  Cleanup();
}

base::DictionaryValue* SandboxedUnpacker::RewriteManifestFile(
    const base::DictionaryValue& manifest) {
  // Add the public key extracted earlier to the parsed manifest and overwrite
  // the original manifest. We do this to ensure the manifest doesn't contain an
  // exploitable bug that could be used to compromise the browser.
  DCHECK(!public_key_.empty());
  std::unique_ptr<base::DictionaryValue> final_manifest(manifest.DeepCopy());
  final_manifest->SetString(manifest_keys::kPublicKey, public_key_);

  std::string manifest_json;
  JSONStringValueSerializer serializer(&manifest_json);
  serializer.set_pretty_print(true);
  if (!serializer.Serialize(*final_manifest)) {
    // Error serializing manifest.json.
    ReportFailure(ERROR_SERIALIZING_MANIFEST_JSON,
                  l10n_util::GetStringFUTF16(
                      IDS_EXTENSION_PACKAGE_INSTALL_ERROR,
                      ASCIIToUTF16("ERROR_SERIALIZING_MANIFEST_JSON")));
    return NULL;
  }

  base::FilePath manifest_path = extension_root_.Append(kManifestFilename);
  int size = base::checked_cast<int>(manifest_json.size());
  if (base::WriteFile(manifest_path, manifest_json.data(), size) != size) {
    // Error saving manifest.json.
    ReportFailure(
        ERROR_SAVING_MANIFEST_JSON,
        l10n_util::GetStringFUTF16(IDS_EXTENSION_PACKAGE_INSTALL_ERROR,
                                   ASCIIToUTF16("ERROR_SAVING_MANIFEST_JSON")));
    return NULL;
  }

  return final_manifest.release();
}

void SandboxedUnpacker::Cleanup() {
  DCHECK(unpacker_io_task_runner_->RunsTasksInCurrentSequence());
  if (temp_dir_.IsValid() && !temp_dir_.Delete()) {
    LOG(WARNING) << "Can not delete temp directory at "
                 << temp_dir_.GetPath().value();
  }
  connector_.reset();
  image_sanitizer_.reset();
  json_file_sanitizer_.reset();
  json_parser_ptr_.reset();
}

void SandboxedUnpacker::ParseJsonFile(
    const base::FilePath& path,
    data_decoder::mojom::JsonParser::ParseCallback callback) {
  DCHECK(unpacker_io_task_runner_->RunsTasksInCurrentSequence());
  std::string contents;
  if (!base::ReadFileToString(path, &contents)) {
    std::move(callback).Run(
        /*value=*/nullptr,
        /*error=*/base::Optional<std::string>("File doesn't exist."));
    return;
  }

  GetJsonParserPtr()->Parse(contents, std::move(callback));
}

}  // namespace extensions
