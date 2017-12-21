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
    "child0child1child2child3child4child5child6child7child8";

const char kElevenChildrenNested[] =
    "<div id='target'>"
    "<div id='a1'>child0"
    "<div id='a2'>child1"
    "<div id='a3'>child2"
    "<div id='a4'>child3"
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
    "child0child1child2child3child4child5child6child7child8";

const char kSkipElement[] =
    "<div id='target'>"
    "<div>child0</div>"
    "<div class='skip'>child1</div>"
    "<div>child2</div>"
    "</div>";
const char kSkipElementExpected[] = "child0child2";

const char kRightLimit[] =
    "<div id='target'>"
    "<div>child0</div>"
    "<div class='right'>child1</div>"
    "<div>child2</div>"
    "</div>";
const char kRightLimitExpected[] = "child0child1";

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
      {"Space after text", "<div id='target'>one<span>two</span></div>",
       "onetwo"},
      {"Ignore input", "<div id='target'>one<input value='test'/>two</div>",
       "onetwo"},
      {"Trim", "<div id='target'>   one<input value='test'/>two  </div>",
       "onetwo"},
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
      {"Right limit", kRightLimit, kRightLimitExpected},
  };
  for (auto test_case : test_cases) {
    SCOPED_TRACE(test_case.description);
    LoadHTML(test_case.html);
    WebLocalFrame* web_frame = GetMainFrame();
    ASSERT_NE(nullptr, web_frame);
    WebElement target = web_frame->GetDocument().GetElementById("target");
    WebVector<WebElement> web_to_skip =
        web_frame->GetDocument().QuerySelectorAll("div[class='skip']");
    std::set<blink::WebNode> to_skip;
    printf("------------- skip size %d\n", (int)web_to_skip.size());
    for (size_t i = 0; i < web_to_skip.size(); ++i) {
      to_skip.insert(web_to_skip[i]);
    }
    WebVector<WebElement> web_right_limits =
        web_frame->GetDocument().QuerySelectorAll("div[class='right']");
    printf("------------- right size %d\n", (int)web_right_limits.size());
    std::set<blink::WebNode> right_limits;
    for (size_t i = 0; i < web_right_limits.size(); ++i) {
      right_limits.insert(web_right_limits[i]);
    }
    ASSERT_FALSE(target.IsNull());
    EXPECT_EQ(base::UTF8ToUTF16(test_case.expected_label),
              autofill::form_util::FindChildTextWithIgnoreList(target, to_skip,
                                                               right_limits));
  }
}
