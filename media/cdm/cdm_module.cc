// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/cdm/cdm_module.h"

#include "base/base_paths.h"
#include "base/memory/ptr_util.h"
#include "base/path_service.h"
#include "build/build_config.h"
#include "media/base/key_system_names.h"
#include "media/base/key_systems.h"
#include "media/cdm/cdm_paths.h"

#if defined(OS_MACOSX)
#include "base/mac/bundle_locations.h"
#endif

#include "widevine_cdm_version.h"  // In SHARED_INTERMEDIATE_DIR.

// INITIALIZE_CDM_MODULE is a macro in api/content_decryption_module.h.
// However, we need to pass it as a string to GetFunctionPointer(). The follow
// macro helps expanding it into a string.
#define STRINGIFY(X) #X
#define MAKE_STRING(X) STRINGIFY(X)

namespace media {

namespace {

// TODO(xhwang): We should have the CDM path forwarded from the browser process.
// See http://crbug.com/510604
base::FilePath GetCdmPath(const std::string& key_system) {
  base::FilePath cdm_path;

#if defined(WIDEVINE_CDM_AVAILABLE)
  if (key_system == kWidevineKeySystem) {
    // Build the library path for the Widevine CDM.
    base::FilePath cdm_base_path;

#if defined(OS_MACOSX)
    base::FilePath framework_bundle_path = base::mac::FrameworkBundlePath();
    cdm_base_path = framework_bundle_path.Append("Libraries");
#else
    base::PathService::Get(base::DIR_MODULE, &cdm_base_path);
#endif

    cdm_base_path = cdm_base_path.Append(
        GetPlatformSpecificDirectory(kWidevineCdmBaseDirectory));
    cdm_path = cdm_base_path.AppendASCII(
        base::GetNativeLibraryName(kWidevineCdmLibraryName));
    DVLOG(1) << "CDM path: " << cdm_path.value();
  }
#endif  // defined(WIDEVINE_CDM_AVAILABLE)

// The hard-coded path for ClearKeyCdm does not work on Mac due to bundling.
// See http://crbug.com/736106
#if !defined(OS_MACOSX)
  if (cdm_path.empty() && IsExternalClearKey(key_system)) {
    base::FilePath cdm_base_path;
    base::PathService::Get(base::DIR_MODULE, &cdm_base_path);
    cdm_base_path = cdm_base_path.Append(
        GetPlatformSpecificDirectory(kClearKeyCdmBaseDirectory));
    cdm_path = cdm_base_path.AppendASCII(
        base::GetNativeLibraryName(kClearKeyCdmLibraryName));
  }
#endif  // !defined(OS_MACOSX)

  return cdm_path;
}

}  // namespace

// static
CdmModule* CdmModule::GetInstance() {
  // Use std::unique_ptr() to make sure the Cdm module is destructed properly,
  // e.g. make sure DeinitializeCdmModule() is called.
  static auto cdm_module = std::unique_ptr<CdmModule>(new CdmModule());
  return cdm_module.get();
}

CdmModule::CdmModule() {}

CdmModule::~CdmModule() {
  if (deinitialize_cdm_module_func_)
    deinitialize_cdm_module_func_();
}

CdmModule::CreateCdmFunc CdmModule::GetCreateCdmFunc(
    const std::string& key_system) {
  if (!is_initialized_) {
    Initialize(key_system);
    DCHECK(is_initialized_);
  }

  return create_cdm_func_;
}

void CdmModule::Initialize(const std::string& key_system) {
  DVLOG(2) << __func__ << ": key_system = " << key_system;
  DCHECK(!is_initialized_);

  is_initialized_ = true;

  base::FilePath cdm_path = GetCdmPath(key_system);
  if (cdm_path.empty()) {
    DVLOG(1) << "CDM for " + key_system + " could not be found.";
    return;
  }

  // Load the CDM.
  // TODO(xhwang): Report CDM load error to UMA.
  base::NativeLibraryLoadError error;
  library_.Reset(base::LoadNativeLibrary(cdm_path, &error));
  if (!library_.is_valid()) {
    LOG(ERROR) << "CDM at " << cdm_path.value() << " could not be loaded.";
    LOG(ERROR) << "Error: " << error.ToString();
    return;
  }

  // Get function pointers.
  // TODO(xhwang): Define function names in macros to avoid typo errors.
  using InitializeCdmModuleFunc = void (*)();
  InitializeCdmModuleFunc initialize_cdm_module_func =
      reinterpret_cast<InitializeCdmModuleFunc>(
          library_.GetFunctionPointer(MAKE_STRING(INITIALIZE_CDM_MODULE)));
  deinitialize_cdm_module_func_ = reinterpret_cast<DeinitializeCdmModuleFunc>(
      library_.GetFunctionPointer("DeinitializeCdmModule"));
  create_cdm_func_ = reinterpret_cast<CreateCdmFunc>(
      library_.GetFunctionPointer("CreateCdmInstance"));

  if (!initialize_cdm_module_func || !deinitialize_cdm_module_func_ ||
      !create_cdm_func_) {
    LOG(ERROR) << "Missing entry function in CDM for " + key_system;
    deinitialize_cdm_module_func_ = nullptr;
    create_cdm_func_ = nullptr;
    library_.Release();
    return;
  }

  initialize_cdm_module_func();
}

}  // namespace media
