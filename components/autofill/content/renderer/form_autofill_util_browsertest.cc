// Copyright (c) 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/autofill/content/renderer/form_autofill_util.h"

#include "base/strings/utf_string_conversions.h"
#include "content/public/test/render_view_test.h"
#include "third_party/WebKit/public/platform/WebString.h"
#include "third_party/WebKit/public/platform/WebVector.h"
#include "third_party/WebKit/public/web/WebDocument.h"
#include "third_party/WebKit/public/web/WebElement.h"
#include "third_party/WebKit/public/web/WebElementCollection.h"
#include "third_party/WebKit/public/web/WebFormControlElement.h"
#include "third_party/WebKit/public/web/WebFormElement.h"
#include "third_party/WebKit/public/web/WebInputElement.h"
#include "third_party/WebKit/public/web/WebLocalFrame.h"
#include "third_party/WebKit/public/web/WebSelectElement.h"

using blink::WebDocument;
using blink::WebElement;
using blink::WebFormControlElement;
using blink::WebFormElement;
using blink::WebInputElement;
using blink::WebLocalFrame;
using blink::WebSelectElement;
using blink::WebString;
using blink::WebVector;

struct AutofillFieldUtilCase {
  const char* description;
  const char* html;
  const char* expected_label;
};

const char kElevenChildren[] =
    "<div id='target'>"
    "<div>child0</div>"
    "<div>child1</div>"
    "<div>child2</div>"
    "<div>child3</div>"
    "<div>child4</div>"
    "<div>child5</div>"
    "<div>child6</div>"
    "<div>child7</div>"
    "<div>child8</div>"
    "<div>child9</div>"
    "<div>child10</div>"
    "</div>";
const char kElevenChildrenExpected[] =
    "child0child1child2child3child4child5child6child7child8child9";

const char kElevenChildrenNested[] =
    "<div id='target'>"
    "<div>child0"
    "<div>child1"
    "<div>child2"
    "<div>child3"
    "<div>child4"
    "<div>child5"
    "<div>child6"
    "<div>child7"
    "<div>child8"
    "<div>child9"
    "<div>child10"
    "</div></div></div></div></div></div></div></div></div></div></div></div>";
// Take 10 elements -1 for target element, -1 as text is a leaf element.
const char kElevenChildrenNestedExpected[] =
    "child0child1child2child3child4child5child6child7child8child9";

const char kSkipElement[] =
    "<div id='target'>"
    "<div>child0</div>"
    "<div class='skip'>child1</div>"
    "<div>child2</div>"
    "</div>";
const char kSkipElementExpected[] = "child0child2";

const char kDivTableExample1[] =
    "<div>"
    "<div>label</div><div><input id='target'/></div>"
    "</div>";
const char kDivTableExample1Expected[] = "label";

const char kDivTableExample2[] =
    "<div>"
    "<div>label</div>"
    "<div>should be skipped<input/></div>"
    "<div><input id='target'/></div>"
    "</div>";
const char kDivTableExample2Expected[] = "label";

const char kDivTableExample3[] =
    "<div>"
    "<div>should be skipped<input/></div>"
    "<div>label</div>"
    "<div><input id='target'/></div>"
    "</div>";
const char kDivTableExample3Expected[] = "label";

const char kDivTableExample4[] =
    "<div>"
    "<div>should be skipped<input/></div>"
    "label"
    "<div><input id='target'/></div>"
    "</div>";
const char kDivTableExample4Expected[] = "label";

const char kDivTableExample5[] =
    "<div>"
    "<div>label<div><input id='target'/></div>behind</div>"
    "</div>";
const char kDivTableExample5Expected[] = "label";

const char kDivTableExample6[] =
    "<div>"
    "<div>label<div><div>-<div><input id='target'/></div></div>"
    "</div>";
const char kDivTableExample6Expected[] = "label-";

class FormAutofillUtilsTest : public content::RenderViewTest {
 public:
  FormAutofillUtilsTest() : content::RenderViewTest() {}
  ~FormAutofillUtilsTest() override {}
};

TEST_F(FormAutofillUtilsTest, FindChildTextTest) {
  static const AutofillFieldUtilCase test_cases[] = {
      {"simple test", "<div id='target'>test</div>", "test"},
      {"Concatenate test", "<div id='target'><span>one</span>two</div>",
       "onetwo"},
      {"Ignore input", "<div id='target'>one<input value='test'/>two</div>",
       "onetwo"},
      {"Trim", "<div id='target'>   one<span>two  </span></div>", "onetwo"},
      {"Preserve space", "<div id='target'>one <div><div/>two</div>",
       "one two"},
      {"Preserve space 2", "<div id='target'>one<div><div/> two</div>",
       "one two"},
      {"Preserve space 3", "<div id='target'>one<div> <div/>two</div>",
       "one two"},
      {"eleven children", kElevenChildren, kElevenChildrenExpected},
      {"eleven children nested", kElevenChildrenNested,
       kElevenChildrenNestedExpected},
  };
  for (auto test_case : test_cases) {
    SCOPED_TRACE(test_case.description);
    LoadHTML(test_case.html);
    WebLocalFrame* web_frame = GetMainFrame();
    ASSERT_NE(nullptr, web_frame);
    WebElement target = web_frame->GetDocument().GetElementById("target");
    ASSERT_FALSE(target.IsNull());
    EXPECT_EQ(base::UTF8ToUTF16(test_case.expected_label),
              autofill::form_util::FindChildText(target));
  }
}

TEST_F(FormAutofillUtilsTest, FindChildTextSkipElementTest) {
  static const AutofillFieldUtilCase test_cases[] = {
      {"Skip div element", kSkipElement, kSkipElementExpected},
  };
  for (auto test_case : test_cases) {
    SCOPED_TRACE(test_case.description);
    LoadHTML(test_case.html);
    WebLocalFrame* web_frame = GetMainFrame();
    ASSERT_NE(nullptr, web_frame);
    WebElement target = web_frame->GetDocument().GetElementById("target");
    ASSERT_FALSE(target.IsNull());
    WebVector<WebElement> web_to_skip =
        web_frame->GetDocument().QuerySelectorAll("div[class='skip']");
    std::set<blink::WebNode> to_skip;
    for (size_t i = 0; i < web_to_skip.size(); ++i) {
      to_skip.insert(web_to_skip[i]);
    }
    WebVector<WebElement> web_right_limits =
        web_frame->GetDocument().QuerySelectorAll("div[class='right']");
    std::set<blink::WebNode> right_limits;
    for (size_t i = 0; i < web_right_limits.size(); ++i) {
      right_limits.insert(web_right_limits[i]);
    }

    EXPECT_EQ(base::UTF8ToUTF16(test_case.expected_label),
              autofill::form_util::FindChildTextWithIgnoreListForTesting(
                  target, to_skip, right_limits));
  }
}

TEST_F(FormAutofillUtilsTest, InferLabelForElementTest) {
  std::vector<base::char16> stop_words;
  stop_words.push_back(static_cast<base::char16>('-'));
  static const AutofillFieldUtilCase test_cases[] = {
      {"DIV table test 1", kDivTableExample1, kDivTableExample1Expected},
      {"DIV table test 2", kDivTableExample2, kDivTableExample2Expected},
      {"DIV table test 3", kDivTableExample3, kDivTableExample3Expected},
      {"DIV table test 4", kDivTableExample4, kDivTableExample4Expected},
      {"DIV table test 5", kDivTableExample5, kDivTableExample5Expected},
      {"DIV table test 6", kDivTableExample6, kDivTableExample6Expected},
  };
  for (auto test_case : test_cases) {
    SCOPED_TRACE(test_case.description);
    LoadHTML(test_case.html);
    WebLocalFrame* web_frame = GetMainFrame();
    ASSERT_NE(nullptr, web_frame);
    WebElement target = web_frame->GetDocument().GetElementById("target");
    ASSERT_FALSE(target.IsNull());
    const WebFormControlElement form_target =
        target.ToConst<WebFormControlElement>();
    ASSERT_FALSE(form_target.IsNull());

    EXPECT_EQ(base::UTF8ToUTF16(test_case.expected_label),
              autofill::form_util::InferLabelForElementForTesting(form_target,
                                                                  stop_words));
  }
}
