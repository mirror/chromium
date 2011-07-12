// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_RENDERER_AUTOFILL_FORM_MANAGER_H_
#define CHROME_RENDERER_AUTOFILL_FORM_MANAGER_H_
#pragma once

#include <map>
#include <vector>

#include "base/callback_old.h"
#include "base/memory/scoped_vector.h"
#include "base/string16.h"
#include "third_party/WebKit/Source/WebKit/chromium/public/WebFormElement.h"

namespace webkit_glue {
struct FormData;
struct FormDataPredictions;
struct FormField;
}  // namespace webkit_glue

namespace WebKit {
class WebFormControlElement;
class WebFrame;
}  // namespace WebKit

namespace autofill {

// Manages the forms in a RenderView.
class FormManager {
 public:
  // A bit field mask for form requirements.
  enum RequirementsMask {
    REQUIRE_NONE         = 0,       // No requirements.
    REQUIRE_AUTOCOMPLETE = 1 << 0,  // Require that autocomplete != off.
    REQUIRE_ENABLED      = 1 << 1,  // Require that disabled attribute is off.
    REQUIRE_EMPTY        = 1 << 2,  // Require that the fields are empty.
  };

  // A bit field mask to extract data from WebFormControlElement.
  enum ExtractMask {
    EXTRACT_NONE        = 0,
    EXTRACT_VALUE       = 1 << 0,  // Extract value from WebFormControlElement.
    EXTRACT_OPTION_TEXT = 1 << 1,  // Extract option text from
                                   // WebFormSelectElement. Only valid when
                                   // |EXTRACT_VALUE| is set.
                                   // This is used for form submission where
                                   // human readable value is captured.
    EXTRACT_OPTIONS     = 1 << 2,  // Extract options from
                                   // WebFormControlElement.
  };

  FormManager();
  virtual ~FormManager();

  // Fills out a FormField object from a given WebFormControlElement.
  // |extract_mask|: See the enum ExtractMask above for details.
  static void WebFormControlElementToFormField(
      const WebKit::WebFormControlElement& element,
      ExtractMask extract_mask,
      webkit_glue::FormField* field);

  // Returns the corresponding label for |element|.  WARNING: This method can
  // potentially be very slow.  Do not use during any code paths where the page
  // is loading.
  // TODO(isherman): Refactor autofill_agent.cc not to require this method to be
  // exposed.
  static string16 LabelForElement(const WebKit::WebFormControlElement& element);

  // Fills out a FormData object from a given WebFormElement. If |get_values|
  // is true, the fields in |form| will have the values filled out. If
  // |get_options| is true, the fields in |form will have select options filled
  // out. Returns true if |form| is filled out; it's possible that |element|
  // won't meet the requirements in |requirements|. This also returns false if
  // there are no fields in |form|.
  static bool WebFormElementToFormData(const WebKit::WebFormElement& element,
                                       RequirementsMask requirements,
                                       ExtractMask extract_mask,
                                       webkit_glue::FormData* form);

  // Scans the DOM in |frame| extracting and storing forms.
  // Returns a vector of the extracted forms.
  void ExtractForms(const WebKit::WebFrame* frame,
                    std::vector<webkit_glue::FormData>* forms);

  // Finds the form that contains |element| and returns it in |form|. Returns
  // false if the form is not found.
  bool FindFormWithFormControlElement(
      const WebKit::WebFormControlElement& element,
      webkit_glue::FormData* form);

  // Fills the form represented by |form|. |node| is the input element that
  // initiated the auto-fill process.
  void FillForm(const webkit_glue::FormData& form, const WebKit::WebNode& node);

  // Previews the form represented by |form|. |node| is the input element that
  // initiated the preview process.
  void PreviewForm(const webkit_glue::FormData& form,
                   const WebKit::WebNode &node);

  // For each field in the |form|, sets the field's placeholder text to the
  // field's overall predicted type.  Also sets the title to include the field's
  // heuristic type, server type, and signature; as well as the form's signature
  // and the experiment id for the server predictions.
  bool ShowPredictions(const webkit_glue::FormDataPredictions& form);

  // Clears the values of all input elements in the form that contains |node|.
  // Returns false if the form is not found.
  bool ClearFormWithNode(const WebKit::WebNode& node);

  // Clears the placeholder values and the auto-filled background for any fields
  // in the form containing |node| that have been previewed. Resets the
  // autofilled state of |node| to |was_autofilled|. Returns false if the form
  // is not found.
  bool ClearPreviewedFormWithNode(const WebKit::WebNode& node,
                                  bool was_autofilled);

  // Resets the forms for the specified |frame|.
  void ResetFrame(const WebKit::WebFrame* frame);

  // Returns true if |form| has any auto-filled fields.
  bool FormWithNodeIsAutofilled(const WebKit::WebNode& node);

 private:
  // Stores the WebFormElement and the form control elements for a form.
  // Original form values are stored so when we clear a form we can reset
  // <select> elements to their original value.
  struct FormElement;

  // Type for cache of FormElement objects.
  typedef ScopedVector<FormElement> FormElementList;

  // Finds the cached FormElement that contains |node|.
  bool FindCachedFormElementWithNode(const WebKit::WebNode& node,
                                     FormElement** form_element);

  // Uses the data in |form| to find the cached FormElement.
  bool FindCachedFormElement(const webkit_glue::FormData& form,
                             FormElement** form_element);

  // The cached FormElement objects.
  FormElementList form_elements_;

  DISALLOW_COPY_AND_ASSIGN(FormManager);
};

}  // namespace autofill

#endif  // CHROME_RENDERER_AUTOFILL_FORM_MANAGER_H_
