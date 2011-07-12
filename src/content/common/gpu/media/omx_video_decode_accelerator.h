// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_COMMON_GPU_MEDIA_OMX_VIDEO_DECODE_ACCELERATOR_H_
#define CONTENT_COMMON_GPU_MEDIA_OMX_VIDEO_DECODE_ACCELERATOR_H_

#include <dlfcn.h>
#include <map>
#include <queue>
#include <set>
#include <string>
#include <utility>
#include <vector>

#include "base/bind.h"
#include "base/callback.h"
#include "base/logging.h"
#include "base/message_loop.h"
#include "base/shared_memory.h"
#include "media/video/video_decode_accelerator.h"
#include "third_party/angle/include/EGL/egl.h"
#include "third_party/angle/include/EGL/eglext.h"
#include "third_party/openmax/il/OMX_Component.h"
#include "third_party/openmax/il/OMX_Core.h"
#include "third_party/openmax/il/OMX_Video.h"

// Class to wrap OpenMAX IL accelerator behind VideoDecodeAccelerator interface.
// The implementation assumes an OpenMAX IL 1.1.2 implementation conforming to
// http://www.khronos.org/registry/omxil/specs/OpenMAX_IL_1_1_2_Specification.pdf
//
// This class lives on a single thread and DCHECKs that it is never accessed
// from any other.  OMX callbacks are trampolined from the OMX component's
// thread to maintain this invariant.
class OmxVideoDecodeAccelerator : public media::VideoDecodeAccelerator {
 public:
  // Does not take ownership of |client| which must outlive |*this|.
  OmxVideoDecodeAccelerator(media::VideoDecodeAccelerator::Client* client);
  virtual ~OmxVideoDecodeAccelerator();

  // media::VideoDecodeAccelerator implementation.
  bool GetConfigs(const std::vector<uint32>& requested_configs,
                  std::vector<uint32>* matched_configs) OVERRIDE;
  bool Initialize(const std::vector<uint32>& config) OVERRIDE;
  void Decode(const media::BitstreamBuffer& bitstream_buffer) OVERRIDE;
  virtual void AssignGLESBuffers(
      const std::vector<media::GLESBuffer>& buffers) OVERRIDE;
  virtual void AssignSysmemBuffers(
      const std::vector<media::SysmemBuffer>& buffers) OVERRIDE;
  void ReusePictureBuffer(int32 picture_buffer_id) OVERRIDE;
  void Flush() OVERRIDE;
  void Abort() OVERRIDE;

  void SetEglState(EGLDisplay egl_display, EGLContext egl_context);

 private:
  // Helper struct for keeping track of the relationship between an OMX output
  // buffer and the GLESBuffer it points to.
  struct OutputPicture {
    OutputPicture(media::GLESBuffer g_b, OMX_BUFFERHEADERTYPE* o_b_h,
                  EGLImageKHR e_i)
        : gles_buffer(g_b), omx_buffer_header(o_b_h),  egl_image(e_i) {}
    media::GLESBuffer gles_buffer;
    OMX_BUFFERHEADERTYPE* omx_buffer_header;
    EGLImageKHR egl_image;
  };
  typedef std::map<int32, OutputPicture> OutputPictureById;

  MessageLoop* message_loop_;
  OMX_HANDLETYPE component_handle_;

  // Create the Component for OMX. Handles all OMX initialization.
  bool CreateComponent();
  // Buffer allocation/free methods for input and output buffers.
  bool AllocateInputBuffers();
  bool AllocateOutputBuffers();
  void FreeInputBuffers();
  void FreeOutputBuffers();

  // Methods to handle OMX state transitions.
  bool TransitionToState(OMX_STATETYPE new_state);
  void OnStateChangeLoadedToIdle(OMX_STATETYPE state);
  void OnStateChangeIdleToExecuting(OMX_STATETYPE state);
  void OnStateChangeExecutingToIdle(OMX_STATETYPE state);
  void OnStateChangeIdleToLoaded(OMX_STATETYPE state);
  // Stop the components when error is detected.
  void StopOnError(media::VideoDecodeAccelerator::Error error);
  // Methods for shutdown
  void PauseFromExecuting(OMX_STATETYPE ignored);
  void FlushIOPorts();
  void InputPortFlushDone(int port);
  void OutputPortFlushDone(int port);
  void FlushBegin();
  void ShutDownOMXFromExecuting();

  // Determine whether we actually start decoding the bitstream.
  bool CanAcceptInput();
  // Determine whether we can issue fill buffer or empty buffer
  // to the decoder based on the current state and port state.
  bool CanEmptyBuffer();
  bool CanFillBuffer();
  void OnIndexParamPortDefinitionChanged(int port);
  // Whenever port settings change, the first thing we must do is disable the
  // port (see Figure 3-18 of the OpenMAX IL spec linked to above).  When the
  // port is disabled, the component will call us back here.  We then re-enable
  // the port once we have textures, and that's the second method below.
  void PortDisabledForSettingsChange(int port);
  void PortEnabledAfterSettingsChange(int port);

  // IL-client state.
  OMX_STATETYPE client_state_;

  // Following are input port related variables.
  int input_buffer_count_;
  int input_buffer_size_;
  int input_port_;
  int input_buffers_at_component_;

  // Following are output port related variables.
  int output_port_;
  int output_buffers_at_component_;

  // NOTE: someday there may be multiple contexts for a single decoder.  But not
  // today.
  // TODO(fischman,vrk): handle lost contexts?
  EGLDisplay egl_display_;
  EGLContext egl_context_;

  // Free input OpenMAX buffers that can be used to take bitstream from demuxer.
  std::queue<OMX_BUFFERHEADERTYPE*> free_input_buffers_;

  // For output buffer recycling cases.
  OutputPictureById pictures_;

  // To kick the component from Loaded to Idle before we know the real size of
  // the video (so can't yet ask for textures) we populate it with fake output
  // buffers.  Keep track of them here.
  // TODO(fischman): do away with this madness.
  std::set<OMX_BUFFERHEADERTYPE*> fake_output_buffers_;

  // To expose client callbacks from VideoDecodeAccelerator.
  // NOTE: all calls to this object *MUST* be executed in message_loop_.
  Client* client_;

  // Method to handle events
  void EventHandlerCompleteTask(OMX_EVENTTYPE event,
                                OMX_U32 data1,
                                OMX_U32 data2);

  // Method to receive buffers from component's input port
  void EmptyBufferDoneTask(OMX_BUFFERHEADERTYPE* buffer);

  // Method to receive buffers from component's output port
  void FillBufferDoneTask(OMX_BUFFERHEADERTYPE* buffer);

  // Method used the change the state of the port.
  void ChangePort(OMX_COMMANDTYPE cmd, int port_index);

  // Member function pointers to respond to events
  void (OmxVideoDecodeAccelerator::*on_port_disable_event_func_)(int port);
  void (OmxVideoDecodeAccelerator::*on_port_enable_event_func_)(int port);
  void (OmxVideoDecodeAccelerator::*on_state_event_func_)(OMX_STATETYPE state);
  void (OmxVideoDecodeAccelerator::*on_flush_event_func_)(int port);
  void (OmxVideoDecodeAccelerator::*on_buffer_flag_event_func_)();

  // Callback methods for the OMX component.
  // When these callbacks are received, the
  // call is delegated to the three internal methods above.
  static OMX_ERRORTYPE EventHandler(OMX_HANDLETYPE component,
                                    OMX_PTR priv_data,
                                    OMX_EVENTTYPE event,
                                    OMX_U32 data1, OMX_U32 data2,
                                    OMX_PTR event_data);
  static OMX_ERRORTYPE EmptyBufferCallback(OMX_HANDLETYPE component,
                                           OMX_PTR priv_data,
                                           OMX_BUFFERHEADERTYPE* buffer);
  static OMX_ERRORTYPE FillBufferCallback(OMX_HANDLETYPE component,
                                          OMX_PTR priv_data,
                                          OMX_BUFFERHEADERTYPE* buffer);
};

#endif  // CONTENT_COMMON_GPU_MEDIA_OMX_VIDEO_DECODE_ACCELERATOR_H_
