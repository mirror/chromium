// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/inspector/LeakDetector.h"

#include "bindings/core/v8/V8GCController.h"
#include "core/editing/spellcheck/SpellChecker.h"
#include "core/frame/LocalFrame.h"
#include "core/workers/InProcessWorkerMessagingProxy.h"
#include "core/workers/WorkerThread.h"
#include "platform/InstanceCounters.h"
#include "platform/Timer.h"
#include "platform/bindings/V8PerIsolateData.h"
#include "platform/loader/fetch/MemoryCache.h"
#include "public/platform/Platform.h"

namespace blink {

namespace {
LeakDetector::AbstractAnimationWorkletThreadGCFunction
    g_abstract_animation_worklet_gc_function = nullptr;
}

LeakDetector::LeakDetector(LeakDetectorClient* client)
    : delayed_gc_timer_(
          Platform::Current()->CurrentThread()->GetWebTaskRunner(),
          this,
          &LeakDetector::DelayedGC),
      delayed_report_timer_(
          Platform::Current()->CurrentThread()->GetWebTaskRunner(),
          this,
          &LeakDetector::Report),
      number_of_gc_needed_(0),
      client_(client) {}

LeakDetector::~LeakDetector() {}

void LeakDetector::PrepareForLeakDetection(LocalFrame* local_frame) {
  v8::Isolate* isolate = v8::Isolate::GetCurrent();
  v8::HandleScope handle_scope(isolate);

  // For example, calling isValidEmailAddress in EmailInputType.cpp with a
  // non-empty string creates a static ScriptRegexp value which holds a
  // V8PerContextData indirectly. This affects the number of V8PerContextData.
  // To ensure that context data is created, call ensureScriptRegexpContext
  // here.
  V8PerIsolateData::From(isolate)->EnsureScriptRegexpContext();

  WorkerThread::TerminateAllWorkersForTesting();
  GetMemoryCache()->EvictResources();

  // If the spellchecker is allowed to continue issuing requests while the
  // leak detector runs, leaks may flakily be reported as the requests keep
  // their associated element (and document) alive.
  //
  // Stop the spellchecker to prevent this.
  if (local_frame)
    local_frame->GetSpellChecker().PrepareForLeakDetection();

  // FIXME: HTML5 Notification should be closed because notification affects the
  // result of number of DOM objects.
  V8PerIsolateData::From(isolate)->ClearScriptRegexpContext();
}

void LeakDetector::CollectGarbage() {
  for (int i = 0; i < 5; ++i)
    V8PerIsolateData::MainThreadIsolate()->LowMemoryNotification();

  ExecuteAbstractAnimationWorkletThreadGC();
  // Note: Oilpan precise GC is scheduled at the end of the event loop.

  // Task queue may contain delayed object destruction tasks.
  // This method is called from navigation hook inside FrameLoader,
  // so previous document is still held by the loader until the next event loop.
  // Complete all pending tasks before proceeding to gc.
  number_of_gc_needed_ = 2;
  delayed_gc_timer_.StartOneShot(0, BLINK_FROM_HERE);
}

void LeakDetector::DelayedGC(TimerBase*) {
  // We do a second and third GC here to address flakiness
  // The second GC is necessary as Resource GC may have postponed clean-up tasks
  // to next event loop.  The third GC is necessary for cleaning up Document
  // after worker object died.
  for (int i = 0; i < 5; ++i)
    V8PerIsolateData::MainThreadIsolate()->LowMemoryNotification();

  ExecuteAbstractAnimationWorkletThreadGC();
  // Note: Oilpan precise GC is scheduled at the end of the event loop.

  // Inspect counters on the next event loop.
  if (--number_of_gc_needed_ > 0) {
    delayed_gc_timer_.StartOneShot(0, BLINK_FROM_HERE);
  } else if (number_of_gc_needed_ > -1 &&
             InProcessWorkerMessagingProxy::ProxyCount()) {
    // It is possible that all posted tasks for finalizing in-process proxy
    // objects will not have run before the final round of GCs started. If so,
    // do yet another pass, letting these tasks run and then afterwards perform
    // a GC to tidy up.
    //
    // TODO(sof): use proxyCount() to always decide if another GC needs to be
    // scheduled.  Some debug bots running browser unit tests disagree
    // (crbug.com/616714)
    delayed_gc_timer_.StartOneShot(0, BLINK_FROM_HERE);
  } else {
    delayed_report_timer_.StartOneShot(0, BLINK_FROM_HERE);
  }
}

void LeakDetector::Report(TimerBase*) {
  DCHECK(client_);
  client_->OnLeakDetectionComplete();
}

// static
void LeakDetector::InitAbstractAnimationWorkletThreadGC(
    AbstractAnimationWorkletThreadGCFunction gc_function) {
  DCHECK(!g_abstract_animation_worklet_gc_function);
  g_abstract_animation_worklet_gc_function = gc_function;
}

void LeakDetector::ExecuteAbstractAnimationWorkletThreadGC() {
  DCHECK(g_abstract_animation_worklet_gc_function);
  return g_abstract_animation_worklet_gc_function();
}
}  // namespace blink
