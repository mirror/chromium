// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/css/FontFaceSetWorker.h"

#include "bindings/core/v8/Dictionary.h"
#include "bindings/core/v8/ScriptPromiseResolver.h"
#include "core/css/CSSSegmentedFontFace.h"
#include "core/css/FontFaceCache.h"
#include "core/css/FontFaceSetLoadEvent.h"
#include "core/css/OffscreenFontSelector.h"
#include "core/css/StylePropertySet.h"
#include "core/css/parser/CSSParser.h"
#include "core/css/resolver/FontStyleResolver.h"
#include "core/css/resolver/StyleResolver.h"
#include "core/frame/LocalFrame.h"
#include "core/frame/LocalFrameView.h"
#include "core/style/ComputedStyle.h"
#include "platform/Histogram.h"
#include "platform/bindings/ScriptState.h"

namespace blink {

static const int kDefaultFontSize = 10;
static const char kDefaultFontFamily[] = "sans-serif";

FontFaceSetWorker::FontFaceSetWorker(WorkerGlobalScope& worker)
    : Supplement<WorkerGlobalScope>(worker),
      SuspendableObject(&worker),
      should_fire_loading_event_(false),
      is_loading_(false),
      ready_(new ReadyProperty(GetExecutionContext(),
                               this,
                               ReadyProperty::kReady)),
      async_runner_(AsyncMethodRunner<FontFaceSetWorker>::Create(
          this,
          &FontFaceSetWorker::HandlePendingEventsAndPromises)) {
  SuspendIfNeeded();
}

FontFaceSetWorker::~FontFaceSetWorker() {}

WorkerGlobalScope* FontFaceSetWorker::GetWorker() const {
  return ToWorkerGlobalScope(GetExecutionContext());
}

void FontFaceSetWorker::AddFontFacesToFontFaceCache(
    FontFaceCache* font_face_cache) {
  for (const auto& font_face : non_css_connected_faces_)
    font_face_cache->AddFontFace(font_face, false);
}

ExecutionContext* FontFaceSetWorker::GetExecutionContext() const {
  return SuspendableObject::GetExecutionContext();
}

AtomicString FontFaceSetWorker::status() const {
  DEFINE_THREAD_SAFE_STATIC_LOCAL(AtomicString, loading, ("loading"));
  DEFINE_THREAD_SAFE_STATIC_LOCAL(AtomicString, loaded, ("loaded"));
  return is_loading_ ? loading : loaded;
}

void FontFaceSetWorker::HandlePendingEventsAndPromisesSoon() {
  // async_runner_ will be automatically stopped on destruction.
  async_runner_->RunAsync();
}

bool FontFaceSetWorker::ShouldSignalReady() const {
  if (!loading_fonts_.IsEmpty())
    return false;
  return is_loading_ || ready_->GetState() == ReadyProperty::kPending;
}

void FontFaceSetWorker::HandlePendingEventsAndPromises() {
  FireLoadingEvent();
  FireDoneEventIfPossible();
}

void FontFaceSetWorker::FireLoadingEvent() {
  if (should_fire_loading_event_) {
    should_fire_loading_event_ = false;
    DispatchEvent(
        FontFaceSetLoadEvent::CreateForFontFaces(EventTypeNames::loading));
  }
}

void FontFaceSetWorker::Suspend() {
  async_runner_->Suspend();
}

void FontFaceSetWorker::Resume() {
  async_runner_->Resume();
}

void FontFaceSetWorker::ContextDestroyed(ExecutionContext*) {
  async_runner_->Stop();
}

void FontFaceSetWorker::BeginFontLoading(FontFace* font_face) {
  histogram_.IncrementCount();
  AddToLoadingFonts(font_face);
}

void FontFaceSetWorker::NotifyLoaded(FontFace* font_face) {
  histogram_.UpdateStatus(font_face);
  loaded_fonts_.push_back(font_face);
  RemoveFromLoadingFonts(font_face);
}

void FontFaceSetWorker::NotifyError(FontFace* font_face) {
  histogram_.UpdateStatus(font_face);
  failed_fonts_.push_back(font_face);
  RemoveFromLoadingFonts(font_face);
}

void FontFaceSetWorker::AddToLoadingFonts(FontFace* font_face) {
  if (!is_loading_) {
    is_loading_ = true;
    should_fire_loading_event_ = true;
    if (ready_->GetState() != ReadyProperty::kPending)
      ready_->Reset();
    HandlePendingEventsAndPromisesSoon();
  }
  loading_fonts_.insert(font_face);
  font_face->AddCallback(this);
}

void FontFaceSetWorker::RemoveFromLoadingFonts(FontFace* font_face) {
  loading_fonts_.erase(font_face);
  if (loading_fonts_.IsEmpty())
    HandlePendingEventsAndPromisesSoon();
}

ScriptPromise FontFaceSetWorker::ready(ScriptState* script_state) {
  return ready_->Promise(script_state->World());
}

FontFaceSet* FontFaceSetWorker::addForBinding(ScriptState*,
                                              FontFace* font_face,
                                              ExceptionState&) {
  DCHECK(font_face);
  if (non_css_connected_faces_.Contains(font_face))
    return this;
  OffscreenFontSelector* font_selector = GetWorker()->GetFontSelector();
  non_css_connected_faces_.insert(font_face);
  font_selector->GetFontFaceCache()->AddFontFace(font_face, false);
  font_face->MaybeLoad();
  if (font_face->LoadStatus() == FontFace::kLoading)
    AddToLoadingFonts(font_face);
  font_selector->FontFaceInvalidated();
  return this;
}

void FontFaceSetWorker::clearForBinding(ScriptState*, ExceptionState&) {
  if (non_css_connected_faces_.IsEmpty())
    return;
  OffscreenFontSelector* font_selector = GetWorker()->GetFontSelector();
  FontFaceCache* font_face_cache = font_selector->GetFontFaceCache();
  for (const auto& font_face : non_css_connected_faces_) {
    font_face_cache->RemoveFontFace(font_face.Get(), false);
    if (font_face->LoadStatus() == FontFace::kLoading)
      RemoveFromLoadingFonts(font_face);
  }
  non_css_connected_faces_.clear();
  font_selector->FontFaceInvalidated();
}

bool FontFaceSetWorker::deleteForBinding(ScriptState*,
                                         FontFace* font_face,
                                         ExceptionState&) {
  DCHECK(font_face);
  HeapListHashSet<Member<FontFace>>::iterator it =
      non_css_connected_faces_.find(font_face);
  if (it != non_css_connected_faces_.end()) {
    non_css_connected_faces_.erase(it);
    OffscreenFontSelector* font_selector = GetWorker()->GetFontSelector();
    font_selector->GetFontFaceCache()->RemoveFontFace(font_face, false);
    if (font_face->LoadStatus() == FontFace::kLoading)
      RemoveFromLoadingFonts(font_face);
    font_selector->FontFaceInvalidated();
    return true;
  }
  return false;
}

bool FontFaceSetWorker::hasForBinding(ScriptState*,
                                      FontFace* font_face,
                                      ExceptionState&) const {
  DCHECK(font_face);
  return non_css_connected_faces_.Contains(font_face);
}

size_t FontFaceSetWorker::size() const {
  return non_css_connected_faces_.size();
}

void FontFaceSetWorker::FireDoneEventIfPossible() {
  if (should_fire_loading_event_)
    return;
  if (!ShouldSignalReady())
    return;

  if (is_loading_) {
    FontFaceSetLoadEvent* done_event = nullptr;
    FontFaceSetLoadEvent* error_event = nullptr;
    done_event = FontFaceSetLoadEvent::CreateForFontFaces(
        EventTypeNames::loadingdone, loaded_fonts_);
    loaded_fonts_.clear();
    if (!failed_fonts_.IsEmpty()) {
      error_event = FontFaceSetLoadEvent::CreateForFontFaces(
          EventTypeNames::loadingerror, failed_fonts_);
      failed_fonts_.clear();
    }
    is_loading_ = false;
    DispatchEvent(done_event);
    if (error_event) {
      DispatchEvent(error_event);
      if (ready_->GetState() == ReadyProperty::kPending) {
        ready_->Reject(
            DOMException::Create(kSyntaxError, "Error loading font"));
      }
    }
  }

  if (ready_->GetState() == ReadyProperty::kPending)
    ready_->Resolve(this);
}

ScriptPromise FontFaceSetWorker::load(ScriptState* script_state,
                                      const String& font_string,
                                      const String& text) {
  Font font;
  if (!ResolveFontStyle(font_string, font)) {
    ScriptPromiseResolver* resolver =
        ScriptPromiseResolver::Create(script_state);
    ScriptPromise promise = resolver->Promise();
    resolver->Reject(DOMException::Create(
        kSyntaxError, "Could not resolve '" + font_string + "' as a font."));
    return promise;
  }

  FontFaceCache* font_face_cache =
      GetWorker()->GetFontSelector()->GetFontFaceCache();
  FontFaceArray faces;
  for (const FontFamily* f = &font.GetFontDescription().Family(); f;
       f = f->Next()) {
    CSSSegmentedFontFace* segmented_font_face =
        font_face_cache->Get(font.GetFontDescription(), f->Family());
    if (segmented_font_face)
      segmented_font_face->Match(text, faces);
  }

  LoadFontPromiseResolver* resolver =
      LoadFontPromiseResolver::Create(faces, script_state);
  ScriptPromise promise = resolver->Promise();
  // After this, resolver->promise() may return null.
  resolver->LoadFonts();
  return promise;
}

bool FontFaceSetWorker::check(const String& font_string,
                              const String& text,
                              ExceptionState& exception_state) {
  Font font;
  if (!ResolveFontStyle(font_string, font)) {
    exception_state.ThrowDOMException(
        kSyntaxError, "Could not resolve '" + font_string + "' as a font.");
    return false;
  }

  OffscreenFontSelector* font_selector = GetWorker()->GetFontSelector();
  FontFaceCache* font_face_cache = font_selector->GetFontFaceCache();

  bool has_loaded_faces = false;
  for (const FontFamily* f = &font.GetFontDescription().Family(); f;
       f = f->Next()) {
    CSSSegmentedFontFace* face =
        font_face_cache->Get(font.GetFontDescription(), f->Family());
    if (face) {
      if (!face->CheckFont(text))
        return false;
      has_loaded_faces = true;
    }
  }
  if (has_loaded_faces)
    return true;
  for (const FontFamily* f = &font.GetFontDescription().Family(); f;
       f = f->Next()) {
    if (font_selector->IsPlatformFamilyMatchAvailable(font.GetFontDescription(),
                                                      f->Family()))
      return true;
  }
  return false;
}

bool FontFaceSetWorker::ResolveFontStyle(const String& font_string,
                                         Font& font) {
  if (font_string.IsEmpty())
    return false;

  // Interpret fontString in the same way as the 'font' attribute of
  // CanvasRenderingContext2D.
  MutableStylePropertySet* parsed_style =
      MutableStylePropertySet::Create(kHTMLStandardMode);
  CSSParser::ParseValue(parsed_style, CSSPropertyFont, font_string, true);
  if (parsed_style->IsEmpty())
    return false;

  String font_value = parsed_style->GetPropertyValue(CSSPropertyFont);
  if (font_value == "inherit" || font_value == "initial")
    return false;

  FontFamily font_family;
  font_family.SetFamily(kDefaultFontFamily);

  FontDescription default_font_description;
  default_font_description.SetFamily(font_family);
  default_font_description.SetSpecifiedSize(kDefaultFontSize);
  default_font_description.SetComputedSize(kDefaultFontSize);

  FontDescription description = FontStyleResolver::ComputeFont(
      *parsed_style, GetWorker()->GetFontSelector());

  font = Font(description);
  font.Update(GetWorker()->GetFontSelector());

  return true;
}

void FontFaceSetWorker::FontLoadHistogram::UpdateStatus(FontFace* font_face) {
  if (status_ == kReported)
    return;
  if (font_face->HadBlankText())
    status_ = kHadBlankText;
  else if (status_ == kNoWebFonts)
    status_ = kDidNotHaveBlankText;
}

void FontFaceSetWorker::FontLoadHistogram::Record() {
  if (!recorded_) {
    recorded_ = true;
    DEFINE_STATIC_LOCAL(CustomCountHistogram, web_fonts_in_page_histogram,
                        ("WebFont.WebFontsInPage", 1, 100, 50));
    web_fonts_in_page_histogram.Count(count_);
  }
  if (status_ == kHadBlankText || status_ == kDidNotHaveBlankText) {
    DEFINE_STATIC_LOCAL(EnumerationHistogram, had_blank_text_histogram,
                        ("WebFont.HadBlankText", 2));
    had_blank_text_histogram.Count(status_ == kHadBlankText ? 1 : 0);
    status_ = kReported;
  }
}

FontFaceSetWorker* FontFaceSetWorker::From(WorkerGlobalScope& worker) {
  FontFaceSetWorker* fonts = static_cast<FontFaceSetWorker*>(
      Supplement<WorkerGlobalScope>::From(worker, SupplementName()));
  if (!fonts) {
    fonts = FontFaceSetWorker::Create(worker);
    Supplement<WorkerGlobalScope>::ProvideTo(worker, SupplementName(), fonts);
  }

  return fonts;
}

FontFaceSetIterable::IterationSource* FontFaceSetWorker::StartIteration(
    ScriptState*,
    ExceptionState&) {
  // Setlike should iterate each item in insertion order, and items should
  // be keep on up to date. But since blink does not have a way to hook up CSS
  // modification, take a snapshot here, and make it ordered as follows.
  HeapVector<Member<FontFace>> font_faces;
  font_faces.ReserveInitialCapacity(non_css_connected_faces_.size());
  for (const auto& font_face : non_css_connected_faces_)
    font_faces.push_back(font_face);
  return new IterationSource(font_faces);
}

DEFINE_TRACE(FontFaceSetWorker) {
  visitor->Trace(ready_);
  visitor->Trace(loading_fonts_);
  visitor->Trace(loaded_fonts_);
  visitor->Trace(failed_fonts_);
  visitor->Trace(non_css_connected_faces_);
  visitor->Trace(async_runner_);
  EventTargetWithInlineData::Trace(visitor);
  Supplement<WorkerGlobalScope>::Trace(visitor);
  SuspendableObject::Trace(visitor);
  FontFace::LoadFontCallback::Trace(visitor);
  FontFaceSet::Trace(visitor);
}

}  // namespace blink
