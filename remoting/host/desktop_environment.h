// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef REMOTING_HOST_DESKTOP_ENVIRONMENT_H_
#define REMOTING_HOST_DESKTOP_ENVIRONMENT_H_

#include <cstdint>
#include <memory>
#include <string>

#include "base/callback.h"
#include "base/callback_forward.h"
#include "base/memory/ref_counted.h"
#include "base/memory/weak_ptr.h"
#include "remoting/host/desktop_environment_options.h"
#include "remoting/proto/process_stats.pb.h"

namespace webrtc {
class DesktopCapturer;
class MouseCursorMonitor;
}  // namespace webrtc

namespace remoting {

class AudioCapturer;
class ClientSessionControl;
class FileProxyWrapper;
class InputInjector;
class ScreenControls;

// Provides factory methods for creation of audio/video capturers and event
// executor for a given desktop environment.
class DesktopEnvironment {
 public:
  virtual ~DesktopEnvironment() {}

  // Factory methods used to create audio/video capturers, event executor, and
  // screen controls object for a particular desktop environment.
  virtual std::unique_ptr<AudioCapturer> CreateAudioCapturer() = 0;
  virtual std::unique_ptr<InputInjector> CreateInputInjector() = 0;
  virtual std::unique_ptr<ScreenControls> CreateScreenControls() = 0;
  virtual std::unique_ptr<webrtc::DesktopCapturer> CreateVideoCapturer() = 0;
  virtual std::unique_ptr<webrtc::MouseCursorMonitor>
  CreateMouseCursorMonitor() = 0;
  virtual std::unique_ptr<FileProxyWrapper> CreateFileProxyWrapper() = 0;

  // Returns the set of all capabilities supported by |this|.
  virtual std::string GetCapabilities() const = 0;

  // Passes the final set of capabilities negotiated between the client and host
  // to |this|.
  virtual void SetCapabilities(const std::string& capabilities) = 0;

  // Returns an id which identifies the current desktop session on Windows.
  // Other platforms will always return the default value (UINT32_MAX).
  virtual uint32_t GetDesktopSessionId() const = 0;

  // Returns the resource usage of host processes associated with current
  // session. On Windows, the return value covers desktop, network, daemon and
  // rdp (in curtain mode) processes. On other platforms, only the resource
  // usage of current process will be returned. The callback will carry the
  // result, but it may be executed in a random thread. In case of error, the
  // callback may be executed with an empty AggregatedProcessResourceUsage.
  using HostResourceUsageCallback =
      base::OnceCallback<void(const protocol::AggregatedProcessResourceUsage&)>;
  virtual void GetHostResourceUsage(HostResourceUsageCallback on_result) = 0;
};

// Used to create |DesktopEnvironment| instances.
class DesktopEnvironmentFactory {
 public:
  virtual ~DesktopEnvironmentFactory() {}

  // Creates an instance of |DesktopEnvironment|. Returns a nullptr pointer if
  // the desktop environment could not be created for any reason (if the curtain
  // failed to active for instance). |client_session_control| must outlive
  // the created desktop environment.
  virtual std::unique_ptr<DesktopEnvironment> Create(
      base::WeakPtr<ClientSessionControl> client_session_control,
      const DesktopEnvironmentOptions& options) = 0;

  // Returns |true| if created |DesktopEnvironment| instances support audio
  // capture.
  virtual bool SupportsAudioCapture() const = 0;
};

}  // namespace remoting

#endif  // REMOTING_HOST_DESKTOP_ENVIRONMENT_H_
