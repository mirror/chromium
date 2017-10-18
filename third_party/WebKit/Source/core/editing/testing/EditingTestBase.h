// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EditingTestBase_h
#define EditingTestBase_h

#include <gtest/gtest.h>
#include <memory>
#include <string>
#include "core/editing/Forward.h"
#include "core/layout/LayoutTestHelper.h"
#include "platform/wtf/Forward.h"

namespace blink {

class FrameSelection;

class EditingTestBase : public RenderingTest {
  USING_FAST_MALLOC(EditingTestBase);

 public:
  static ShadowRoot* CreateShadowRootForElementWithIDAndSetInnerHTML(
      TreeScope&,
      const char* host_element_id,
      const char* shadow_root_content);

 protected:
  EditingTestBase();
  ~EditingTestBase() override;

  // Insert STYLE element with |style_rules|, no need to have "<style>", into
  // HEAD.
  // TODO(crbug.com/771800): This function should be moved to RenderingTest.
  void InsertStyleElement(const std::string& style_rules);

  // Returns |Position| for specified |caret_text|, which is HTML markup with
  // caret marker "|".
  Position SetCaretTextToBody(const std::string& caret_text);

  // Returns |SelectionInDOMTree| for specified |selection_text| by using
  // |SetSelectionText()| on BODY.
  SelectionInDOMTree SetSelectionTextToBody(const std::string& selection_text);

  // Sets |HTMLElement#innerHTML| with |selection_text|, which is HTML markup
  // with selection markers "^" and "|" and returns |SelectionInDOMTree| of
  // specified selection markers.
  // See also |GetSelectionText()| which returns selection text from specified
  // |ContainerNode| and |SelectionInDOMTree|.
  // Note: Unlike |assert_selection()|, this function doesn't change
  // |FrameSelection|.
  SelectionInDOMTree SetSelectionText(HTMLElement*,
                                      const std::string& selection_text);

  // Returns selection text for child nodes of BODY with specified
  // |SelectionInDOMTree|.
  std::string GetSelectionTextFromBody(const SelectionInDOMTree&) const;

  // TODO(crbug.com/771800): This function should be removed. Callers should
  // call RenderingTest::SetBodyInnerHTML directly.
  void SetBodyContent(const std::string&);

  ShadowRoot* SetShadowContent(const char* shadow_content,
                               const char* shadow_host_id);

  // TODO(crbug.com/771800): This function should be moved to RenderingTest.
  void UpdateAllLifecyclePhases();

};

}  // namespace blink

#endif  // EditingTestBase_h
