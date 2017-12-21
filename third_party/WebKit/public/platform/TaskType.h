// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef TaskType_h
#define TaskType_h

namespace blink {

// A list of task sources known to Blink according to the spec.
// This enum is used for a histogram and it should not be re-numbered.
enum class TaskType : unsigned {
  ///////////////////////////////////////
  // Speced tasks should use one of the following task types
  ///////////////////////////////////////

  // Speced tasks and related internal tasks should be posted to one of
  // the following task runners. These task runners may be throttled.

  // 0 is reserved to represent that TaskType is not specified.

  // https://html.spec.whatwg.org/multipage/webappapis.html#generic-task-sources
  //
  // This task source is used for features that react to DOM manipulations, such
  // as things that happen in a non-blocking fashion when an element is inserted
  // into the document.
  kDOMManipulation = 1,
  // This task source is used for features that react to user interaction, for
  // example keyboard or mouse input. Events sent in response to user input
  // (e.g. click events) must be fired using tasks queued with the user
  // interaction task source.
  kUserInteraction = 2,
  // This task source is used for features that trigger in response to network
  // activity.
  kNetworking = 3,
  // This task source is used for control messages between kNetworking tasks.
  kNetworkingControl = 4,
  // This task source is used to queue calls to history.back() and similar APIs.
  kHistoryTraversal = 5,

  // https://html.spec.whatwg.org/multipage/embedded-content.html#the-embed-element
  // This task source is used for the embed element setup steps.
  kEmbed = 6,

  // https://html.spec.whatwg.org/multipage/embedded-content.html#media-elements
  // This task source is used for all tasks queued in the [Media elements]
  // section and subsections of the spec unless explicitly specified otherwise.
  kMediaElementEvent = 7,

  // https://html.spec.whatwg.org/multipage/scripting.html#the-canvas-element
  // This task source is used to invoke the result callback of
  // HTMLCanvasElement.toBlob().
  kCanvasBlobSerialization = 8,

  // https://html.spec.whatwg.org/multipage/webappapis.html#event-loop-processing-model
  // This task source is used when an algorithm requires a microtask to be
  // queued.
  kMicrotask = 9,

  // https://html.spec.whatwg.org/multipage/webappapis.html#timers
  // This task source is used to queue tasks queued by setInterval() and similar
  // APIs.
  kJavascriptTimer = 10,

  // https://html.spec.whatwg.org/multipage/comms.html#sse-processing-model
  // This task source is used for any tasks that are queued by EventSource
  // objects.
  kRemoteEvent = 11,

  // https://html.spec.whatwg.org/multipage/comms.html#feedback-from-the-protocol
  // The task source for all tasks queued in the [WebSocket] section of the
  // spec.
  kWebSocket = 12,

  // https://html.spec.whatwg.org/multipage/comms.html#web-messaging
  // This task source is used for the tasks in cross-document messaging.
  kPostedMessage = 13,

  // https://html.spec.whatwg.org/multipage/comms.html#message-ports
  kUnshippedPortMessage = 14,

  // https://www.w3.org/TR/FileAPI/#blobreader-task-source
  // This task source is used for all tasks queued in the FileAPI spec to read
  // byte sequences associated with Blob and File objects.
  kFileReading = 15,

  // https://www.w3.org/TR/IndexedDB/#request-api
  kDatabaseAccess = 16,

  // https://w3c.github.io/presentation-api/#common-idioms
  // This task source is used for all tasks in the Presentation API spec.
  kPresentation = 17,

  // https://www.w3.org/TR/2016/WD-generic-sensor-20160830/#sensor-task-source
  // This task source is used for all tasks in the Sensor API spec.
  kSensor = 18,

  // https://w3c.github.io/performance-timeline/#performance-timeline
  kPerformanceTimeline = 19,

  // https://www.khronos.org/registry/webgl/specs/latest/1.0/#5.15
  // This task source is used for all tasks in the WebGL spec.
  kWebGL = 20,

  // https://www.w3.org/TR/requestidlecallback/#start-an-event-loop-s-idle-period
  kIdleTask = 21,

  // Use MiscPlatformAPI for a task that is defined in the spec but is not yet
  // associated with any specific task runner in the spec. MiscPlatformAPI is
  // not encouraged for stable and matured APIs. The spec should define the task
  // runner explicitly.
  // The task runner may be throttled.
  kMiscPlatformAPI = 22,

  ///////////////////////////////////////
  // The following task types are DEPRECATED! Use kInternal* instead.
  ///////////////////////////////////////

  // Other internal tasks that cannot fit any of the above task runners
  // can be posted here, but the usage is not encouraged. The task runner
  // may be throttled.
  //
  // UnspecedLoading type should be used for all tasks associated with
  // loading page content, UnspecedTimer should be used for all other purposes.
  kUnspecedTimer = 23,
  kUnspecedLoading = 24,

  // Tasks that must not be throttled should be posted here, but the usage
  // should be very limited.
  kUnthrottled = 25,

  ///////////////////////////////////////
  // Not-speced tasks should use one of the following task types
  ///////////////////////////////////////

  // Tasks for tests or mock objects.
  kInternalTest = 26,

  // Tasks that are related to blob storage .
  // Tasks with this task type are typically posted at:
  // //content/renderer/blob_storage
  kInternalBlobStorage = 27,

  // Tasks that are related to dev tool e.g. dispatching protocol messages.
  // Tasks with this task type are typically
  // posted at:
  // * //content/renderer/devtools
  kInternalDevTools = 28,

  // Tasks that are related to GPU like painting asynchronously. Task with this
  // task types are typically posted at:
  // * //components/viz/common/gpu
  // * //content/renderer/gpu/render_widget_compositor.cc
  // * //gpu
  // * //services/ui/public/cpp/gpu
  kInternalGPU = 29,

  // Used for tasks posted at RenderThreadImpl::ScheduleIdleHandler
  // (the task runner is |RenderThreadImpl::idle_timer_|).
  kInternalIdle = 30,

  // Tasks that are related to IndexedDB like calling callbacks on some events
  // of IndexDB. Tasks with this task types are typically posted at:
  // * //content/renderer/indexed_db
  kInternalIndexedDB = 31,

  // Tasks that are related to IPC or mojo like dispatching received data. Tasks
  // with this task type are typically posted at:
  // * //content/renderer/child_message_filter.cc
  // * //content/renderer/mojo
  // * //ipc
  // * //mojo
  // Note that there are other cases to use this task type e.g. //gpu/ipc.
  kInternalIPC = 32,

  // This task type should be used for implementation details of loading
  // process, for example notifying some component that loading has been
  // finished. This task type should not run any javascript, use kNetworking or
  // another appropriate task source in this case. Tasks with this task type are
  // typically posted at:
  // * //content/renderer/loader
  // * //WebKit/Source/core/loader
  kInternalLoading = 33,

  // Tasks that are related to media e.g. sending a frame to a decoder. Tasks
  // with this task type are typically posted at:
  // * //content/renderer/media
  // * //media
  kInternalMedia = 34,

  // Tasks that are related to plugin, pepper or PPAPI and posted at
  // * //components/plugins/renderer
  // * //content/renderer/pepper
  // * //ppapi
  kInternalPlugin = 35,

  // Tasks that are used for asynchronous closing of a RenderWidget.
  kInternalRenderWidget = 36,

  // Tasks that are related to ServiceManager e.g. InterfaceBinder's callbacks
  // or calling a callback on connection lost.
  // * //content/common/service_manager/
  // * //services/service_manager
  kInternalServiceManager = 37,

  // Tasks that are related to trace event e.g. flushing or dispatching trace
  // events. Tasks with this type are typically posted at:
  kInternalTracing = 38,

  // Tasks that are related to user interaction e.g. handling mojo events
  // related to input. Tasks with this task type are typically posted at:
  // * //content/renderer/input/
  kInternalUserInteraction = 39,

  // Tasks that are related to WebCrypto e.g. handling callbacks of WebCrypto
  // functions. Tasks with this type are typically posted at:
  // * //components/webcrypto
  kInternalWebCrypto = 40,

  // Tasks that are related to service workers e.g. initializing worker clients
  // asynchronously. Tasks with this task type are typically posted at:
  // * //content/renderer/service_worker
  kInternalWorker = 41,

  kCount = 42,
};

}  // namespace blink

#endif
