// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media/cdm/cdm_module.h"

#include "base/memory/ptr_util.h"
#include "build/build_config.h"

#if BUILDFLAG(ENABLE_CDM_HOST_VERIFICATION)
#include "base/metrics/histogram_macros.h"
#endif  // BUILDFLAG(ENABLE_CDM_HOST_VERIFICATION)

// INITIALIZE_CDM_MODULE is a macro in api/content_decryption_module.h.
// However, we need to pass it as a string to GetFunctionPointer(). The follow
// macro helps expanding it into a string.
#define STRINGIFY(X) #X
#define MAKE_STRING(X) STRINGIFY(X)

namespace media {

static CdmModule* g_cdm_module = nullptr;

// static
CdmModule* CdmModule::GetInstance() {
  // The |cdm_module| will be leaked and we will not be able to call
  // |deinitialize_cdm_module_func_|. This is fine since it's never guaranteed
  // to be called, e.g. in the fast shutdown case.
  // TODO(xhwang): Find a better ownership model to make sure |cdm_module| is
  // destructed properly whenever possible (e.g. in non-fast-shutdown case).
  if (!g_cdm_module)
    g_cdm_module = new CdmModule();

  return g_cdm_module;
}

// static
void CdmModule::ResetInstanceForTesting() {
  if (!g_cdm_module)
    return;

  delete g_cdm_module;
  g_cdm_module = nullptr;
}

CdmModule::CdmModule() {}

CdmModule::~CdmModule() {
  if (deinitialize_cdm_module_func_)
    deinitialize_cdm_module_func_();
}

CdmModule::CreateCdmFunc CdmModule::GetCreateCdmFunc() {
  if (!initialize_called_) {
    DCHECK(false) << __func__ << " called before CdmModule is initialized.";
    return nullptr;
  }

  // If initialization failed, nullptr will be returned.
  return create_cdm_func_;
}

bool CdmModule::Initialize(const base::FilePath& cdm_path) {
  DVLOG(2) << __func__ << ": cdm_path = " << cdm_path.value();

  DCHECK(!initialize_called_);
  initialize_called_ = true;

  cdm_path_ = cdm_path;

  // Load the CDM.
  // TODO(xhwang): Report CDM load error to UMA.
  base::NativeLibraryLoadError error;
  library_.Reset(base::LoadNativeLibrary(cdm_path, &error));
  if (!library_.is_valid()) {
    LOG(ERROR) << "CDM at " << cdm_path.value() << " could not be loaded.";
    LOG(ERROR) << "Error: " << error.ToString();
    return false;
  }

  // Get function pointers.
  // TODO(xhwang): Define function names in macros to avoid typo errors.
  initialize_cdm_module_func_ = reinterpret_cast<InitializeCdmModuleFunc>(
      library_.GetFunctionPointer(MAKE_STRING(INITIALIZE_CDM_MODULE)));
  deinitialize_cdm_module_func_ = reinterpret_cast<DeinitializeCdmModuleFunc>(
      library_.GetFunctionPointer("DeinitializeCdmModule"));
  create_cdm_func_ = reinterpret_cast<CreateCdmFunc>(
      library_.GetFunctionPointer("CreateCdmInstance"));

  if (!initialize_cdm_module_func_ || !deinitialize_cdm_module_func_ ||
      !create_cdm_func_) {
    LOG(ERROR) << "Missing entry function in CDM at " << cdm_path.value();
    Release();
    return false;
  }

  return true;
}

void CdmModule::InitializeCdmModule() {
  if (initialize_cdm_module_func_)
    initialize_cdm_module_func_();
}

void CdmModule::Release() {
  initialize_cdm_module_func_ = nullptr;
  deinitialize_cdm_module_func_ = nullptr;
  create_cdm_func_ = nullptr;
  library_.Release();
}

base::FilePath CdmModule::GetCdmPath() const {
  DCHECK(initialize_called_);
  return cdm_path_;
}

#if BUILDFLAG(ENABLE_CDM_HOST_VERIFICATION)

bool CdmModule::PreSandboxHostVerification(
    const std::vector<CdmHostFilePath>& cdm_host_file_paths) {
  // Open CDM host files before the process is sandboxed.
  cdm_host_files_.Initialize(cdm_path_, cdm_host_file_paths);

#if defined(OS_WIN)
  // On Windows, initialize CDM host verification unsandboxed. On other
  // platforms, this is called sandboxed below.
  return InitVerification();
#else
  return true;
#endif  // defined(OS_WIN)
}

bool CdmModule::PostSandboxHostVerification() {
// Now we are sandboxed, initialize CDM host verification if not done yet.
#if defined(OS_WIN)
  return true;
#else
  return InitVerification();
#endif  // defined(OS_WIN)
}

bool CdmModule::InitVerification() {
  DCHECK(library_.is_valid());
  auto status = cdm_host_files_.InitVerification(library_.get());
  UMA_HISTOGRAM_ENUMERATION("Media.EME.CdmHostVerificationStatus", status,
                            CdmHostFiles::Status::kStatusMax + 1);

  // Ignore other failures for backward compatibility, e.g. when using an
  // old CDM which doesn't implement the verification API.
  if (status == CdmHostFiles::Status::kInitVerificationFailed) {
    LOG(WARNING) << "CDM host verification failed.";
    Release();
    return false;
  }

  return true;
}

#endif  // BUILDFLAG(ENABLE_CDM_HOST_VERIFICATION)

}  // namespace media
