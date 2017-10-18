// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef FontFaceSetWorker_h
#define FontFaceSetWorker_h

#include "bindings/core/v8/Iterable.h"
#include "bindings/core/v8/ScriptPromise.h"
#include "core/css/FontFace.h"
#include "core/css/FontFaceSet.h"
#include "core/dom/SuspendableObject.h"
#include "core/workers/WorkerGlobalScope.h"
#include "platform/AsyncMethodRunner.h"
#include "platform/heap/Handle.h"
#include "platform/wtf/Allocator.h"
#include "platform/wtf/Vector.h"

// Mac OS X 10.6 SDK defines check() macro that interferes with our check()
// method
#ifdef check
#undef check
#endif

namespace blink {

class ExceptionState;
class Font;
class FontFaceCache;
class ExecutionContext;

class CORE_EXPORT FontFaceSetWorker final
    : public FontFaceSet,
      public Supplement<WorkerGlobalScope>,
      public SuspendableObject,
      public FontFace::LoadFontCallback {
  USING_GARBAGE_COLLECTED_MIXIN(FontFaceSetWorker);
  WTF_MAKE_NONCOPYABLE(FontFaceSetWorker);

 public:
  ~FontFaceSetWorker() override;

  bool check(const String& font, const String& text, ExceptionState&) override;
  ScriptPromise load(ScriptState*,
                     const String& font,
                     const String& text) override;
  ScriptPromise ready(ScriptState*) override;

  FontFaceSet* addForBinding(ScriptState*, FontFace*, ExceptionState&) override;
  void clearForBinding(ScriptState*, ExceptionState&) override;
  bool deleteForBinding(ScriptState*, FontFace*, ExceptionState&) override;
  bool hasForBinding(ScriptState*, FontFace*, ExceptionState&) const override;

  size_t size() const override;
  AtomicString status() const override;

  ExecutionContext* GetExecutionContext() const override;

  WorkerGlobalScope* GetWorker() const;

  // FontFace::LoadFontCallback
  void NotifyLoaded(FontFace*) override;
  void NotifyError(FontFace*) override;

  void BeginFontLoading(FontFace*);

  // SuspendableObject
  void Suspend() override;
  void Resume() override;
  void ContextDestroyed(ExecutionContext*) override;

  static FontFaceSetWorker* From(WorkerGlobalScope&);
  static void DidLayout(Document&);
  static size_t ApproximateBlankCharacterCount(Document&);

  static const char* SupplementName() { return "FontFaceSetWorker"; }

  void AddFontFacesToFontFaceCache(FontFaceCache*);

  DECLARE_VIRTUAL_TRACE();

 private:
  FontFaceSetIterable::IterationSource* StartIteration(
      ScriptState*,
      ExceptionState&) override;

  static FontFaceSetWorker* Create(WorkerGlobalScope& worker) {
    return new FontFaceSetWorker(worker);
  }

  class FontLoadHistogram {
    DISALLOW_NEW();

   public:
    enum Status { kNoWebFonts, kHadBlankText, kDidNotHaveBlankText, kReported };
    FontLoadHistogram() : status_(kNoWebFonts), count_(0), recorded_(false) {}
    void IncrementCount() { count_++; }
    void UpdateStatus(FontFace*);
    void Record();

   private:
    Status status_;
    int count_;
    bool recorded_;
  };

  explicit FontFaceSetWorker(WorkerGlobalScope&);

  void AddToLoadingFonts(FontFace*);
  void RemoveFromLoadingFonts(FontFace*);
  void FireLoadingEvent();
  void FireDoneEventIfPossible();
  bool ResolveFontStyle(const String&, Font&);
  void HandlePendingEventsAndPromisesSoon();
  void HandlePendingEventsAndPromises();
  bool ShouldSignalReady() const;

  using ReadyProperty = ScriptPromiseProperty<Member<FontFaceSetWorker>,
                                              Member<FontFaceSetWorker>,
                                              Member<DOMException>>;

  HeapHashSet<Member<FontFace>> loading_fonts_;
  bool should_fire_loading_event_;
  bool is_loading_;
  Member<ReadyProperty> ready_;
  FontFaceArray loaded_fonts_;
  FontFaceArray failed_fonts_;
  HeapListHashSet<Member<FontFace>> non_css_connected_faces_;

  Member<AsyncMethodRunner<FontFaceSetWorker>> async_runner_;

  FontLoadHistogram histogram_;
};

}  // namespace blink

#endif  // FontFaceSetWorker_h
