// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "document_statistics_collector.h"

#include "base/strings/string_util.h"
#include "base/trace_event/trace_event.h"
#include "components/dom_distiller/content/renderer/web_distillability.h"
//#include "third_party/WebKit/Source/platform/Histogram.h"
#include "third_party/WebKit/public/platform/Platform.h"
#include "third_party/WebKit/public/platform/WebString.h"
#include "third_party/WebKit/webagents/document.h"
#include "third_party/WebKit/webagents/element.h"
#include "third_party/WebKit/webagents/html_head_element.h"
#include "third_party/WebKit/webagents/html_input_element.h"
#include "third_party/WebKit/webagents/html_meta_element.h"
#include "third_party/WebKit/webagents/node_iterator.h"
#include "third_party/WebKit/webagents/text.h"

namespace dom_distiller {

using namespace webagents;

namespace {

// Saturate the length of a paragraph to save time.
const int kTextContentLengthSaturation = 1000;

// Filter out short P elements. The threshold is set to around 2 English
// sentences.
const unsigned kParagraphLengthThreshold = 140;

// Saturate the scores to save time. The max is the score of 6 long paragraphs.
// 6 * sqrt(kTextContentLengthSaturation - kParagraphLengthThreshold)
const double kMozScoreSaturation = 175.954539583;
// 6 * sqrt(kTextContentLengthSaturation);
const double kMozScoreAllSqrtSaturation = 189.73665961;
const double kMozScoreAllLinearSaturation = 6 * kTextContentLengthSaturation;

unsigned TextContentLengthSaturated(const Element& root) {
  unsigned length = 0;
  // This skips shadow DOM intentionally, to match the JavaScript
  // implementation.  We would like to use the same statistics extracted by the
  // JavaScript implementation on iOS, and JavaScript cannot peek deeply into
  // shadow DOM except on modern Chrome versions.
  // Given shadow DOM rarely appears in <P> elements in long-form articles, the
  // overall accuracy should not be largely affected.

  for (const Node node : NodeIterator::WithDescendants(root)) {
    if (!node.IsTextNode()) {
      continue;
    }
    length += node.ToText().length();
    if (length > kTextContentLengthSaturation) {
      return kTextContentLengthSaturation;
    }
  }
  return length;
}

bool MatchAttributes(const Element& element,
                     const std::vector<std::string>& lower_words) {
  const std::string& classes = element.GetClassAttribute().Utf8();
  const std::string& id = element.GetIdAttribute().Utf8();
  for (const std::string& lower_word : lower_words) {
    if (base::ToLowerASCII(classes).find(lower_word) != std::string::npos ||
        base::ToLowerASCII(id).find(lower_word) != std::string::npos) {
      return true;
    }
  }
  return false;
}

bool IsGoodForScoring(const WebDistillabilityFeatures& features,
                      const Element& element) {
  CR_DEFINE_STATIC_LOCAL(std::vector<std::string>, unlikely_candidates, ());
  if (unlikely_candidates.empty()) {
    auto words = {
        "banner",  "combx",      "comment", "community",  "disqus",  "extra",
        "foot",    "header",     "menu",    "related",    "remark",  "rss",
        "share",   "shoutbox",   "sidebar", "skyscraper", "sponsor", "ad-break",
        "agegate", "pagination", "pager",   "popup"};
    for (auto* word : words) {
      unlikely_candidates.push_back(word);
    }
  }
  CR_DEFINE_STATIC_LOCAL(std::vector<std::string>, highly_likely_candidates,
                         ());
  if (highly_likely_candidates.empty()) {
    auto words = {"and", "article", "body", "column", "main", "shadow"};
    for (auto* word : words) {
      highly_likely_candidates.push_back(word);
    }
  }

  if (!element.NotStandardIsVisible())
    return false;
  if (features.moz_score >= kMozScoreSaturation &&
      features.moz_score_all_sqrt >= kMozScoreAllSqrtSaturation &&
      features.moz_score_all_linear >= kMozScoreAllLinearSaturation)
    return false;
  if (MatchAttributes(element, unlikely_candidates) &&
      !MatchAttributes(element, highly_likely_candidates))
    return false;
  return true;
}

// underListItem denotes that at least one of the ancesters is <li> element.
void CollectFeatures(const Element& root,
                     WebDistillabilityFeatures& features,
                     bool under_list_item = false) {
  for (Node node : NodeIterator::ChildrenOf(root)) {
    bool is_list_item = false;
    if (!node.IsElementNode()) {
      continue;
    }

    features.element_count++;
    const Element element = node.ToElement();
    // TODO: HTMLNames
    if (element.tagName() == "A") {
      features.anchor_count++;
    } else if (element.tagName() == "FORM") {
      features.form_count++;
    } else if (element.tagName() == "INPUT") {
      const HTMLInputElement input = element.ToHTMLInputElement();
      // TODO: InputTypeNames
      if (input.type() == "text") {
        features.text_input_count++;
      } else if (input.type() == "password") {
        features.password_input_count++;
      }
    } else if (element.tagName() == "P" || element.tagName() == "PRE") {
      if (element.tagName() == "P") {
        features.p_count++;
      } else {
        features.pre_count++;
      }
      if (!under_list_item && IsGoodForScoring(features, element)) {
        unsigned length = TextContentLengthSaturated(element);
        if (length >= kParagraphLengthThreshold) {
          features.moz_score += sqrt(length - kParagraphLengthThreshold);
          features.moz_score =
              std::min(features.moz_score, kMozScoreSaturation);
        }
        features.moz_score_all_sqrt += sqrt(length);
        features.moz_score_all_sqrt =
            std::min(features.moz_score_all_sqrt, kMozScoreAllSqrtSaturation);

        features.moz_score_all_linear += length;
        features.moz_score_all_linear = std::min(features.moz_score_all_linear,
                                                 kMozScoreAllLinearSaturation);
      }
    } else if (element.tagName() == "LI") {
      is_list_item = true;
    }
    CollectFeatures(element, features, under_list_item || is_list_item);
  }
}

bool HasOpenGraphArticle(const Element& head) {
  CR_DEFINE_STATIC_LOCAL(blink::WebString, og_type, ("og:type"));
  CR_DEFINE_STATIC_LOCAL(blink::WebString, property_attr, ("property"));
  for (base::Optional<Node> child = head.firstChild(); child;
       child = child->nextSibling()) {
    if (!child->IsElementNode() || child->ToElement().tagName() != "META")
      continue;

    const HTMLMetaElement meta = child->ToHTMLMetaElement();

    if (meta.name() == og_type || meta.getAttribute(property_attr) == og_type) {
      if (base::LowerCaseEqualsASCII(meta.content().Utf8(), "article")) {
        return true;
      }
    }
  }
  return false;
}

bool IsMobileFriendly(const Document& document) {
  // TODO: fix
  // if (Page* page = document.GetPage())
  //  return page->GetVisualViewport().ShouldDisableDesktopWorkarounds();
  return false;
}

}  // namespace

WebDistillabilityFeatures DocumentStatisticsCollector::CollectStatistics(
    Document document) {
  TRACE_EVENT0("blink", "DocumentStatisticsCollector::collectStatistics");

  WebDistillabilityFeatures features = WebDistillabilityFeatures();

  // TODO: fix
  // if (!document.GetFrame() || !document.GetFrame()->IsMainFrame())
  //  return features;

  // TODO: fix
  // DCHECK(document.HasFinishedParsing());

  base::Optional<HTMLElement> body = document.body();
  base::Optional<HTMLHeadElement> head = document.head();

  if (!body || !head)
    return features;

  features.is_mobile_friendly = IsMobileFriendly(document);

  // double start_time = MonotonicallyIncreasingTime();
  // base::TimeTicks start_time =  base::TimeTicks::Now();

  // This should be cheap since collectStatistics is only called right after
  // layout.
  document.NotStandardUpdateStyleAndLayoutTree();

  // Traverse the DOM tree and collect statistics.
  CollectFeatures(body.value(), features);
  features.open_graph = HasOpenGraphArticle(head.value());

  // TODO: fix - relies on Source/platform/Histogram.h import
  // CR_DEFINE_STATIC_LOCAL(CustomCountHistogram, distillability_histogram,
  //                    ("WebCore.DistillabilityUs", 1, 1000000, 50));
  // distillability_histogram.Count(base::TimeTicks.Now() - start_time);

  return features;
}

}  // namespace dom_distiller
