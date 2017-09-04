// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EditingTestBase_h
#define EditingTestBase_h

#include <gtest/gtest.h>
#include <memory>
#include <string>
#include "core/editing/Position.h"
#include "core/editing/SelectionTemplate.h"
#include "core/testing/DummyPageHolder.h"
#include "platform/wtf/Forward.h"

namespace blink {

class FrameSelection;
class LocalFrame;

class EditingTestBase : public ::testing::Test {
  USING_FAST_MALLOC(EditingTestBase);

 public:
  static ShadowRoot* CreateShadowRootForElementWithIDAndSetInnerHTML(
      TreeScope&,
      const char* host_element_id,
      const char* shadow_root_content);

 protected:
  EditingTestBase();
  ~EditingTestBase() override;

  // Returns |Position| for specified |selection_text| by using
  // |SetSelectionText()| on BODY.
  Position AsPosition(const std::string& selection_text);

  // Returns |SelectionInDOMTree| for specified |selection_text| by using
  // |SetSelectionText()| on BODY.
  SelectionInDOMTree AsSelection(const std::string& selection_text);

  // Sets |HTMLElement#innerHTML| with |selection_text|, which is HTML markup
  // with selection markers "^" and "|" and returns |SelectionInDOMTree| of
  // specified selection markers.
  // See also |ToSelectionText()| which returns selection text from specified
  // |ContainerNode| and |SelectionInDOMTree|.
  SelectionInDOMTree SetSelectionText(HTMLElement*,
                                      const std::string& selection_text);

  // Returns selection text, which is HTML markup with selection marker "^" and
  // "|" for specified |ContainerNode| and |SelectionInDOMTree|.
  std::string ToSelectionText(const ContainerNode&,
                              const SelectionInDOMTree&) const;

  // Returns selection text for child nodes of BODY with specified
  // |SelectionInDOMTree|.
  std::string ToSelectionText(const SelectionInDOMTree&) const;

  void SetUp() override;

  void SetupPageWithClients(Page::PageClients*);

  Document& GetDocument() const;
  DummyPageHolder& GetDummyPageHolder() const { return *dummy_page_holder_; }
  LocalFrame& GetFrame() const;
  FrameSelection& Selection() const;

  void SetBodyContent(const std::string&);
  ShadowRoot* SetShadowContent(const char* shadow_content,
                               const char* shadow_host_id);
  void UpdateAllLifecyclePhases();

 private:
  std::unique_ptr<DummyPageHolder> dummy_page_holder_;
};

}  // namespace blink

#endif  // EditingTestBase_h
