// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/zygote/content_zygote_main_linux.h"

#include <utility>

#include "base/bind.h"
#include "base/command_line.h"
#include "base/compiler_specific.h"
#include "base/memory/protected_memory.h"
#include "base/memory/protected_memory_cfi.h"
#include "base/native_library.h"
#include "base/posix/eintr_wrapper.h"
#include "base/posix/unix_domain_socket.h"
#include "base/rand_util.h"
#include "base/strings/safe_sprintf.h"
#include "base/strings/string_number_conversions.h"
#include "base/sys_info.h"
#include "build/build_config.h"
#include "content/common/font_config_ipc_linux.h"
#include "content/public/common/common_sandbox_support_linux.h"
#include "content/public/common/content_descriptors.h"
#include "content/public/common/content_switches.h"
#include "content/public/common/zygote_fork_delegate_linux.h"
#include "content/zygote/zygote_linux.h"
#include "content/zygote/zygote_main_linux.h"
#include "media/media_features.h"
#include "ppapi/features/features.h"
#include "sandbox/linux/services/credentials.h"
#include "sandbox/linux/services/init_process_reaper.h"
#include "sandbox/linux/services/libc_interceptor.h"
#include "sandbox/linux/services/namespace_sandbox.h"
#include "sandbox/linux/services/thread_helpers.h"
#include "third_party/WebKit/public/web/linux/WebFontRendering.h"
#include "third_party/boringssl/src/include/openssl/crypto.h"
#include "third_party/boringssl/src/include/openssl/rand.h"
#include "third_party/icu/source/i18n/unicode/timezone.h"
#include "third_party/skia/include/ports/SkFontConfigInterface.h"
#include "third_party/skia/include/ports/SkFontMgr_android.h"

#if BUILDFLAG(ENABLE_PLUGINS)
#include "content/common/pepper_plugin_list.h"
#include "content/public/common/pepper_plugin_info.h"
#endif

#if BUILDFLAG(ENABLE_LIBRARY_CDMS)
#include "content/public/common/cdm_info.h"
#include "content/public/common/content_client.h"
#endif

#if BUILDFLAG(ENABLE_WEBRTC)
#include "third_party/webrtc_overrides/init_webrtc.h"  // nogncheck
#endif

namespace content {

#if BUILDFLAG(ENABLE_PLUGINS)
// Loads the (native) libraries but does not initialize them (i.e., does not
// call PPP_InitializeModule). This is needed by the zygote on Linux to get
// access to the plugins before entering the sandbox.
void PreloadPepperPlugins() {
  std::vector<PepperPluginInfo> plugins;
  ComputePepperPluginList(&plugins);
  for (const auto& plugin : plugins) {
    if (!plugin.is_internal) {
      base::NativeLibraryLoadError error;
      base::NativeLibrary library =
          base::LoadNativeLibrary(plugin.path, &error);
      VLOG_IF(1, !library) << "Unable to load plugin " << plugin.path.value()
                           << " " << error.ToString();

      ignore_result(library);  // Prevent release-mode warning.
    }
  }
}
#endif

#if BUILDFLAG(ENABLE_LIBRARY_CDMS)
// Loads registered library CDMs but does not initialize them. This is needed by
// the zygote on Linux to get access to the CDMs before entering the sandbox.
void PreloadLibraryCdms() {
  std::vector<CdmInfo> cdms;
  GetContentClient()->AddContentDecryptionModules(&cdms, nullptr);
  for (const auto& cdm : cdms) {
    base::NativeLibraryLoadError error;
    base::NativeLibrary library = base::LoadNativeLibrary(cdm.path, &error);
    VLOG_IF(1, !library) << "Unable to load CDM " << cdm.path.value()
                         << " (error: " << error.ToString() << ")";
    ignore_result(library);  // Prevent release-mode warning.
  }
}
#endif  // BUILDFLAG(ENABLE_LIBRARY_CDMS)

static void ContentZygotePreSandboxInit() {
#if defined(ARCH_CPU_ARM_FAMILY)
  // On ARM, BoringSSL requires access to /proc/cpuinfo to determine processor
  // features. Query this before entering the sandbox.
  CRYPTO_library_init();
#endif

  // Pass BoringSSL a copy of the /dev/urandom file descriptor so RAND_bytes
  // will work inside the sandbox.
  RAND_set_urandom_fd(base::GetUrandomFD());

#if BUILDFLAG(ENABLE_PLUGINS)
  // Ensure access to the Pepper plugins before the sandbox is turned on.
  PreloadPepperPlugins();
#endif
#if BUILDFLAG(ENABLE_LIBRARY_CDMS)
  // Ensure access to the library CDMs before the sandbox is turned on.
  PreloadLibraryCdms();
#endif
#if BUILDFLAG(ENABLE_WEBRTC)
  InitializeWebRtcModule();
#endif

  SkFontConfigInterface::SetGlobal(new FontConfigIPC(GetSandboxFD()))->unref();

  // Set the android SkFontMgr for blink. We need to ensure this is done
  // before the sandbox is initialized to allow the font manager to access
  // font configuration files on disk.
  if (base::CommandLine::ForCurrentProcess()->HasSwitch(
          switches::kAndroidFontsPath)) {
    std::string android_fonts_dir =
        base::CommandLine::ForCurrentProcess()->GetSwitchValueASCII(
            switches::kAndroidFontsPath);

    if (android_fonts_dir.size() > 0 && android_fonts_dir.back() != '/')
      android_fonts_dir += '/';

    SkFontMgr_Android_CustomFonts custom;
    custom.fSystemFontUse =
        SkFontMgr_Android_CustomFonts::SystemFontUse::kOnlyCustom;
    custom.fBasePath = android_fonts_dir.c_str();

    std::string font_config;
    std::string fallback_font_config;
    if (android_fonts_dir.find("kitkat") != std::string::npos) {
      font_config = android_fonts_dir + "system_fonts.xml";
      fallback_font_config = android_fonts_dir + "fallback_fonts.xml";
      custom.fFallbackFontsXml = fallback_font_config.c_str();
    } else {
      font_config = android_fonts_dir + "fonts.xml";
      custom.fFallbackFontsXml = nullptr;
    }
    custom.fFontsXml = font_config.c_str();
    custom.fIsolated = true;

    blink::WebFontRendering::SetSkiaFontManager(SkFontMgr_New_Android(&custom));
  }
}

bool ContentZygoteMain(
    std::vector<std::unique_ptr<ZygoteForkDelegate>> fork_delegates) {
  return ZygoteMain(
      base::BindOnce(ContentZygotePreSandboxInit),
      base::GlobalDescriptors::Descriptor(
          static_cast<uint32_t>(kSandboxIPCChannel), GetSandboxFD()),
      std::move(fork_delegates));
}

}  // namespace content
