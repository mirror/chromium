// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "modules/zoom_metrics/StyleCollector.h"

#include "core/dom/Document.h"
#include "core/dom/Element.h"
#include "core/dom/ElementShadow.h"
#include "core/dom/Node.h"
#include "core/dom/NodeTraversal.h"
#include "core/frame/LocalFrame.h"
#include "core/frame/LocalFrameView.h"
#include "core/html/HTMLElement.h"
#include "core/html/HTMLImageElement.h"
#include "core/html/forms/HTMLInputElement.h"
#include "core/html_element_type_helpers.h"
#include "core/layout/LayoutObject.h"
#include "core/layout/api/LayoutViewItem.h"
#include "core/style/ComputedStyle.h"
#include "mojo/public/cpp/bindings/strong_binding.h"
#include "platform/LayoutUnit.h"
#include "platform/geometry/IntRect.h"
#include "platform/geometry/IntSize.h"
#include "platform/scheduler/child/web_scheduler.h"
#include "platform/wtf/Optional.h"
#include "platform/wtf/PtrUtil.h"
#include "platform/wtf/text/WTFString.h"
#include "platform/zoom_metrics/StyleInfo.h"
#include "public/platform/Platform.h"
#include "public/platform/WebSize.h"

namespace blink {
namespace zoom_metrics {

namespace {

int Area(const IntSize& size) {
  return size.Area();
}

int Area(const IntRect& rect) {
  return Area(rect.Size());
}

// Return the specified font size for |elem|.
int FontSizeForElement(const Element* elem) {
  DCHECK(elem);
  const LayoutObject* layout = elem->GetLayoutObject();
  if (!layout)
    return 0;

  return static_cast<int>(layout->StyleRef().SpecifiedFontSize());
}

// Fill |info| with style information about the text |elem_text| contained
// in |elem|.
void ProcessTextInElement(const String& elem_text,
                          const Element* elem,
                          const IntRect& text_bounds,
                          StyleInfo& info) {
  const String text = elem_text.StripWhiteSpace();
  if (text.IsEmpty())
    return;

  const int font_size = FontSizeForElement(elem);
  if (font_size <= 0)
    return;

  auto insertion = info.font_size_distribution.insert(font_size, 0);
  insertion.stored_value->value += text.length();
  info.text_area += Area(text_bounds);
}

// Fill |info| with style information about the given text node.
void VisitTextNode(const Node* node, StyleInfo& info) {
  DCHECK(node->IsTextNode());
  const Element* elem = node->parentElement();
  if (!elem)
    return;

  // We want to use the bounds of the text node itself and not the element's,
  // since the element could be much larger than the space taken up by the
  // actual text. Also, the element may have multiple text nodes and we don't
  // want to double count the area.
  const IntRect bounds = node->PixelSnappedBoundingBox();

  ProcessTextInElement(node->nodeValue(), elem, bounds, info);
}

// Fill |info| with style information about the given input element.
void VisitInputElement(const HTMLInputElement* input, StyleInfo& info) {
  // Button text is defined in the value attribute, not in a text node, so
  // we have to check this separately.
  if (!input->IsTextButton())
    return;
  ProcessTextInElement(input->ValueOrDefaultLabel(), input,
                       input->BoundsInViewport(), info);
}

// Fill |info| with style information about the given image element.
void VisitImageElement(const HTMLImageElement* image, StyleInfo& info) {
  const IntRect bounds = image->BoundsInViewport();
  info.image_area += Area(bounds);

  if (!info.largest_image || Area(info.largest_image.value()) < Area(bounds))
    info.largest_image = WebSize(bounds.Size());
}

// Fill |info| with style information about elements where we only care about
// generic sizing information.
void VisitOtherElement(const Element* elem, StyleInfo& info) {
  const IntRect bounds = elem->BoundsInViewport();
  info.object_area += Area(bounds);

  if (!info.largest_object || Area(info.largest_object.value()) < Area(bounds))
    info.largest_object = WebSize(bounds.Size());
}

// Fill |info| with style information about the |node|.
// Returns true if we should visit the node's children.
bool VisitNode(const Node* node, StyleInfo& info) {
  if (!node || node->getNodeType() == Node::kCommentNode ||
      (node->IsElementNode() && !ToElement(node)->HasNonEmptyLayoutSize())) {
    return false;
  }

  if (node->IsTextNode()) {
    VisitTextNode(node, info);
    return false;
  }

  if (node->IsElementNode()) {
    const Element* elem = ToElement(node);

    if (IsHTMLInputElement(elem)) {
      VisitInputElement(ToHTMLInputElement(elem), info);
      return false;
    }

    if (IsHTMLImageElement(elem)) {
      VisitImageElement(ToHTMLImageElement(elem), info);
      return false;
    }

    if (IsHTMLAudioElement(elem) || IsHTMLCanvasElement(elem) ||
        IsHTMLEmbedElement(elem) || IsHTMLObjectElement(elem) ||
        IsHTMLVideoElement(elem) || elem->IsSVGElement()) {
      VisitOtherElement(elem, info);
      return false;
    }
  }

  return true;
}

// If |current| is in a shadow tree and there is a node following the shadow
// host in a pre-order traversal, returns that next node. Otherwise, returns
// nullptr.
const Node* NextNodeOutsideShadowTree(const Node& current) {
  const Element* shadow_host = current.OwnerShadowHost();
  if (!shadow_host)
    return nullptr;
  return NodeTraversal::NextSkippingChildren(*shadow_host);
}

// Like NodeTraversal::Next, but traverses across shadow trees.
const Node* NextNodeIncludingShadow(const Node& current) {
  if (IsShadowHost(current))
    return &ToElement(current).Shadow()->YoungestShadowRoot();

  const Node* next_node = NodeTraversal::Next(current);
  if (!next_node)
    next_node = NextNodeOutsideShadowTree(current);
  return next_node;
}

// Like NodeTraversal::NextSkippingChildren, but traverses across shadow trees.
const Node* NextNodeSkippingChildrenIncludingShadow(const Node& current) {
  const Node* next_node = NodeTraversal::NextSkippingChildren(current);
  if (!next_node)
    next_node = NextNodeOutsideShadowTree(current);
  return next_node;
}

// We consider ourselves too close to a deadline if we're within this period of
// time (in seconds).
const double kDeadlineSlack = 0.001;

// Returns true if we've reached the deadline, or we're too close to the
// deadline to continue.
bool IsDeadlineExceededOrClose(double deadline_seconds) {
  return (deadline_seconds - kDeadlineSlack) <= MonotonicallyIncreasingTime();
}

// Traverse the document, filling |info| with the style information.
// |start_node| is either the node from which we start the traversal or a node
// from which we resume the traversal if we needed to schedule another idle
// task.
void Traverse(WeakPersistent<const Node> start_node,
              StyleInfo info,
              StyleCollector::GetStyleCallback callback,
              double deadline_seconds) {
  if (!start_node) {
    std::move(callback).Run(WTF::nullopt);
    return;
  }

  const Node* node = start_node.Get();

  while (true) {
    if ((node != start_node.Get()) &&
        IsDeadlineExceededOrClose(deadline_seconds)) {
      // We couldn't finish the traversal within the deadline, so request
      // another idle task to continue from this node.
      Platform::Current()->CurrentThread()->Scheduler()->PostIdleTask(
          BLINK_FROM_HERE, WTF::Bind(Traverse, WrapWeakPersistent(node), info,
                                     WTF::Passed(std::move(callback))));
      return;
    }

    bool visit_children = VisitNode(node, info);

    node = (visit_children ? NextNodeIncludingShadow(*node)
                           : NextNodeSkippingChildrenIncludingShadow(*node));

    if (!node) {
      std::move(callback).Run(std::move(info));
      return;
    }
  }
}

}  // namespace

StyleCollector::StyleCollector(const LocalFrame& frame)
    : frame_(frame), weak_ptr_factory_(this) {}

// static
void StyleCollector::BindMojoRequest(
    const LocalFrame* frame,
    mojom::zoom_metrics::blink::StyleCollectorRequest request) {
  DCHECK(frame);

  mojo::MakeStrongBinding(std::make_unique<StyleCollector>(*frame),
                          std::move(request));
}

void StyleCollector::GetStyleImmediately(GetStyleCallback callback,
                                         double deadline_seconds) {
  if (!frame_ || !frame_->GetDocument()) {
    std::move(callback).Run(WTF::nullopt);
    return;
  }

  Document* document = frame_->GetDocument();

  if (!document->IsActive() || !document->IsHTMLDocument() ||
      !document->body()) {
    std::move(callback).Run(WTF::nullopt);
    return;
  }

  StyleInfo info;

  if (frame_->IsMainFrame()) {
    const LocalFrameView* view = frame_->View();
    DCHECK(view);
    const LayoutViewItem layout_view_item = document->GetLayoutViewItem();
    DCHECK(!layout_view_item.IsNull());

    info.main_frame_info = StyleInfo::MainFrameStyleInfo();
    info.main_frame_info->visible_content_size = view->VisibleContentSize();
    info.main_frame_info->contents_size = view->ContentsSize();
    info.main_frame_info->preferred_width =
        layout_view_item.MinPreferredLogicalWidth().ToInt();
  }

  Traverse(document, info, std::move(callback), deadline_seconds);
}

void StyleCollector::GetStyle(GetStyleCallback callback) {
  Platform::Current()->CurrentThread()->Scheduler()->PostIdleTask(
      BLINK_FROM_HERE, WTF::Bind(&StyleCollector::GetStyleImmediately,
                                 weak_ptr_factory_.CreateWeakPtr(),
                                 WTF::Passed(std::move(callback))));
}

}  // namespace zoom_metrics
}  // namespace blink
