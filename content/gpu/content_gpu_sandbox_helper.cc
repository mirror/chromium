// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/gpu/content_gpu_sandbox_helper.h"

#include <memory>

#include "gpu/config/gpu_info.h"
#include "gpu/ipc/service/gpu_init.h"
#include "third_party/angle/src/gpu_info_util/SystemInfo.h"

namespace {

inline bool IsArchitectureX86_64() {
#if defined(__x86_64__)
  return true;
#else
  return false;
#endif
}

inline bool IsArchitectureI386() {
#if defined(__i386__)
  return true;
#else
  return false;
#endif
}

#if defined(OS_CHROMEOS)
inline bool IsArchitectureArm() {
#if defined(__arm__)
  return true;
#else
  return false;
#endif
}

void AddArmMaliGpuWhitelist(std::vector<BrokerFilePermission>* permissions) {
  // Device file needed by the ARM GPU userspace.
  static const char kMali0Path[] = "/dev/mali0";

  // Image processor used on ARM platforms.
  static const char kDevImageProc0Path[] = "/dev/image-proc0";

  permissions->push_back(BrokerFilePermission::ReadWrite(kMali0Path));
  permissions->push_back(BrokerFilePermission::ReadWrite(kDevImageProc0Path));
}

void AddArmGpuWhitelist(std::vector<BrokerFilePermission>* permissions) {
  // On ARM we're enabling the sandbox before the X connection is made,
  // so we need to allow access to |.Xauthority|.
  static const char kXAuthorityPath[] = "/home/chronos/.Xauthority";
  static const char kLdSoCache[] = "/etc/ld.so.cache";

  // Files needed by the ARM GPU userspace.
  static const char kLibGlesPath[] = "/usr/lib/libGLESv2.so.2";
  static const char kLibEglPath[] = "/usr/lib/libEGL.so.1";

  permissions->push_back(BrokerFilePermission::ReadOnly(kXAuthorityPath));
  permissions->push_back(BrokerFilePermission::ReadOnly(kLdSoCache));
  permissions->push_back(BrokerFilePermission::ReadOnly(kLibGlesPath));
  permissions->push_back(BrokerFilePermission::ReadOnly(kLibEglPath));

  AddArmMaliGpuWhitelist(permissions);
}
void AddAmdGpuWhitelist(std::vector<BrokerFilePermission>* permissions) {
  static const char* kReadOnlyList[] = {"/etc/ld.so.cache",
                                        "/usr/lib64/libEGL.so.1",
                                        "/usr/lib64/libGLESv2.so.2"};
  int listSize = arraysize(kReadOnlyList);

  for (int i = 0; i < listSize; i++) {
    permissions->push_back(BrokerFilePermission::ReadOnly(kReadOnlyList[i]));
  }

  static const char* kReadWriteList[] = {
      "/dev/dri",
      "/dev/dri/card0",
      "/dev/dri/controlD64",
      "/dev/dri/renderD128",
      "/sys/class/drm/card0/device/config",
      "/sys/class/drm/controlD64/device/config",
      "/sys/class/drm/renderD128/device/config",
      "/usr/share/libdrm/amdgpu.ids"};

  listSize = arraysize(kReadWriteList);

  for (int i = 0; i < listSize; i++) {
    permissions->push_back(BrokerFilePermission::ReadWrite(kReadWriteList[i]));
  }

  static const char kCharDevices[] = "/sys/dev/char/";
  permissions->push_back(BrokerFilePermission::ReadOnlyRecursive(kCharDevices));
}
#endif  // defined(OS_CHROMEOS)

inline bool IsChromeOS() {
#if defined(OS_CHROMEOS)
  return true;
#else
  return false;
#endif
}

inline bool UseV4L2Codec() {
#if BUILDFLAG(USE_V4L2_CODEC)
  return true;
#else
  return false;
#endif
}

inline bool UseLibV4L2() {
#if BUILDFLAG(USE_LIBV4L2)
  return true;
#else
  return false;
#endif
}

bool IsAcceleratedVaapiVideoEncodeEnabled() {
  bool accelerated_encode_enabled = false;
#if defined(OS_CHROMEOS)
  const base::CommandLine& command_line =
      *base::CommandLine::ForCurrentProcess();
  accelerated_encode_enabled =
      !command_line.HasSwitch(switches::kDisableVaapiAcceleratedVideoEncode);
#endif
  return accelerated_encode_enabled;
}

bool IsAcceleratedVideoDecodeEnabled() {
  const base::CommandLine& command_line =
      *base::CommandLine::ForCurrentProcess();
  return !command_line.HasSwitch(switches::kDisableAcceleratedVideoDecode);
}

void AddV4L2GpuWhitelist(std::vector<BrokerFilePermission>* permissions) {
  if (IsAcceleratedVideoDecodeEnabled()) {
    // Device nodes for V4L2 video decode accelerator drivers.
    static const base::FilePath::CharType kDevicePath[] =
        FILE_PATH_LITERAL("/dev/");
    static const base::FilePath::CharType kVideoDecPattern[] = "video-dec[0-9]";
    base::FileEnumerator enumerator(base::FilePath(kDevicePath), false,
                                    base::FileEnumerator::FILES,
                                    base::FilePath(kVideoDecPattern).value());
    for (base::FilePath name = enumerator.Next(); !name.empty();
         name = enumerator.Next())
      permissions->push_back(BrokerFilePermission::ReadWrite(name.value()));
  }

  // Device node for V4L2 video encode accelerator drivers.
  static const char kDevVideoEncPath[] = "/dev/video-enc";
  permissions->push_back(BrokerFilePermission::ReadWrite(kDevVideoEncPath));

  // Device node for V4L2 JPEG decode accelerator drivers.
  static const char kDevJpegDecPath[] = "/dev/jpeg-dec";
  permissions->push_back(BrokerFilePermission::ReadWrite(kDevJpegDecPath));
}

void WarmUpPreGPUIndoResources() {
  {
    TRACE_EVENT0("gpu", "Warm up rand");
    // Warm up the random subsystem, which needs to be done pre-sandbox on all
    // platforms.
    (void)base::RandUint64();
  }

#if BUILDFLAG(USE_VAAPI)
  media::VaapiWrapper::PreSandboxInitialization();
#endif
#if defined(OS_WIN)
  media::DXVAVideoDecodeAccelerator::PreSandboxInitialization();
  media::MediaFoundationVideoEncodeAccelerator::PreSandboxInitialization();
#endif

  // On Linux, reading system memory doesn't work through the GPU sandbox.
  // This value is cached, so access it here to populate the cache.
  base::SysInfo::AmountOfPhysicalMemory();
}

}  // namespace

namespace gpu {

#if defined(OS_LINUX)
class ArmGpuSandboxHelper : public ContentGpuSandboxHelper {
 public:
  ArmGpuSandboxHelper(const GPUInfo* gpu_info);

  // SandboxHelper:
  void PreSandboxStartup() override;
  bool EnsureSandboxInitialized(GpuWatchdogThread* watchdog_thread) override;
};

class AmdGpuSandboxHelper : public ContentGpuSandboxHelper {
 public:
  ArmGpuSandboxHelper(const GPUInfo* gpu_info);

  // SandboxHelper:
  void PreSandboxStartup() override;
  bool EnsureSandboxInitialized(GpuWatchdogThread* watchdog_thread) override;
};
#endif  // defined(OS_LINUX)

// static
std::unique_ptr<SandboxHelper> SandboxHelper::Create(const GPUInfo* gpu_info) {
#if defined(OS_CHROMEOS)
#if defined(__arm__)
  return std::make_unique<ArmGpuSandboxHelper>(gpu_info());
#endif  // defined(__arm__)
  if (gpu_info() && angle::IsAMD(gpu_info()->active_gpu().vendor_id))
    return std::make_unique<AmdGpuSandboxHelper>(gpu_info());
#endif  // defined(OS_CHROMEOS)
  return std::make_unique<ContentGpuSandboxHelper>(gpu_info());
}

GPUSandboxHelper::GPUSandboxHelper(const GPUInfo* gpu_info)
    : SandboxInfo(gpu_info) {}

bool GPUSandboxHelper::PreSandboxStartup() {
  WarmUpPreGPUInfoResources();

  // Warm up resources needed by the policy we're about to enable and
  // eventually start a broker process.
  const bool chromeos_arm_gpu = IsChromeOS() && IsArchitectureArm();
  // This policy is for x86 or Desktop.
  DCHECK(!chromeos_arm_gpu);

  DCHECK(!broker_process());
  // Create a new broker process.
  InitGpuBrokerProcess(
      GpuBrokerProcessPolicy::Create,
      std::vector<BrokerFilePermission>());  // No extra files in whitelist.

  if (IsArchitectureX86_64() || IsArchitectureI386()) {
    // Accelerated video dlopen()'s some shared objects
    // inside the sandbox, so preload them now.
    if (IsAcceleratedVaapiVideoEncodeEnabled() ||
        IsAcceleratedVideoDecodeEnabled()) {
      const char* I965DrvVideoPath = NULL;
      const char* I965HybridDrvVideoPath = NULL;

      if (IsArchitectureX86_64()) {
        I965DrvVideoPath = "/usr/lib64/va/drivers/i965_drv_video.so";
        I965HybridDrvVideoPath = "/usr/lib64/va/drivers/hybrid_drv_video.so";
      } else if (IsArchitectureI386()) {
        I965DrvVideoPath = "/usr/lib/va/drivers/i965_drv_video.so";
      }

      dlopen(I965DrvVideoPath, RTLD_NOW | RTLD_GLOBAL | RTLD_NODELETE);
      if (I965HybridDrvVideoPath)
        dlopen(I965HybridDrvVideoPath, RTLD_NOW | RTLD_GLOBAL | RTLD_NODELETE);
      dlopen("libva.so.1", RTLD_NOW | RTLD_GLOBAL | RTLD_NODELETE);
#if defined(USE_OZONE)
      dlopen("libva-drm.so.1", RTLD_NOW | RTLD_GLOBAL | RTLD_NODELETE);
#elif defined(USE_X11)
      dlopen("libva-x11.so.1", RTLD_NOW | RTLD_GLOBAL | RTLD_NODELETE);
#endif
    }
  }
  return true;
}

bool GPUSandboxHelper::EnsureSandboxInitialized(
    GpuWatchdogThread* watchdog_thread) {
#if defined(OS_LINUX)
  return StartSandboxLinux(watchdog_thread, gpu_info);
#elif defined(OS_WIN)
  return StartSandboxWindows(sandbox_info_);
#elif defined(OS_MACOSX)
  return Sandbox::SandboxIsCurrentlyActive();
#else
  return false;
#endif
}

#if defined(OS_LINUX)
std::vector<BrokerFilePermission> ContentGpuSandboxHelper::EnumerateMumble(
    const std::vector<BrokerFilePermission>& permission_extra) {
  static const char kDriRcPath[] = "/etc/drirc";
  static const char kDriCardBasePath[] = "/dev/dri/card";
  static const char kNvidiaCtlPath[] = "/dev/nvidiactl";
  static const char kNvidiaDeviceBasePath[] = "/dev/nvidia";
  static const char kNvidiaParamsPath[] = "/proc/driver/nvidia/params";
  static const char kDevShm[] = "/dev/shm/";

  // All GPU process policies need these files brokered out.
  std::vector<BrokerFilePermission> permissions;
  permissions.push_back(BrokerFilePermission::ReadOnly(kDriRcPath));

  if (!IsChromeOS()) {
    // For shared memory.
    permissions.push_back(
        BrokerFilePermission::ReadWriteCreateUnlinkRecursive(kDevShm));
    // For DRI cards.
    for (int i = 0; i <= 9; ++i) {
      permissions.push_back(BrokerFilePermission::ReadWrite(
          base::StringPrintf("%s%d", kDriCardBasePath, i)));
    }
    // For Nvidia GLX driver.
    permissions.push_back(BrokerFilePermission::ReadWrite(kNvidiaCtlPath));
    for (int i = 0; i <= 9; ++i) {
      permissions.push_back(BrokerFilePermission::ReadWrite(
          base::StringPrintf("%s%d", kNvidiaDeviceBasePath, i)));
    }
    permissions.push_back(BrokerFilePermission::ReadOnly(kNvidiaParamsPath));
  } else if (UseV4L2Codec()) {
    AddV4L2GpuWhitelist(&permissions);
    if (UseLibV4L2()) {
      dlopen("/usr/lib/libv4l2.so", RTLD_NOW | RTLD_GLOBAL | RTLD_NODELETE);
      // This is a device-specific encoder plugin.
      dlopen("/usr/lib/libv4l/plugins/libv4l-encplugin.so",
             RTLD_NOW | RTLD_GLOBAL | RTLD_NODELETE);
    }
  }

  // Add eventual extra files from permissions_extra.
  for (const auto& perm : permissions_extra) {
    permissions.push_back(perm);
  }

  return permissions;
}
#endif  // defined(OS_LINUX)

#if defined(OS_CHROMEOS)
#if defined(__arm__)
ArmGPUSandboxHelper::GPUSandboxHelper(const GPUInfo* gpu_info)
    : GpuSandboxInfo(gpu_info) {}

void ArmGPUSandboxHelper::PreSandboxStartup() {
  WarmUpPreGPUInfoResources();

  DCHECK(IsChromeOS() && IsArchitectureArm());
  // Create a new broker process.
  DCHECK(!broker_process());

  // Add ARM-specific files to whitelist in the broker.
  std::vector<BrokerFilePermission> permissions;

  AddArmGpuWhitelist(&permissions);
  InitGpuBrokerProcess(CrosArmGpuBrokerProcessPolicy::Create, permissions);

  const int dlopen_flag = RTLD_NOW | RTLD_GLOBAL | RTLD_NODELETE;

  // Preload the Mali library.
  dlopen("/usr/lib/libmali.so", dlopen_flag);
  // Preload the Tegra V4L2 (video decode acceleration) library.
  dlopen("/usr/lib/libtegrav4l2.so", dlopen_flag);
  // Resetting errno since platform-specific libraries will fail on other
  // platforms.
  errno = 0;

  return true;
}

bool ArmGPUSandboxHelper::EnsureSandboxInitialized(
    GpuWatchdogThread* watchdog_thread) {
  // Options go here.
  return StartSandboxLinux(watchdog_thread, gpu_info);
}
#endif  // defined(__arm__)

AmdGPUSandboxHelper::GPUSandboxHelper(const GPUInfo* gpu_info)
    : GpuSandboxInfo(gpu_info) {}

bool AmdGPUSandboxHelper::PreSandboxStartup() {
  WarmUpPreGPUInfoResources();

  // Create a new broker process.
  DCHECK(!broker_process());

  // Add AMD-specific files to whitelist in the broker.
  std::vector<BrokerFilePermission> permissions;

  AddAmdGpuWhitelist(&permissions);
  InitGpuBrokerProcess(CrosAmdGpuBrokerProcessPolicy::Create, permissions);

  const int dlopen_flag = RTLD_NOW | RTLD_GLOBAL | RTLD_NODELETE;

  // Preload the amdgpu-dependent libraries.
  errno = 0;
  if (NULL == dlopen("libglapi.so", dlopen_flag)) {
    LOG(ERROR) << "dlopen(libglapi.so) failed with error: " << dlerror();
    return false;
  }
  if (NULL == dlopen("/usr/lib64/dri/swrast_dri.so", dlopen_flag)) {
    LOG(ERROR) << "dlopen(swrast_dri.so) failed with error: " << dlerror();
    return false;
  }
  if (NULL == dlopen("/usr/lib64/dri/radeonsi_dri.so", dlopen_flag)) {
    LOG(ERROR) << "dlopen(radeonsi_dri.so) failed with error: " << dlerror();
    return false;
  }

  return true;
}

bool AmdGPUSandboxHelper::EnsureSandboxInitialized(
    GpuWatchdogThread* watchdog_thread) {
  // Options go here.
  return StartSandboxLinux(watchdog_thread, gpu_info);
}

#endif  // defined(OS_CHROMEOS)

}  // namespace gpu
