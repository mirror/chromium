// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/ozone/platform/dri/dri_gpu_platform_support.h"

#include "base/bind.h"
#include "base/thread_task_runner_handle.h"
#include "ipc/ipc_message_macros.h"
#include "ui/display/types/display_mode.h"
#include "ui/display/types/display_snapshot.h"
#include "ui/ozone/common/display_util.h"
#include "ui/ozone/common/gpu/ozone_gpu_message_params.h"
#include "ui/ozone/common/gpu/ozone_gpu_messages.h"
#include "ui/ozone/platform/dri/dri_window_delegate_impl.h"
#include "ui/ozone/platform/dri/dri_window_delegate_manager.h"
#include "ui/ozone/platform/dri/native_display_delegate_dri.h"

namespace ui {

namespace {

void MessageProcessedOnMain(
    scoped_refptr<base::SingleThreadTaskRunner> io_thread_task_runner,
    const base::Closure& io_thread_task) {
  io_thread_task_runner->PostTask(FROM_HERE, io_thread_task);
}

class DriGpuPlatformSupportMessageFilter : public IPC::MessageFilter {
 public:
  DriGpuPlatformSupportMessageFilter(DriWindowDelegateManager* window_manager,
                                     IPC::Listener* main_thread_listener)
      : window_manager_(window_manager),
        main_thread_listener_(main_thread_listener),
        main_thread_task_runner_(base::ThreadTaskRunnerHandle::Get()),
        pending_main_thread_operations_(0),
        cursor_animating_(false),
        start_on_main_(true) {}

  void OnFilterAdded(IPC::Sender* sender) override {
    io_thread_task_runner_ = base::ThreadTaskRunnerHandle::Get();
  }

  // This code is meant to be very temporary and only as a special case to fix
  // cursor movement jank resulting from slowdowns on the gpu main thread.
  // It handles cursor movement on IO thread when display config is stable
  // and returns it to main thread during transitions.
  bool OnMessageReceived(const IPC::Message& message) override {
    // If this message affects the state needed to set cursor, handle it on
    // the main thread. If a cursor move message arrives but we haven't
    // processed the previous main thread message, keep processing on main
    // until nothing is pending.
    bool cursor_position_message = MessageAffectsCursorPosition(message.type());
    bool cursor_state_message = MessageAffectsCursorState(message.type());

    // Only handle cursor related messages here.
    if (!cursor_position_message && !cursor_state_message)
      return false;

    bool cursor_was_animating = cursor_animating_;
    UpdateAnimationState(message);
    if (cursor_state_message || pending_main_thread_operations_ ||
        cursor_animating_ || cursor_was_animating || start_on_main_) {
      start_on_main_ = false;
      pending_main_thread_operations_++;

      base::Closure main_thread_message_handler =
          base::Bind(base::IgnoreResult(&IPC::Listener::OnMessageReceived),
                     base::Unretained(main_thread_listener_), message);
      main_thread_task_runner_->PostTask(FROM_HERE,
                                         main_thread_message_handler);

      // This is an echo from the main thread to decrement pending ops.
      // When the main thread is done with the task, it posts back to IO to
      // signal completion.
      base::Closure io_thread_task = base::Bind(
          &DriGpuPlatformSupportMessageFilter::DecrementPendingOperationsOnIO,
          this);

      base::Closure message_processed_callback = base::Bind(
          &MessageProcessedOnMain, io_thread_task_runner_, io_thread_task);
      main_thread_task_runner_->PostTask(FROM_HERE, message_processed_callback);

      return true;
    }

    // Otherwise, we are in a steady state and it's safe to move cursor on IO.
    bool handled = true;
    IPC_BEGIN_MESSAGE_MAP(DriGpuPlatformSupportMessageFilter, message)
    IPC_MESSAGE_HANDLER(OzoneGpuMsg_CursorMove, OnCursorMove)
    IPC_MESSAGE_HANDLER(OzoneGpuMsg_CursorSet, OnCursorSet)
    IPC_MESSAGE_UNHANDLED(handled = false);
    IPC_END_MESSAGE_MAP()

    return handled;
  }

 protected:
  ~DriGpuPlatformSupportMessageFilter() override {}

  void OnCursorMove(gfx::AcceleratedWidget widget, const gfx::Point& location) {
    window_manager_->GetWindowDelegate(widget)->MoveCursor(location);
  }

  void OnCursorSet(gfx::AcceleratedWidget widget,
                   const std::vector<SkBitmap>& bitmaps,
                   const gfx::Point& location,
                   int frame_delay_ms) {
    window_manager_->GetWindowDelegate(widget)
        ->SetCursorWithoutAnimations(bitmaps, location);
  }

  void DecrementPendingOperationsOnIO() { pending_main_thread_operations_--; }

  bool MessageAffectsCursorState(uint32 message_type) {
    switch (message_type) {
      case OzoneGpuMsg_CreateWindowDelegate::ID:
      case OzoneGpuMsg_DestroyWindowDelegate::ID:
      case OzoneGpuMsg_WindowBoundsChanged::ID:
      case OzoneGpuMsg_ConfigureNativeDisplay::ID:
      case OzoneGpuMsg_DisableNativeDisplay::ID:
        return true;
      default:
        return false;
    }
  }

  bool MessageAffectsCursorPosition(uint32 message_type) {
    switch (message_type) {
      case OzoneGpuMsg_CursorMove::ID:
      case OzoneGpuMsg_CursorSet::ID:
        return true;
      default:
        return false;
    }
  }

  void UpdateAnimationState(const IPC::Message& message) {
    if (message.type() != OzoneGpuMsg_CursorSet::ID)
      return;

    OzoneGpuMsg_CursorSet::Param param;
    if (!OzoneGpuMsg_CursorSet::Read(&message, &param))
      return;

    int frame_delay_ms = get<3>(param);
    cursor_animating_ = frame_delay_ms != 0;
  }

  DriWindowDelegateManager* window_manager_;
  IPC::Listener* main_thread_listener_;
  scoped_refptr<base::SingleThreadTaskRunner> main_thread_task_runner_;
  scoped_refptr<base::SingleThreadTaskRunner> io_thread_task_runner_;
  int32 pending_main_thread_operations_;
  bool cursor_animating_;
  // The filter is not installed early enough, so some messages may
  // get routed to main thread. Once we get our first message on io, send it
  // to main to ensure all prior main thread operations are finished before we
  // continue on io.
  // TODO: remove this once filter is properly installed.
  bool start_on_main_;
};
}

DriGpuPlatformSupport::DriGpuPlatformSupport(
    DriWrapper* drm,
    DriWindowDelegateManager* window_manager,
    ScreenManager* screen_manager,
    scoped_ptr<NativeDisplayDelegateDri> ndd)
    : sender_(NULL),
      drm_(drm),
      window_manager_(window_manager),
      screen_manager_(screen_manager),
      ndd_(ndd.Pass()) {
  filter_ = new DriGpuPlatformSupportMessageFilter(window_manager, this);
}

DriGpuPlatformSupport::~DriGpuPlatformSupport() {
}

void DriGpuPlatformSupport::AddHandler(scoped_ptr<GpuPlatformSupport> handler) {
  handlers_.push_back(handler.release());
}

void DriGpuPlatformSupport::OnChannelEstablished(IPC::Sender* sender) {
  sender_ = sender;

  for (size_t i = 0; i < handlers_.size(); ++i)
    handlers_[i]->OnChannelEstablished(sender);
}

bool DriGpuPlatformSupport::OnMessageReceived(const IPC::Message& message) {
  bool handled = true;

  IPC_BEGIN_MESSAGE_MAP(DriGpuPlatformSupport, message)
  IPC_MESSAGE_HANDLER(OzoneGpuMsg_CreateWindowDelegate, OnCreateWindowDelegate)
  IPC_MESSAGE_HANDLER(OzoneGpuMsg_DestroyWindowDelegate,
                      OnDestroyWindowDelegate)
  IPC_MESSAGE_HANDLER(OzoneGpuMsg_WindowBoundsChanged, OnWindowBoundsChanged)

  IPC_MESSAGE_HANDLER(OzoneGpuMsg_CursorSet, OnCursorSet)
  IPC_MESSAGE_HANDLER(OzoneGpuMsg_CursorMove, OnCursorMove)

  IPC_MESSAGE_HANDLER(OzoneGpuMsg_ForceDPMSOn, OnForceDPMSOn)
  IPC_MESSAGE_HANDLER(OzoneGpuMsg_RefreshNativeDisplays,
                      OnRefreshNativeDisplays)
  IPC_MESSAGE_HANDLER(OzoneGpuMsg_ConfigureNativeDisplay,
                      OnConfigureNativeDisplay)
  IPC_MESSAGE_HANDLER(OzoneGpuMsg_DisableNativeDisplay, OnDisableNativeDisplay)
  IPC_MESSAGE_HANDLER(OzoneGpuMsg_TakeDisplayControl, OnTakeDisplayControl)
  IPC_MESSAGE_HANDLER(OzoneGpuMsg_RelinquishDisplayControl,
                      OnRelinquishDisplayControl)
  IPC_MESSAGE_HANDLER(OzoneGpuMsg_AddGraphicsDevice, OnAddGraphicsDevice)
  IPC_MESSAGE_HANDLER(OzoneGpuMsg_RemoveGraphicsDevice, OnRemoveGraphicsDevice)
  IPC_MESSAGE_UNHANDLED(handled = false);
  IPC_END_MESSAGE_MAP()

  if (!handled)
    for (size_t i = 0; i < handlers_.size(); ++i)
      if (handlers_[i]->OnMessageReceived(message))
        return true;

  return false;
}

void DriGpuPlatformSupport::OnCreateWindowDelegate(
    gfx::AcceleratedWidget widget) {
  // Due to how the GPU process starts up this IPC call may happen after the IPC
  // to create a surface. Since a surface wants to know the window associated
  // with it, we create it ahead of time. So when this call happens we do not
  // create a delegate if it already exists.
  if (!window_manager_->HasWindowDelegate(widget)) {
    scoped_ptr<DriWindowDelegate> delegate(new DriWindowDelegateImpl(
        widget, drm_, window_manager_, screen_manager_));
    delegate->Initialize();
    window_manager_->AddWindowDelegate(widget, delegate.Pass());
  }
}

void DriGpuPlatformSupport::OnDestroyWindowDelegate(
    gfx::AcceleratedWidget widget) {
  scoped_ptr<DriWindowDelegate> delegate =
      window_manager_->RemoveWindowDelegate(widget);
  delegate->Shutdown();
}

void DriGpuPlatformSupport::OnWindowBoundsChanged(gfx::AcceleratedWidget widget,
                                                  const gfx::Rect& bounds) {
  window_manager_->GetWindowDelegate(widget)->OnBoundsChanged(bounds);
}

void DriGpuPlatformSupport::OnCursorSet(gfx::AcceleratedWidget widget,
                                        const std::vector<SkBitmap>& bitmaps,
                                        const gfx::Point& location,
                                        int frame_delay_ms) {
  window_manager_->GetWindowDelegate(widget)
      ->SetCursor(bitmaps, location, frame_delay_ms);
}

void DriGpuPlatformSupport::OnCursorMove(gfx::AcceleratedWidget widget,
                                         const gfx::Point& location) {
  window_manager_->GetWindowDelegate(widget)->MoveCursor(location);
}

void DriGpuPlatformSupport::OnForceDPMSOn() {
  ndd_->ForceDPMSOn();
}

void DriGpuPlatformSupport::OnRefreshNativeDisplays() {
  std::vector<DisplaySnapshot_Params> displays;
  std::vector<DisplaySnapshot*> native_displays = ndd_->GetDisplays();

  for (size_t i = 0; i < native_displays.size(); ++i)
    displays.push_back(GetDisplaySnapshotParams(*native_displays[i]));

  sender_->Send(new OzoneHostMsg_UpdateNativeDisplays(displays));
}

void DriGpuPlatformSupport::OnConfigureNativeDisplay(
    int64_t id,
    const DisplayMode_Params& mode_param,
    const gfx::Point& origin) {
  DisplaySnapshot* display = ndd_->FindDisplaySnapshot(id);
  if (!display) {
    LOG(ERROR) << "There is no display with ID " << id;
    sender_->Send(new OzoneHostMsg_DisplayConfigured(id, false));
    return;
  }

  const DisplayMode* mode = NULL;
  for (size_t i = 0; i < display->modes().size(); ++i) {
    if (mode_param.size == display->modes()[i]->size() &&
        mode_param.is_interlaced == display->modes()[i]->is_interlaced() &&
        mode_param.refresh_rate == display->modes()[i]->refresh_rate()) {
      mode = display->modes()[i];
      break;
    }
  }

  // If the display doesn't have the mode natively, then lookup the mode from
  // other displays and try using it on the current display (some displays
  // support panel fitting and they can use different modes even if the mode
  // isn't explicitly declared).
  if (!mode)
    mode = ndd_->FindDisplayMode(mode_param.size, mode_param.is_interlaced,
                                 mode_param.refresh_rate);

  if (!mode) {
    LOG(ERROR) << "Failed to find mode: size=" << mode_param.size.ToString()
               << " is_interlaced=" << mode_param.is_interlaced
               << " refresh_rate=" << mode_param.refresh_rate;
    sender_->Send(new OzoneHostMsg_DisplayConfigured(id, false));
    return;
  }

  bool success = ndd_->Configure(*display, mode, origin);
  if (success) {
    display->set_origin(origin);
    display->set_current_mode(mode);
  }

  sender_->Send(new OzoneHostMsg_DisplayConfigured(id, success));
}

void DriGpuPlatformSupport::OnDisableNativeDisplay(int64_t id) {
  DisplaySnapshot* display = ndd_->FindDisplaySnapshot(id);
  bool success = false;
  if (display)
    success = ndd_->Configure(*display, NULL, gfx::Point());
  else
    LOG(ERROR) << "There is no display with ID " << id;

  sender_->Send(new OzoneHostMsg_DisplayConfigured(id, success));
}

void DriGpuPlatformSupport::OnTakeDisplayControl() {
  ndd_->TakeDisplayControl();
}

void DriGpuPlatformSupport::OnRelinquishDisplayControl() {
  ndd_->RelinquishDisplayControl();
}

void DriGpuPlatformSupport::OnAddGraphicsDevice(const base::FilePath& path) {
  NOTIMPLEMENTED();
}

void DriGpuPlatformSupport::OnRemoveGraphicsDevice(const base::FilePath& path) {
  NOTIMPLEMENTED();
}

void DriGpuPlatformSupport::RelinquishGpuResources(
    const base::Closure& callback) {
  callback.Run();
}

IPC::MessageFilter* DriGpuPlatformSupport::GetMessageFilter() {
  return filter_.get();
}

}  // namespace ui
