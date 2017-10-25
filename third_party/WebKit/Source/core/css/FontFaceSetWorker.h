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

namespace blink {

class ExceptionState;
class Font;
class FontFaceCache;

class CORE_EXPORT FontFaceSetWorker final
    : public FontFaceSet,
      public Supplement<WorkerGlobalScope>,
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

  WorkerGlobalScope* GetWorker() const;

  // FontFace::LoadFontCallback
  void NotifyLoaded(FontFace*) override;
  void NotifyError(FontFace*) override;

  void BeginFontLoading(FontFace*);

  static FontFaceSetWorker* From(WorkerGlobalScope&);

  static const char* SupplementName() { return "FontFaceSetWorker"; }

  void AddFontFacesToFontFaceCache(FontFaceCache*);

  void Trace(Visitor*) override;

 private:
  FontFaceSetIterable::IterationSource* StartIteration(
      ScriptState*,
      ExceptionState&) override;

  static FontFaceSetWorker* Create(WorkerGlobalScope& worker) {
    return new FontFaceSetWorker(worker);
  }

  explicit FontFaceSetWorker(WorkerGlobalScope&);

  void AddToLoadingFonts(FontFace*);
  void RemoveFromLoadingFonts(FontFace*);
  void FireDoneEventIfPossible() override;
  bool ResolveFontStyle(const String&, Font&);
  bool ShouldSignalReady() const;

  using ReadyProperty = ScriptPromiseProperty<Member<FontFaceSetWorker>,
                                              Member<FontFaceSetWorker>,
                                              Member<DOMException>>;

  HeapHashSet<Member<FontFace>> loading_fonts_;
  bool is_loading_;
  Member<ReadyProperty> ready_;
  FontFaceArray loaded_fonts_;
  FontFaceArray failed_fonts_;
  HeapListHashSet<Member<FontFace>> non_css_connected_faces_;
};

}  // namespace blink

#endif  // FontFaceSetWorker_h
