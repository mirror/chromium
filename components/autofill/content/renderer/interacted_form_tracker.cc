// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/autofill/content/renderer/interacted_form_tracker.h"

#include "components/autofill/content/renderer/form_autofill_util.h"
#include "content/public/renderer/document_state.h"
#include "content/public/renderer/navigation_state.h"
#include "content/public/renderer/render_frame.h"
#include "third_party/WebKit/public/web/WebInputElement.h"
#include "third_party/WebKit/public/web/WebLocalFrame.h"
#include "third_party/WebKit/public/web/WebUserGestureIndicator.h"
#include "third_party/WebKit/public/web/modules/password_manager/WebFormElementObserver.h"
#include "third_party/WebKit/public/web/modules/password_manager/WebFormElementObserverCallback.h"
#include "ui/base/page_transition_types.h"

using blink::WebDocumentLoader;
using blink::WebInputElement;
using blink::WebFormControlElement;
using blink::WebFormElement;

namespace autofill {

namespace {

bool IsUserGesture() {
  return blink::WebUserGestureIndicator::IsProcessingUserGesture();
}

}  // namespace

class InteractedFormTracker::FormElementObserverCallback
    : public blink::WebFormElementObserverCallback {
 public:
  explicit FormElementObserverCallback(InteractedFormTracker* tracker)
      : tracker_(tracker) {}
  ~FormElementObserverCallback() override = default;

  void ElementWasHiddenOrRemoved() override {
    tracker_->FireSuccessfulFormSubmission(
        Observer::SubmissionSource::DOM_MUTATION_AFTER_XHR);
  }

 private:
  InteractedFormTracker* tracker_;

  DISALLOW_COPY_AND_ASSIGN(FormElementObserverCallback);
};

InteractedFormTracker::InteractedFormTracker(content::RenderFrame* render_frame)
    : content::RenderFrameObserver(render_frame),
      ignore_text_changes_(false),
      user_gesture_required_(true),
      form_element_observer_(nullptr),
      weak_ptr_factory_(this) {}

InteractedFormTracker::~InteractedFormTracker() {
  ResetLastInteractedElements();
}

void InteractedFormTracker::AddObserver(Observer* observer) {
  DCHECK(observer);
  observers_.AddObserver(observer);
}

void InteractedFormTracker::RemoveObserver(Observer* observer) {
  DCHECK(observer);
  observers_.RemoveObserver(observer);
}

void InteractedFormTracker::AjaxSucceeded() {
  OnSameDocumentNavigationCompleted(Observer::SubmissionSource::XHR_SUCCEEDED);
}

void InteractedFormTracker::TextFieldDidChange(
    const WebFormControlElement& element) {
  DCHECK(ToWebInputElement(&element) || form_util::IsTextAreaElement(element));

  if (ignore_text_changes_)
    return;

  // Disregard text changes that aren't caused by user gestures or pastes. Note
  // that pastes aren't necessarily user gestures because Blink's conception of
  // user gestures is centered around creating new windows/tabs.
  if (user_gesture_required_ && !IsUserGesture() &&
      !render_frame()->IsPasting())
    return;

  // We post a task for doing the Autofill as the caret position is not set
  // properly at this point (http://bugs.webkit.org/show_bug.cgi?id=16976) and
  // it is needed to trigger autofill.
  weak_ptr_factory_.InvalidateWeakPtrs();
  base::ThreadTaskRunnerHandle::Get()->PostTask(
      FROM_HERE, base::Bind(&InteractedFormTracker::TextFieldDidChangeImpl,
                            weak_ptr_factory_.GetWeakPtr(), element));
}

void InteractedFormTracker::TextFieldDidChangeImpl(
    const WebFormControlElement& element) {
  // If the element isn't focused then the changes don't matter. This check is
  // required to properly handle IME interactions.
  if (!element.Focused())
    return;

  const WebInputElement* input_element = ToWebInputElement(&element);
  if (!input_element)
    return;

  if (element.Form().IsNull()) {
    last_interacted_formless_element_ = *input_element;
  } else {
    last_interacted_form_ = element.Form();
  }

  for (auto& observer : observers_)
    observer.OnProvisionallySaveForm(
        element.Form(), *input_element,
        Observer::ElementChangeSource::TEXTFIELD_CHANGED);
}

void InteractedFormTracker::DidCommitProvisionalLoad(
    bool is_new_navigation,
    bool is_same_document_navigation) {
  if (!is_same_document_navigation)
    return;

  OnSameDocumentNavigationCompleted(
      Observer::SubmissionSource::SAME_DOCUMENT_NAVIGATION);
}

void InteractedFormTracker::DidStartProvisionalLoad(
    WebDocumentLoader* document_loader) {
  blink::WebLocalFrame* navigated_frame = render_frame()->GetWebFrame();
  if (navigated_frame->Parent()) {
    return;
  }

  // Bug fix for crbug.com/368690. isProcessingUserGesture() is false when
  // the user is performing actions outside the page (e.g. typed url,
  // history navigation). We don't want to trigger saving in these cases.
  content::DocumentState* document_state =
      content::DocumentState::FromDocumentLoader(document_loader);
  content::NavigationState* navigation_state =
      document_state->navigation_state();
  ui::PageTransition type = navigation_state->GetTransitionType();
  if (ui::PageTransitionIsWebTriggerable(type) &&
      ui::PageTransitionIsNewNavigation(type) &&
      !blink::WebUserGestureIndicator::IsProcessingUserGesture(
          navigated_frame)) {
    FireFormSubmitted(blink::WebFormElement());
  }
}

void InteractedFormTracker::FrameDetached() {
  FireSuccessfulFormSubmission(Observer::SubmissionSource::FRAME_DETACHED);
}

void InteractedFormTracker::WillSendSubmitEvent(const WebFormElement& form) {
  last_interacted_form_ = form;
  for (auto& observer : observers_) {
    observer.OnProvisionallySaveForm(
        form, blink::WebInputElement(),
        Observer::ElementChangeSource::WILL_SEND_SUBMIT_EVENT);
  }
}

void InteractedFormTracker::WillSubmitForm(const WebFormElement& form) {
  FireFormSubmitted(form);
}

void InteractedFormTracker::OnDestruct() {
  ResetLastInteractedElements();
}

void InteractedFormTracker::FireFormSubmitted(
    const blink::WebFormElement& form) {
  for (auto& observer : observers_)
    observer.OnFormSubmitted(form);
  ResetLastInteractedElements();
}

void InteractedFormTracker::FireSuccessfulFormSubmission(
    Observer::SubmissionSource source) {
  for (auto& observer : observers_)
    observer.OnSuccessfulSubmission(source);
  ResetLastInteractedElements();
}

void InteractedFormTracker::OnSameDocumentNavigationCompleted(
    Observer::SubmissionSource source) {
  if (CanInferFormSubmitted()) {
    FireSuccessfulFormSubmission(source);
    return;
  }
  TrackElement();
}

bool InteractedFormTracker::CanInferFormSubmitted() {
  // If last interacted form is available, assume form submission only if the
  // form is now gone, either invisible or removed from the DOM.
  // Otherwise (i.e., there is no form tag), we check if the last element the
  // user has interacted with are gone, to decide if submission has occurred.
  if (!last_interacted_form_.IsNull()) {
    return !form_util::AreFormContentsVisible(last_interacted_form_);
  } else if (!last_interacted_formless_element_.IsNull()) {
    return !form_util::IsWebElementVisible(last_interacted_formless_element_);
  }
  return false;
}

void InteractedFormTracker::TrackElement() {
  // Already has observer for last interacted element.
  if (form_element_observer_)
    return;
  std::unique_ptr<FormElementObserverCallback> callback(
      new FormElementObserverCallback(this));

  if (!last_interacted_form_.IsNull()) {
    form_element_observer_ = blink::WebFormElementObserver::Create(
        last_interacted_form_, std::move(callback));
  } else if (!last_interacted_formless_element_.IsNull()) {
    form_element_observer_ = blink::WebFormElementObserver::Create(
        last_interacted_formless_element_, std::move(callback));
  }
}

void InteractedFormTracker::ResetLastInteractedElements() {
  last_interacted_form_.Reset();
  last_interacted_formless_element_.Reset();
  if (form_element_observer_) {
    form_element_observer_->Disconnect();
    form_element_observer_ = nullptr;
  }
}

}  // namespace autofill
