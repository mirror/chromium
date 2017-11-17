// Copyright (c) 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/viz/common/gpu/vulkan_in_process_context_provider.h"
#include "gpu/vulkan/features.h"

#if BUILDFLAG(ENABLE_VULKAN)
#include "gpu/vulkan/vulkan_device_queue.h"
#include "gpu/vulkan/vulkan_implementation.h"
#include "third_party/skia/include/gpu/GrContext.h"
#include "third_party/skia/include/gpu/vk/GrVkInterface.h"
#endif  // BUILDFLAG(ENABLE_VULKAN)

namespace viz {

scoped_refptr<VulkanInProcessContextProvider>
VulkanInProcessContextProvider::Create() {
#if BUILDFLAG(ENABLE_VULKAN)
  if (!gpu::VulkanSupported())
    return nullptr;

  scoped_refptr<VulkanInProcessContextProvider> context_provider(
      new VulkanInProcessContextProvider);
  if (!context_provider->Initialize())
    return nullptr;
  return context_provider;
#else
  return nullptr;
#endif
}

#if BUILDFLAG(ENABLE_VULKAN)
#include <dlfcn.h>

void* load_dynamic_library(const char* library_name) {
  return dlopen(library_name, RTLD_LAZY);
}

void* get_procedure_address(void* library, const char* function_name) {
  return dlsym(library, function_name);
}

bool load_vklibrary_getprocaddr(PFN_vkGetInstanceProcAddr* instance_proc,
                                PFN_vkGetDeviceProcAddr* device_proc) {
  static void* vk_lib = nullptr;
  static PFN_vkGetInstanceProcAddr local_instance_proc = nullptr;
  static PFN_vkGetDeviceProcAddr local_device_proc = nullptr;
  if (!vk_lib) {
    vk_lib = load_dynamic_library("libvulkan.so");
    if (!vk_lib) {
      return false;
    }
    local_instance_proc = (PFN_vkGetInstanceProcAddr)get_procedure_address(
        vk_lib, "vkGetInstanceProcAddr");
    local_device_proc = (PFN_vkGetDeviceProcAddr)get_procedure_address(
        vk_lib, "vkGetDeviceProcAddr");
  }
  if (!local_instance_proc || !local_device_proc)
    return false;

  *instance_proc = local_instance_proc;
  *device_proc = local_device_proc;
  return true;
}

GrVkInterface::GetProc make_unified_getter(
    const GrVkInterface::GetInstanceProc& iproc,
    const GrVkInterface::GetDeviceProc& dproc) {
  return [&iproc, &dproc](const char* proc_name, VkInstance instance,
                          VkDevice device) {
    if (device != VK_NULL_HANDLE) {
      return dproc(device, proc_name);
    }
    return iproc(instance, proc_name);
  };
}
#endif

bool VulkanInProcessContextProvider::Initialize() {
#if BUILDFLAG(ENABLE_VULKAN)
  DCHECK(!device_queue_);
  std::unique_ptr<gpu::VulkanDeviceQueue> device_queue(
      new gpu::VulkanDeviceQueue);
  if (!device_queue->Initialize(
          gpu::VulkanDeviceQueue::GRAPHICS_QUEUE_FLAG |
          gpu::VulkanDeviceQueue::PRESENTATION_SUPPORT_QUEUE_FLAG)) {
    device_queue->Destroy();
    return false;
  }

  device_queue_ = std::move(device_queue);
  const uint32_t feature_flags = kGeometryShader_GrVkFeatureFlag |
                                 kDualSrcBlend_GrVkFeatureFlag |
                                 kSampleRateShading_GrVkFeatureFlag;
  const uint32_t extension_flags =
      kEXT_debug_report_GrVkExtensionFlag | kKHR_surface_GrVkExtensionFlag |
      kKHR_swapchain_GrVkExtensionFlag | kKHR_xcb_surface_GrVkExtensionFlag;
  GrVkBackendContext* backend_context = new GrVkBackendContext();
  backend_context->fInstance = device_queue_->GetVulkanInstance();
  backend_context->fPhysicalDevice = device_queue_->GetVulkanPhysicalDevice();
  backend_context->fDevice = device_queue_->GetVulkanDevice();
  backend_context->fQueue = device_queue_->GetVulkanQueue();
  backend_context->fGraphicsQueueIndex = device_queue_->GetVulkanQueueIndex();
  backend_context->fMinAPIVersion = VK_MAKE_VERSION(1, 0, 8);
  backend_context->fExtensions = extension_flags;
  backend_context->fFeatures = feature_flags;
  PFN_vkGetInstanceProcAddr inst_proc;
  PFN_vkGetDeviceProcAddr dev_proc;
  if (!load_vklibrary_getprocaddr(&inst_proc, &dev_proc))
    return false;

  auto interface = sk_make_sp<GrVkInterface>(
      make_unified_getter(inst_proc, dev_proc), backend_context->fInstance,
      backend_context->fDevice, backend_context->fExtensions);
  backend_context->fInterface.reset(interface.release());
  backend_context->fOwnsInstanceAndDevice = false;
  backend_context_.reset(backend_context);
  gr_context_ = GrContext::Create(
      kVulkan_GrBackend,
      reinterpret_cast<GrBackendContext>(backend_context_.get()));
  return true;
#else
  return false;
#endif
}

void VulkanInProcessContextProvider::Destroy() {
#if BUILDFLAG(ENABLE_VULKAN)
  if (device_queue_) {
    device_queue_->Destroy();
    device_queue_.reset();
    backend_context_.reset();
  }
#endif
}

GrContext* VulkanInProcessContextProvider::GetGrContext() {
  return gr_context_;
}

gpu::VulkanDeviceQueue* VulkanInProcessContextProvider::GetDeviceQueue() {
#if BUILDFLAG(ENABLE_VULKAN)
  return device_queue_.get();
#else
  return nullptr;
#endif
}

VulkanInProcessContextProvider::VulkanInProcessContextProvider() {}

VulkanInProcessContextProvider::~VulkanInProcessContextProvider() {
  Destroy();
}

}  // namespace viz
