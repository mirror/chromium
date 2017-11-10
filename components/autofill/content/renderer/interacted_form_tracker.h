// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_AUTOFILL_CONTENT_RENDERER_INTERACTED_FORM_TRACKER_H_
#define COMPONENTS_AUTOFILL_CONTENT_RENDERER_INTERACTED_FORM_TRACKER_H_

#include "base/memory/weak_ptr.h"
#include "base/observer_list.h"
#include "content/public/renderer/render_frame_observer.h"
#include "third_party/WebKit/public/web/WebInputElement.h"

namespace blink {
class WebFormElementObserver;
}

namespace autofill {

class InteractedFormTracker : public content::RenderFrameObserver {
 public:
  class Observer {
   public:
    // Probably should merge with PasswordForm::SubmissionIndicatorEvent.
    enum class SubmissionSource {
      SAME_DOCUMENT_NAVIGATION,
      XHR_SUCCEEDED,
      FRAME_DETACHED,
      DOM_MUTATION_AFTER_XHR,
    };

    enum class ElementChangeSource {
      TEXTFIELD_CHANGED,
      WILL_SEND_SUBMIT_EVENT,
    };

    // Invoked when form needs to be saved because of |source|, |form| isn't
    // valid if this callback caused by TEXTFIELD_CHANGED, |element| isn't
    // valid for the callback caused by WILL_SEND_SUBMIT_EVENT.
    virtual void OnProvisionallySaveForm(const blink::WebFormElement& form,
                                         const blink::WebInputElement& element,
                                         ElementChangeSource source) = 0;

    // Invoked when |form| is submitted, observer shall assume the form saved
    // in OnProvisionallySaveForm() is submitted if the |form| is not valid.
    // The submission might not be successful, observer needs to check whether
    // the form exists in new page.
    virtual void OnFormSubmitted(const blink::WebFormElement& form) = 0;

    // Invoked when tracker infers the form saved in OnProvisionallySaveForm()
    // is successfully submitted from the |source|, observer doesn't need to
    // check whether the form exists.
    virtual void OnSuccessfulSubmission(SubmissionSource source) = 0;

   protected:
    virtual ~Observer() {}
  };

  InteractedFormTracker(content::RenderFrame* render_frame);
  ~InteractedFormTracker() override;

  void AddObserver(Observer* observer);
  void RemoveObserver(Observer* observer);

  void AjaxSucceeded();
  void TextFieldDidChange(const blink::WebFormControlElement& element);

  void set_ignore_text_changes(bool ignore_text_changes) {
    ignore_text_changes_ = ignore_text_changes;
  }

  void set_user_gesture_required(bool required) {
    user_gesture_required_ = required;
  }

 private:
  class FormElementObserverCallback;

  // content::RenderFrameObserver:
  void DidCommitProvisionalLoad(bool is_new_navigation,
                                bool is_same_document_navigation) override;
  void DidStartProvisionalLoad(
      blink::WebDocumentLoader* document_loader) override;
  void FrameDetached() override;
  void WillSendSubmitEvent(const blink::WebFormElement& form) override;
  void WillSubmitForm(const blink::WebFormElement& form) override;
  void OnDestruct() override;

  // Called in a posted task by textFieldDidChange() to work-around a WebKit bug
  // http://bugs.webkit.org/show_bug.cgi?id=16976
  void TextFieldDidChangeImpl(const blink::WebFormControlElement& element);
  void FireFormSubmitted(const blink::WebFormElement& form);
  void FireSuccessfulFormSubmission(Observer::SubmissionSource source);
  void OnSameDocumentNavigationCompleted(Observer::SubmissionSource source);
  bool CanInferFormSubmitted();
  void TrackElement();

  void ResetLastInteractedElements();

  base::ObserverList<Observer> observers_;
  bool ignore_text_changes_;
  bool user_gesture_required_;
  blink::WebFormElement last_interacted_form_;
  blink::WebInputElement last_interacted_formless_element_;
  blink::WebFormElementObserver* form_element_observer_;

  base::WeakPtrFactory<InteractedFormTracker> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(InteractedFormTracker);
};

}  // namespace autofill

#endif  // COMPONENTS_AUTOFILL_CONTENT_RENDERER_INTERACTED_FORM_TRACKER_H_
