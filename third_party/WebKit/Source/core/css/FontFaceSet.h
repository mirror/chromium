// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef FontFaceSet_h
#define FontFaceSet_h

#include "bindings/core/v8/Iterable.h"
#include "bindings/core/v8/ScriptPromise.h"
#include "bindings/core/v8/ScriptPromiseResolver.h"
#include "core/css/FontFace.h"
#include "core/dom/SuspendableObject.h"
#include "core/dom/events/EventListener.h"
#include "core/dom/events/EventTarget.h"
#include "platform/AsyncMethodRunner.h"
#include "platform/bindings/ScriptWrappable.h"

// Mac OS X 10.6 SDK defines check() macro that interferes with our check()
// method
#ifdef check
#undef check
#endif

namespace blink {

using FontFaceSetIterable = SetlikeIterable<Member<FontFace>>;

class CORE_EXPORT FontFaceSet : public EventTargetWithInlineData,
                                public SuspendableObject,
                                public FontFaceSetIterable {
  DEFINE_WRAPPERTYPEINFO();
  WTF_MAKE_NONCOPYABLE(FontFaceSet);

 public:
  FontFaceSet(ExecutionContext& context)
      : SuspendableObject(&context),
        should_fire_loading_event_(false),
        async_runner_(AsyncMethodRunner<FontFaceSet>::Create(
            this,
            &FontFaceSet::HandlePendingEventsAndPromises)) {}
  ~FontFaceSet() {}

  DEFINE_ATTRIBUTE_EVENT_LISTENER(loading);
  DEFINE_ATTRIBUTE_EVENT_LISTENER(loadingdone);
  DEFINE_ATTRIBUTE_EVENT_LISTENER(loadingerror);

  virtual bool check(const String& font,
                     const String& text,
                     ExceptionState&) = 0;
  virtual ScriptPromise load(ScriptState*,
                             const String& font,
                             const String& text) = 0;
  virtual ScriptPromise ready(ScriptState*) = 0;

  virtual ExecutionContext* GetExecutionContext() const {
    return SuspendableObject::GetExecutionContext();
  }

  const AtomicString& InterfaceName() const {
    return EventTargetNames::FontFaceSet;
  }

  virtual FontFaceSet* addForBinding(ScriptState*,
                                     FontFace*,
                                     ExceptionState&) = 0;
  virtual void clearForBinding(ScriptState*, ExceptionState&) = 0;
  virtual bool deleteForBinding(ScriptState*, FontFace*, ExceptionState&) = 0;
  virtual bool hasForBinding(ScriptState*,
                             FontFace*,
                             ExceptionState&) const = 0;

  // SuspendableObject
  void Suspend() override;
  void Resume() override;
  void ContextDestroyed(ExecutionContext*) override;

  virtual size_t size() const = 0;
  virtual AtomicString status() const = 0;

  virtual void Trace(blink::Visitor*);

 protected:
  // Iterable overrides.
  virtual FontFaceSetIterable::IterationSource* StartIteration(
      ScriptState*,
      ExceptionState&) = 0;

  virtual void FireDoneEventIfPossible() = 0;

  void HandlePendingEventsAndPromisesSoon();

  bool should_fire_loading_event_;

  Member<AsyncMethodRunner<FontFaceSet>> async_runner_;

  class IterationSource final : public FontFaceSetIterable::IterationSource {
   public:
    explicit IterationSource(const HeapVector<Member<FontFace>>& font_faces)
        : index_(0), font_faces_(font_faces) {}
    bool Next(ScriptState*,
              Member<FontFace>&,
              Member<FontFace>&,
              ExceptionState&) override;

    virtual void Trace(blink::Visitor* visitor) {
      visitor->Trace(font_faces_);
      FontFaceSetIterable::IterationSource::Trace(visitor);
    }

   private:
    size_t index_;
    HeapVector<Member<FontFace>> font_faces_;
  };

  class LoadFontPromiseResolver final
      : public GarbageCollectedFinalized<LoadFontPromiseResolver>,
        public FontFace::LoadFontCallback {
    USING_GARBAGE_COLLECTED_MIXIN(LoadFontPromiseResolver);

   public:
    static LoadFontPromiseResolver* Create(FontFaceArray faces,
                                           ScriptState* script_state) {
      return new LoadFontPromiseResolver(faces, script_state);
    }

    void LoadFonts();
    ScriptPromise Promise() { return resolver_->Promise(); }

    void NotifyLoaded(FontFace*) override;
    void NotifyError(FontFace*) override;

    void Trace(blink::Visitor*);

   private:
    LoadFontPromiseResolver(FontFaceArray faces, ScriptState* script_state)
        : num_loading_(faces.size()),
          error_occured_(false),
          resolver_(ScriptPromiseResolver::Create(script_state)) {
      font_faces_.swap(faces);
    }

    HeapVector<Member<FontFace>> font_faces_;
    int num_loading_;
    bool error_occured_;
    Member<ScriptPromiseResolver> resolver_;
  };

 private:
  void HandlePendingEventsAndPromises();
  void FireLoadingEvent();
};

}  // namespace blink

#endif  // FontFaceSet_h
