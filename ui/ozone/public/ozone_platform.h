// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_OZONE_PUBLIC_OZONE_PLATFORM_H_
#define UI_OZONE_PUBLIC_OZONE_PLATFORM_H_

#include <memory>

#include "base/macros.h"
#include "base/message_loop/message_loop.h"
#include "services/service_manager/public/cpp/bind_source_info.h"
#include "services/service_manager/public/cpp/binder_registry.h"
#include "ui/events/system_input_injector.h"
#include "ui/ozone/ozone_export.h"
#include "ui/ozone/public/interfaces/drm_device.mojom.h"

namespace display {
class NativeDisplayDelegate;
}

namespace gfx {
class Rect;
}

namespace IPC {
class MessageFilter;
}

namespace service_manager {
class Connector;
}

namespace ui {

class CursorFactoryOzone;
class InputController;
class GpuPlatformSupportHost;
class OverlayManagerOzone;
class PlatformWindow;
class PlatformWindowDelegate;
class SurfaceFactoryOzone;
class SystemInputInjector;

// Base class for Ozone platform implementations.
//
// Ozone platforms must override this class and implement the virtual
// GetFooFactoryOzone() methods to provide implementations of the
// various ozone interfaces.
//
// The OzonePlatform subclass can own any state needed by the
// implementation that is shared between the various ozone interfaces,
// such as a connection to the windowing system.
//
// A platform is free to use different implementations of each
// interface depending on the context. You can, for example, create
// different objects depending on the underlying hardware, command
// line flags, or whatever is appropriate for the platform.
class OZONE_EXPORT OzonePlatform {
 public:
  OzonePlatform();
  virtual ~OzonePlatform();

  // Additional initialization params for the platform. Platforms must not
  // retain a reference to this structure.
  struct InitParams {
    // Ozone may retain this pointer for later use. An Ozone platform embedder
    // must set this parameter in order for the Ozone platform implementation to
    // be able to use Mojo.
    service_manager::Connector* connector = nullptr;

    // Setting this to true indicates that the platform implementation should
    // operate as a single process for platforms (i.e. drm) that are usually
    // split between a host and viz specific portion.
    bool single_process = false;

    //  Setting this to true indicates that the platform implementation should
    //  use mojo. Setting this to true requires calling |AddInterfaces|
    //  afterwards in the Viz process and |SetServiceManagerConnector| in the
    //  Host process.
    bool using_mojo = false;
  };

  // Ensures the OzonePlatform instance without doing any initialization.
  // No-op in case the instance is already created.
  // This is useful in order call virtual methods that depend on the ozone
  // platform selected at runtime, e.g. ::GetMessageLoopTypeForGpu.
  static OzonePlatform* EnsureInstance();

  // Initializes the subsystems/resources necessary for the UI process (e.g.
  // events) with additional properties to customize the ozone platform
  // implementation. Ozone will not retain InitParams after returning from
  // InitalizeForUI.
  static void InitializeForUI(const InitParams& args);

  // Initializes the subsystems for rendering but with additional properties
  // provided by |args| as with InitalizeForUI.
  static void InitializeForGPU(const InitParams& args);

  // Deletes the instance. Does nothing if OzonePlatform has not yet been
  // initialized.
  static void Shutdown();

  static OzonePlatform* GetInstance();

  // Factory getters to override in subclasses. The returned objects will be
  // injected into the appropriate layer at startup. Subclasses should not
  // inject these objects themselves. Ownership is retained by OzonePlatform.
  virtual ui::SurfaceFactoryOzone* GetSurfaceFactoryOzone() = 0;
  virtual ui::OverlayManagerOzone* GetOverlayManager() = 0;
  virtual ui::CursorFactoryOzone* GetCursorFactoryOzone() = 0;
  virtual ui::InputController* GetInputController() = 0;
  virtual IPC::MessageFilter* GetGpuMessageFilter();
  virtual ui::GpuPlatformSupportHost* GetGpuPlatformSupportHost() = 0;
  virtual std::unique_ptr<SystemInputInjector> CreateSystemInputInjector() = 0;
  virtual std::unique_ptr<PlatformWindow> CreatePlatformWindow(
      PlatformWindowDelegate* delegate,
      const gfx::Rect& bounds) = 0;
  virtual std::unique_ptr<display::NativeDisplayDelegate>
  CreateNativeDisplayDelegate() = 0;

  // Returns the message loop type required for OzonePlatform instance that
  // will be initialized for the GPU process.
  virtual base::MessageLoop::Type GetMessageLoopTypeForGpu();

// TODO(rjk) ---------- update
  // Provides a service_manager::Connector* to the ozone host component (the UI)
  // on platforms that require it. This is separate from |InitializeForUI|
  // because the Chrome initialization calls |InitializeForUI| prior to setting
  // up Mojo. Ozone assumes that the given |Connector*| is valid and usable
  // after calling this method. On ozone platforms that use Mojo IPC, it is
  // essential to call this entry point after |InitializeForUI| but before any
  // other entry points.
  //
  // It is unnecessary to call this entry point on platforms that do not use
  // Mojo.
  //
  // Ozone may retain the connector pointer for later use and it must remain
  // valid until Shutdown has been invoked.
  //
  // A default do-nothing implementation is provided to permit platform
  // implementations to opt out of consuming any Mojo interfaces.
// -----------------
  virtual void SetServiceManagerInterfacePtr(ui::ozone::mojom::DrmDevicePtr drm_device_ptr);

  // Ozone platform implementations may also choose to expose mojo interfaces to
  // internal functionality. Embedders wishing to take advantage of ozone mojo
  // implementations must invoke AddInterfaces with a valid
  // service_manager::BinderRegistry* pointer to export all Mojo interfaces
  // defined within Ozone.
  //
  // Requests arriving before they can be immediately handled will be queued and
  // executed later.
  //
  // A default do-nothing implementation is provided to permit platform
  // implementations to opt out of implementing any Mojo interfaces.
  virtual void AddInterfaces(service_manager::BinderRegistryWithArgs<
                             const service_manager::BindSourceInfo&>* registry);

  // The GPU-specific portion of Ozone would typically run in a sandboxed
  // process for additional security. Some startup might need to wait until
  // after the sandbox has been configured. The embedder should use this method
  // to specify that the sandbox is configured and that GPU-side setup should
  // complete. A default do-nothing implementation is provided to permit
  // platform implementations to ignore sandboxing and any associated launch
  // ordering issues.
  virtual void AfterSandboxEntry();

 private:
  virtual void InitializeUI(const InitParams& params) = 0;
  virtual void InitializeGPU(const InitParams& params) = 0;

  static OzonePlatform* instance_;

  DISALLOW_COPY_AND_ASSIGN(OzonePlatform);
};

}  // namespace ui

#endif  // UI_OZONE_PUBLIC_OZONE_PLATFORM_H_
