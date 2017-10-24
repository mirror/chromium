// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/renderer_host/media/video_capture_manager.h"

#include <algorithm>
#include <set>
#include <utility>

#include "base/bind.h"
#include "base/bind_helpers.h"
#include "base/command_line.h"
#include "base/location.h"
#include "base/logging.h"
#include "base/memory/ptr_util.h"
#include "base/metrics/histogram_macros.h"
#include "base/single_thread_task_runner.h"
#include "base/stl_util.h"
#include "base/strings/stringprintf.h"
#include "base/task_runner_util.h"
#include "base/threading/sequenced_worker_pool.h"
#include "base/threading/thread_task_runner_handle.h"
#include "build/build_config.h"
#include "content/browser/media/capture/desktop_capture_device_uma_types.h"
#include "content/browser/media/capture/web_contents_video_capture_device.h"
#include "content/browser/media/media_internals.h"
#include "content/browser/renderer_host/media/video_capture_controller.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/desktop_media_id.h"
#include "content/public/common/media_stream_request.h"
#include "media/base/bind_to_current_loop.h"
#include "media/base/media_switches.h"
#include "media/base/video_facing.h"
#include "media/capture/video/video_capture_buffer_pool_impl.h"
#include "media/capture/video/video_capture_buffer_tracker_factory_impl.h"
#include "media/capture/video/video_capture_device.h"
#include "media/capture/video/video_capture_device_client.h"
#include "media/capture/video/video_capture_device_factory.h"

#if defined(ENABLE_SCREEN_CAPTURE)

#if BUILDFLAG(ENABLE_WEBRTC) && !defined(OS_ANDROID)
#include "content/browser/media/capture/desktop_capture_device.h"
#endif

#if defined(USE_AURA)
#include "content/browser/media/capture/desktop_capture_device_aura.h"
#endif

#if defined(OS_ANDROID)
#include "content/browser/media/capture/screen_capture_device_android.h"
#endif

#endif  // defined(ENABLE_SCREEN_CAPTURE)

namespace {

// Used for logging capture events.
// Elements in this enum should not be deleted or rearranged; the only
// permitted operation is to add new elements before NUM_VIDEO_CAPTURE_EVENT.
enum VideoCaptureEvent {
  VIDEO_CAPTURE_START_CAPTURE = 0,
  VIDEO_CAPTURE_STOP_CAPTURE_OK = 1,
  VIDEO_CAPTURE_STOP_CAPTURE_DUE_TO_ERROR = 2,
  VIDEO_CAPTURE_STOP_CAPTURE_OK_NO_FRAMES_PRODUCED_BY_DEVICE = 3,
  VIDEO_CAPTURE_STOP_CAPTURE_OK_NO_FRAMES_PRODUCED_BY_DESKTOP_OR_TAB = 4,
  NUM_VIDEO_CAPTURE_EVENT
};

void LogVideoCaptureEvent(VideoCaptureEvent event) {
  UMA_HISTOGRAM_ENUMERATION("Media.VideoCaptureManager.Event", event,
                            NUM_VIDEO_CAPTURE_EVENT);
}

const media::VideoCaptureSessionId kFakeSessionId = -1;

}  // namespace

namespace content {

VideoCaptureManager::VideoCaptureManager(
    std::unique_ptr<MultiClientVideoCaptureProvider> video_capture_provider,
    base::RepeatingCallback<void(const std::string&)> emit_log_message_cb)
    : new_capture_session_id_(1),
      video_capture_provider_(std::move(video_capture_provider)),
      emit_log_message_cb_(std::move(emit_log_message_cb)) {}

VideoCaptureManager::~VideoCaptureManager() {
}

void VideoCaptureManager::AddVideoCaptureObserver(
    media::VideoCaptureObserver* observer) {
  DCHECK(observer);
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  capture_observers_.AddObserver(observer);
}

void VideoCaptureManager::RemoveAllVideoCaptureObservers() {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  capture_observers_.Clear();
}

void VideoCaptureManager::RegisterListener(
    MediaStreamProviderListener* listener) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  DCHECK(listener);
  listeners_.AddObserver(listener);
#if defined(OS_ANDROID)
  application_state_has_running_activities_ = true;
  app_status_listener_.reset(new base::android::ApplicationStatusListener(
      base::Bind(&VideoCaptureManager::OnApplicationStateChange,
                 base::Unretained(this))));
#endif
}

void VideoCaptureManager::UnregisterListener(
    MediaStreamProviderListener* listener) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  listeners_.RemoveObserver(listener);
}

void VideoCaptureManager::EnumerateDevices(
    const EnumerationCallback& client_callback) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  EmitLogMessage("VideoCaptureManager::EnumerateDevices", 1);

  // Pass a timer for UMA histogram collection.
  video_capture_provider_->GetDeviceInfosAsync(media::BindToCurrentLoop(
      base::Bind(&VideoCaptureManager::OnDeviceInfosReceived, this,
                 base::Owned(new base::ElapsedTimer()), client_callback)));
}

int VideoCaptureManager::Open(const MediaStreamDevice& device) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);

  // Generate a new id for the session being opened.
  const media::VideoCaptureSessionId capture_session_id =
      new_capture_session_id_++;

  DCHECK(sessions_.find(capture_session_id) == sessions_.end());
  std::ostringstream string_stream;
  string_stream << "VideoCaptureManager::Open, device.name = " << device.name
                << ", device.id = " << device.id
                << ", capture_session_id = " << capture_session_id;
  EmitLogMessage(string_stream.str(), 1);

  // We just save the stream info for processing later.
  sessions_[capture_session_id] = device;

  // Notify our listener asynchronously; this ensures that we return
  // |capture_session_id| to the caller of this function before using that
  // same id in a listener event.
  base::ThreadTaskRunnerHandle::Get()->PostTask(
      FROM_HERE, base::BindOnce(&VideoCaptureManager::OnOpened, this,
                                device.type, capture_session_id));
  return capture_session_id;
}

void VideoCaptureManager::Close(int capture_session_id) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  std::ostringstream string_stream;
  string_stream << "VideoCaptureManager::Close, capture_session_id = "
                << capture_session_id;
  EmitLogMessage(string_stream.str(), 1);

  SessionMap::iterator session_it = sessions_.find(capture_session_id);
  if (session_it == sessions_.end()) {
    NOTREACHED();
    return;
  }

  video_capture_provider_->StopAllSessionForDevice(session_it->second.type,
                                                   session_it->second.id);

  // Notify listeners asynchronously, and forget the session.
  base::ThreadTaskRunnerHandle::Get()->PostTask(
      FROM_HERE, base::BindOnce(&VideoCaptureManager::OnClosed, this,
                                session_it->second.type, capture_session_id));
  sessions_.erase(session_it);
}

void VideoCaptureManager::ConnectClient(
    media::VideoCaptureSessionId session_id,
    const media::VideoCaptureParams& params,
    VideoCaptureControllerID client_id,
    VideoCaptureControllerEventHandler* client_handler,
    ConnectClientCallback done_cb) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  std::ostringstream string_stream;
  string_stream << "ConnectClient: session_id = " << session_id << ", request: "
                << media::VideoCaptureFormat::ToString(params.requested_format);
  EmitLogMessage(string_stream.str(), 1);

  MediaStreamDevice device;
  if (!LookupDeviceFromSessionId(session_id, &device)) {
    done_cb.Run(base::WeakPtr<VideoCaptureSession>());
    return;
  }
  // Q: What if the session_id corresponds to a screen capture session?
  video_capture::mojom::CaptureSessionPtr capture_session;
  device_video_capture_provider_->ConnectToDevice(
      device_id, params, client_handler, mojo::MakeRequest(&capture_session),
      base::BindOnce([](video_capture::mojom::ConnectToDeviceAnswerPtr answer) {
        switch (answer->answer_type) {
          case video_capture::mojom::ConnectToDeviceAnswerType::kDeviceNotFound:
            done_cb.Run(
                controller->GetWeakPtrForIOThread()) case video_capture::mojom::
                ConnectToDeviceAnswerType::
                    kConnectionEstablishedWithDesiredSettings
                : case video_capture::mojom::ConnectToDeviceAnswerType::
                      kConnectionEstablishedWithDifferentSettings:
        }
        // TODO: handle answer.
        // We probably want to forward the answer to done_cb.
        // We probably want to store the capture_session so that we can
        // disconnect when needed.
      }));

  LogVideoCaptureEvent(VIDEO_CAPTURE_START_CAPTURE);

  done_cb.Run(controller->GetWeakPtrForIOThread());
}

void VideoCaptureManager::DisconnectClient(VideoCaptureControllerID client_id) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);

  // if (!aborted_due_to_error) {
  //   if (controller->has_received_frames()) {
  //     LogVideoCaptureEvent(VIDEO_CAPTURE_STOP_CAPTURE_OK);
  //   } else if (controller->stream_type() == MEDIA_DEVICE_VIDEO_CAPTURE) {
  //     LogVideoCaptureEvent(
  //         VIDEO_CAPTURE_STOP_CAPTURE_OK_NO_FRAMES_PRODUCED_BY_DEVICE);
  //   } else {
  //     LogVideoCaptureEvent(
  //         VIDEO_CAPTURE_STOP_CAPTURE_OK_NO_FRAMES_PRODUCED_BY_DESKTOP_OR_TAB);
  //   }
  // } else {
  //   LogVideoCaptureEvent(VIDEO_CAPTURE_STOP_CAPTURE_DUE_TO_ERROR);
  //   for (auto it : sessions_) {
  //     if (it.second.type == controller->stream_type() &&
  //         it.second.id == controller->device_id()) {
  //       for (auto& listener : listeners_)
  //         listener.Aborted(it.second.type, it.first);
  //       // Aborted() call might synchronously destroy |controller|, recheck.
  //       if (!IsControllerPointerValid(controller))
  //         return;
  //       break;
  //     }
  //   }
  // }

  // Detach client from controller.
  const media::VideoCaptureSessionId session_id =
      controller->RemoveClient(client_id, client_handler);
  std::ostringstream string_stream;
  string_stream << "DisconnectClient: session_id = " << session_id;
  EmitLogMessage(string_stream.str(), 1);

  // If controller has no more clients, delete controller and device.
  DestroyControllerIfNoClients(controller);
}

void VideoCaptureManager::PauseCaptureForClient(
    VideoCaptureController* controller,
    VideoCaptureControllerID client_id,
    VideoCaptureControllerEventHandler* client_handler) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  DCHECK(controller);
  DCHECK(client_handler);
  if (!IsControllerPointerValid(controller))
    NOTREACHED() << "Got Null controller while pausing capture";

  const bool had_active_client = controller->HasActiveClient();
  controller->PauseClient(client_id, client_handler);
  if (!had_active_client || controller->HasActiveClient())
    return;
  if (!controller->IsDeviceAlive())
    return;
  controller->MaybeSuspend();
}

void VideoCaptureManager::ResumeCaptureForClient(
    media::VideoCaptureSessionId session_id,
    const media::VideoCaptureParams& params,
    VideoCaptureController* controller,
    VideoCaptureControllerID client_id,
    VideoCaptureControllerEventHandler* client_handler) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  DCHECK(controller);
  DCHECK(client_handler);

  if (!IsControllerPointerValid(controller))
    NOTREACHED() << "Got Null controller while resuming capture";

  const bool had_active_client = controller->HasActiveClient();
  controller->ResumeClient(client_id, client_handler);
  if (had_active_client || !controller->HasActiveClient())
    return;
  if (!controller->IsDeviceAlive())
    return;
  controller->Resume();
}

void VideoCaptureManager::RequestRefreshFrameForClient(
    VideoCaptureController* controller) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);

  if (IsControllerPointerValid(controller)) {
    if (!controller->IsDeviceAlive())
      return;
    controller->RequestRefreshFrame();
  }
}

bool VideoCaptureManager::GetDeviceSupportedFormats(
    media::VideoCaptureSessionId capture_session_id,
    media::VideoCaptureFormats* supported_formats) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  DCHECK(supported_formats->empty());

  SessionMap::iterator it = sessions_.find(capture_session_id);
  if (it == sessions_.end())
    return false;
  std::ostringstream string_stream;
  string_stream << "GetDeviceSupportedFormats for device: " << it->second.name;
  EmitLogMessage(string_stream.str(), 1);

  return GetDeviceSupportedFormats(it->second.id, supported_formats);
}

bool VideoCaptureManager::GetDeviceSupportedFormats(
    const std::string& device_id,
    media::VideoCaptureFormats* supported_formats) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  DCHECK(supported_formats->empty());

  // Return all available formats of the device, regardless its started state.
  media::VideoCaptureDeviceInfo* existing_device = GetDeviceInfoById(device_id);
  if (existing_device)
    *supported_formats = existing_device->supported_formats;
  return true;
}

bool VideoCaptureManager::GetDeviceFormatsInUse(
    media::VideoCaptureSessionId capture_session_id,
    media::VideoCaptureFormats* formats_in_use) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  DCHECK(formats_in_use->empty());

  SessionMap::iterator it = sessions_.find(capture_session_id);
  if (it == sessions_.end())
    return false;
  std::ostringstream string_stream;
  string_stream << "GetDeviceFormatsInUse for device: " << it->second.name;
  EmitLogMessage(string_stream.str(), 1);

  base::Optional<media::VideoCaptureFormat> format =
      GetDeviceFormatInUse(it->second.type, it->second.id);
  if (format.has_value())
    formats_in_use->push_back(format.value());

  return true;
}

base::Optional<media::VideoCaptureFormat>
VideoCaptureManager::GetDeviceFormatInUse(MediaStreamType stream_type,
                                          const std::string& device_id) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  // Return the currently in-use format of the device, if it's started.
  VideoCaptureController* device_in_use =
      LookupControllerByMediaTypeAndDeviceId(stream_type, device_id);
  return device_in_use ? device_in_use->GetVideoCaptureFormat() : base::nullopt;
}

void VideoCaptureManager::SetDesktopCaptureWindowId(
    media::VideoCaptureSessionId session_id,
    gfx::NativeViewId window_id) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  VLOG(2) << "SetDesktopCaptureWindowId called for session " << session_id;

  notification_window_ids_[session_id] = window_id;
  MaybePostDesktopCaptureWindowId(session_id);
}

void VideoCaptureManager::MaybePostDesktopCaptureWindowId(
    media::VideoCaptureSessionId session_id) {
  SessionMap::iterator session_it = sessions_.find(session_id);
  if (session_it == sessions_.end())
    return;

  VideoCaptureController* const existing_device =
      LookupControllerByMediaTypeAndDeviceId(session_it->second.type,
                                             session_it->second.id);
  if (!existing_device) {
    LOG(ERROR) << "Failed to find an existing screen capture device.";
    return;
  }

  if (!existing_device->IsDeviceAlive()) {
    LOG(ERROR) << "Screen capture device not yet started.";
    return;
  }

  DCHECK_EQ(MEDIA_DESKTOP_VIDEO_CAPTURE, existing_device->stream_type());
  DesktopMediaID id = DesktopMediaID::Parse(existing_device->device_id());
  if (id.is_null())
    return;

  auto window_id_it = notification_window_ids_.find(session_id);
  if (window_id_it == notification_window_ids_.end()) {
    LOG(ERROR) << "Notification window id not set for screen capture.";
    return;
  }

  existing_device->SetDesktopCaptureWindowIdAsync(
      window_id_it->second,
      base::BindOnce([](scoped_refptr<VideoCaptureManager>) {},
                     scoped_refptr<VideoCaptureManager>(this)));
  notification_window_ids_.erase(window_id_it);
}

void VideoCaptureManager::GetPhotoState(
    int session_id,
    media::VideoCaptureDevice::GetPhotoStateCallback callback) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);

  const VideoCaptureController* controller =
      LookupControllerBySessionId(session_id);
  if (!controller)
    return;
  if (controller->IsDeviceAlive()) {
    controller->GetPhotoState(std::move(callback));
    return;
  }
  // Queue up a request for later.
  photo_request_queue_.emplace_back(
      session_id,
      base::Bind(&VideoCaptureController::GetPhotoState,
                 base::Unretained(controller), base::Passed(&callback)));
}

void VideoCaptureManager::SetPhotoOptions(
    int session_id,
    media::mojom::PhotoSettingsPtr settings,
    media::VideoCaptureDevice::SetPhotoOptionsCallback callback) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);

  VideoCaptureController* controller = LookupControllerBySessionId(session_id);
  if (!controller)
    return;
  if (controller->IsDeviceAlive()) {
    controller->SetPhotoOptions(std::move(settings), std::move(callback));
    return;
  }
  // Queue up a request for later.
  photo_request_queue_.emplace_back(
      session_id, base::Bind(&VideoCaptureController::SetPhotoOptions,
                             base::Unretained(controller),
                             base::Passed(&settings), base::Passed(&callback)));
}

void VideoCaptureManager::TakePhoto(
    int session_id,
    media::VideoCaptureDevice::TakePhotoCallback callback) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);

  VideoCaptureController* controller = LookupControllerBySessionId(session_id);
  if (!controller)
    return;
  if (controller->IsDeviceAlive()) {
    controller->TakePhoto(std::move(callback));
    return;
  }
  // Queue up a request for later.
  photo_request_queue_.emplace_back(
      session_id,
      base::Bind(&VideoCaptureController::TakePhoto,
                 base::Unretained(controller), base::Passed(&callback)));
}

void VideoCaptureManager::OnOpened(
    MediaStreamType stream_type,
    media::VideoCaptureSessionId capture_session_id) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  for (auto& listener : listeners_)
    listener.Opened(stream_type, capture_session_id);
}

void VideoCaptureManager::OnClosed(
    MediaStreamType stream_type,
    media::VideoCaptureSessionId capture_session_id) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  for (auto& listener : listeners_)
    listener.Closed(stream_type, capture_session_id);
}

void VideoCaptureManager::OnDeviceInfosReceived(
    base::ElapsedTimer* timer,
    const EnumerationCallback& client_callback,
    const std::vector<media::VideoCaptureDeviceInfo>& device_infos) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  UMA_HISTOGRAM_TIMES(
      "Media.VideoCaptureManager.GetAvailableDevicesInfoOnDeviceThreadTime",
      timer->Elapsed());
  devices_info_cache_ = device_infos;

  std::ostringstream string_stream;
  string_stream << "VideoCaptureManager::OnDeviceInfosReceived: Recevied "
                << device_infos.size() << " device infos.";
  for (const auto& entry : device_infos) {
    string_stream << std::endl
                  << "device_id: " << entry.descriptor.device_id
                  << ", display_name: " << entry.descriptor.display_name;
  }
  EmitLogMessage(string_stream.str(), 1);

  // Walk the |devices_info_cache_| and produce a
  // media::VideoCaptureDeviceDescriptors for |client_callback|.
  media::VideoCaptureDeviceDescriptors devices;
  std::vector<std::tuple<media::VideoCaptureDeviceDescriptor,
                         media::VideoCaptureFormats>>
      descriptors_and_formats;
  for (const auto& it : devices_info_cache_) {
    devices.emplace_back(it.descriptor);
    descriptors_and_formats.emplace_back(it.descriptor, it.supported_formats);
    MediaInternals::GetInstance()->UpdateVideoCaptureDeviceCapabilities(
        descriptors_and_formats);
  }

  client_callback.Run(devices);
}

void VideoCaptureManager::DestroyControllerIfNoClients(
    VideoCaptureController* controller) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  // Removal of the last client stops the device.
  if (!controller->HasActiveClient() && !controller->HasPausedClient()) {
    std::ostringstream string_stream;
    string_stream << "VideoCaptureManager stopping device (stream_type = "
                  << controller->stream_type()
                  << ", device_id = " << controller->device_id() << ")";
    EmitLogMessage(string_stream.str(), 1);

    // The VideoCaptureController is removed from |controllers_| immediately.
    // The controller is deleted immediately, and the device is freed
    // asynchronously. After this point, subsequent requests to open this same
    // device ID will create a new VideoCaptureController,
    // VideoCaptureController, and VideoCaptureDevice.
    DoStopDevice(controller);
    // TODO(mcasas): use a helper function https://crbug.com/624854.
    auto controller_iter = std::find_if(
        controllers_.begin(), controllers_.end(),
        [controller](
            const scoped_refptr<VideoCaptureController>& device_entry) {
          return device_entry.get() == controller;
        });
    controllers_.erase(controller_iter);
  }
}

VideoCaptureController* VideoCaptureManager::LookupControllerBySessionId(
    int session_id) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  SessionMap::const_iterator session_it = sessions_.find(session_id);
  if (session_it == sessions_.end())
    return nullptr;

  return LookupControllerByMediaTypeAndDeviceId(session_it->second.type,
                                                session_it->second.id);
}

VideoCaptureController*
VideoCaptureManager::LookupControllerByMediaTypeAndDeviceId(
    MediaStreamType type,
    const std::string& device_id) const {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);

  for (const auto& entry : controllers_) {
    if (type == entry->stream_type() && device_id == entry->device_id())
      return entry.get();
  }
  return nullptr;
}

bool VideoCaptureManager::IsControllerPointerValid(
    const VideoCaptureController* controller) const {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);
  return base::ContainsValue(controllers_, controller);
}

scoped_refptr<VideoCaptureController>
VideoCaptureManager::GetControllerSharedRef(
    VideoCaptureController* controller) const {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);

  for (const auto& entry : controllers_) {
    if (entry.get() == controller)
      return entry;
  }
  return nullptr;
}

media::VideoCaptureDeviceInfo* VideoCaptureManager::GetDeviceInfoById(
    const std::string& id) {
  for (auto& it : devices_info_cache_) {
    if (it.descriptor.device_id == id)
      return &it;
  }
  return nullptr;
}

VideoCaptureController* VideoCaptureManager::GetOrCreateController(
    media::VideoCaptureSessionId capture_session_id,
    const media::VideoCaptureParams& params) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);

  SessionMap::iterator session_it = sessions_.find(capture_session_id);
  if (session_it == sessions_.end())
    return nullptr;
  const MediaStreamDevice& device_info = session_it->second;

  // Check if another session has already opened this device. If so, just
  // use that opened device.
  VideoCaptureController* const existing_device =
      LookupControllerByMediaTypeAndDeviceId(device_info.type, device_info.id);
  if (existing_device) {
    DCHECK_EQ(device_info.type, existing_device->stream_type());
    return existing_device;
  }

  VideoCaptureController* new_controller = new VideoCaptureController(
      device_info.id, device_info.type, params,
      video_capture_provider_->CreateDeviceLauncher(), emit_log_message_cb_);
  controllers_.emplace_back(new_controller);
  return new_controller;
}

base::Optional<CameraCalibration> VideoCaptureManager::GetCameraCalibration(
    const std::string& device_id) {
  media::VideoCaptureDeviceInfo* info = GetDeviceInfoById(device_id);
  if (!info)
    return base::Optional<CameraCalibration>();
  return info->descriptor.camera_calibration;
}

#if defined(OS_ANDROID)
void VideoCaptureManager::OnApplicationStateChange(
    base::android::ApplicationState state) {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);

  // Only release/resume devices when the Application state changes from
  // RUNNING->STOPPED->RUNNING.
  if (state == base::android::APPLICATION_STATE_HAS_RUNNING_ACTIVITIES &&
      !application_state_has_running_activities_) {
    ResumeDevices();
    application_state_has_running_activities_ = true;
  } else if (state == base::android::APPLICATION_STATE_HAS_STOPPED_ACTIVITIES) {
    ReleaseDevices();
    application_state_has_running_activities_ = false;
  }
}

void VideoCaptureManager::ReleaseDevices() {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);

  for (auto& controller : controllers_) {
    // Do not stop Content Video Capture devices, e.g. Tab or Screen capture.
    if (controller->stream_type() != MEDIA_DEVICE_VIDEO_CAPTURE)
      continue;

    DoStopDevice(controller.get());
  }
}

void VideoCaptureManager::ResumeDevices() {
  DCHECK_CURRENTLY_ON(BrowserThread::IO);

  for (auto& controller : controllers_) {
    // Do not resume Content Video Capture devices, e.g. Tab or Screen capture.
    // Do not try to restart already running devices.
    if (controller->stream_type() != MEDIA_DEVICE_VIDEO_CAPTURE ||
        controller->IsDeviceAlive())
      continue;

    // Check if the device is already in the start queue.
    bool device_in_queue = false;
    for (auto& request : device_start_request_queue_) {
      if (request.controller() == controller.get()) {
        device_in_queue = true;
        break;
      }
    }

    if (!device_in_queue) {
      // Session ID is only valid for Screen capture. So we can fake it to
      // resume video capture devices here.
      QueueStartDevice(kFakeSessionId, controller.get(),
                       controller->parameters());
    }
  }
}
#endif  // defined(OS_ANDROID)

void VideoCaptureManager::EmitLogMessage(const std::string& message,
                                         int verbose_log_level) {
  LOG(ERROR) << message;
  emit_log_message_cb_.Run(message);
}

}  // namespace content
