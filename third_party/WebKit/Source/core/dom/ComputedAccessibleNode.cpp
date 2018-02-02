// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/dom/ComputedAccessibleNode.h"

#include <stdint.h>

#include "core/dom/DOMException.h"
#include "platform/bindings/ScriptState.h"
#include "third_party/WebKit/Source/bindings/core/v8/ScriptPromiseResolver.h"
#include "third_party/WebKit/Source/core/frame/LocalFrame.h"
#include "third_party/WebKit/Source/core/frame/WebLocalFrameImpl.h"
#include "third_party/WebKit/Source/modules/accessibility/AXObjectCacheImpl.h"
#include "third_party/WebKit/Source/platform/wtf/text/WTFString.h"
#include "third_party/WebKit/public/web/WebFrameClient.h"

namespace blink {

GetComputedAccessibleNodePromiseResolver*
GetComputedAccessibleNodePromiseResolver::Create(ScriptState* script_state,
                                                 Element* element) {
  return new GetComputedAccessibleNodePromiseResolver(script_state, element);
}

GetComputedAccessibleNodePromiseResolver::
    GetComputedAccessibleNodePromiseResolver(ScriptState* script_state,
                                             Element* element)
    : ContextLifecycleObserver(ExecutionContext::From(script_state)),
      element_(element),
      resolver_(ScriptPromiseResolver::Create(script_state)),
      timer_(GetExecutionContext()->GetTaskRunner(TaskType::kMicrotask),
             this,
             &GetComputedAccessibleNodePromiseResolver::OnTimerFired) {}

ScriptPromise GetComputedAccessibleNodePromiseResolver::Promise() {
  return resolver_->Promise();
}

void GetComputedAccessibleNodePromiseResolver::Trace(blink::Visitor* visitor) {
  ContextLifecycleObserver::Trace(visitor);
  visitor->Trace(element_);
  visitor->Trace(resolver_);
}

void GetComputedAccessibleNodePromiseResolver::ComputeAccessibleNode() {
  DCHECK(RuntimeEnabledFeatures::AccessibilityObjectModelEnabled());
  timer_.StartOneShot(TimeDelta(), FROM_HERE);
}

void GetComputedAccessibleNodePromiseResolver::OnTimerFired(TimerBase*) {
  Document& document = element_->GetDocument();
  document.UpdateStyleAndLayoutIgnorePendingStylesheetsForNode(element_);

  AXObjectCache* cache = element_->GetDocument().GetOrCreateAXObjectCache();
  DCHECK(cache);
  AXID ax_id = cache->GetAXID(element_);

  LocalFrame* local_frame = element_->ownerDocument()->GetFrame();
  WebFrameClient* client = WebLocalFrameImpl::FromFrame(local_frame)->Client();
  WebComputedAXTree* tree = client->GetOrCreateWebComputedAXTree();
  tree->ComputeAccessibilityTree();

  ComputedAccessibleNode* accessible_node =
      ComputedAccessibleNode::Create(ax_id, tree);
  resolver_->Resolve(accessible_node);
}

ComputedAccessibleNode* ComputedAccessibleNode::Create(
    AXID ax_id,
    WebComputedAXTree* tree) {
  return new ComputedAccessibleNode(ax_id, tree);
}

ComputedAccessibleNode::ComputedAccessibleNode(AXID ax_id,
                                               WebComputedAXTree* tree)
    : ax_id_(ax_id), tree_(tree) {}

ComputedAccessibleNode::~ComputedAccessibleNode() {}

int32_t ComputedAccessibleNode::GetIntAttribute(WebAOMIntAttribute attr,
                                                bool& is_null) const {
  int32_t out = 0;
  is_null = true;
  if (tree_->GetIntAttributeForAXNode(ax_id_, attr, &out)) {
    is_null = false;
  }
  return out;
}

const String ComputedAccessibleNode::name() const {
  return tree_->GetNameForAXNode(ax_id_);
}

const String ComputedAccessibleNode::role() const {
  LOG(INFO) << "GetRoleForAXNode";
  return tree_->GetRoleForAXNode(ax_id_);
}

int32_t ComputedAccessibleNode::colCount(bool& is_null) const {
  return GetIntAttribute(WebAOMIntAttribute::AOM_ATTR_COLUMN_COUNT, is_null);
}

int32_t ComputedAccessibleNode::colIndex(bool& is_null) const {
  return GetIntAttribute(WebAOMIntAttribute::AOM_ATTR_COLUMN_INDEX, is_null);
}

int32_t ComputedAccessibleNode::colSpan(bool& is_null) const {
  return GetIntAttribute(WebAOMIntAttribute::AOM_ATTR_COLUMN_SPAN, is_null);
}

int32_t ComputedAccessibleNode::level(bool& is_null) const {
  return GetIntAttribute(WebAOMIntAttribute::AOM_ATTR_HIERARCHICAL_LEVEL,
                         is_null);
}

int32_t ComputedAccessibleNode::posInSet(bool& is_null) const {
  return GetIntAttribute(WebAOMIntAttribute::AOM_ATTR_POS_IN_SET, is_null);
}

int32_t ComputedAccessibleNode::rowCount(bool& is_null) const {
  return GetIntAttribute(WebAOMIntAttribute::AOM_ATTR_ROW_COUNT, is_null);
}

int32_t ComputedAccessibleNode::rowIndex(bool& is_null) const {
  return GetIntAttribute(WebAOMIntAttribute::AOM_ATTR_ROW_INDEX, is_null);
}

int32_t ComputedAccessibleNode::rowSpan(bool& is_null) const {
  return GetIntAttribute(WebAOMIntAttribute::AOM_ATTR_ROW_SPAN, is_null);
}

int32_t ComputedAccessibleNode::setSize(bool& is_null) const {
  return GetIntAttribute(WebAOMIntAttribute::AOM_ATTR_SET_SIZE, is_null);
}

ComputedAccessibleNode* ComputedAccessibleNode::parent() const {
  int32_t parent_ax_id;
  if (!tree_->GetParentIdForAXNode(ax_id_, &parent_ax_id)) {
    return nullptr;
  }
  return ComputedAccessibleNode::Create(parent_ax_id, tree_);
}

ComputedAccessibleNode* ComputedAccessibleNode::firstChild() const {
  int32_t child_ax_id;
  if (!tree_->GetFirstChildIdForAXNode(ax_id_, &child_ax_id)) {
    return nullptr;
  }
  return ComputedAccessibleNode::Create(child_ax_id, tree_);
}

ComputedAccessibleNode* ComputedAccessibleNode::lastChild() const {
  int32_t child_ax_id;
  if (!tree_->GetLastChildIdForAXNode(ax_id_, &child_ax_id)) {
    return nullptr;
  }
  return ComputedAccessibleNode::Create(child_ax_id, tree_);
}

ComputedAccessibleNode* ComputedAccessibleNode::previousSibling() const {
  int32_t sibling_ax_id;
  if (!tree_->GetPreviousSiblingIdForAXNode(ax_id_, &sibling_ax_id)) {
    return nullptr;
  }
  return ComputedAccessibleNode::Create(sibling_ax_id, tree_);
}

ComputedAccessibleNode* ComputedAccessibleNode::nextSibling() const {
  int32_t sibling_ax_id;
  if (!tree_->GetNextSiblingIdForAXNode(ax_id_, &sibling_ax_id)) {
    return nullptr;
  }
  return ComputedAccessibleNode::Create(sibling_ax_id, tree_);
}

void ComputedAccessibleNode::OnSnapshotResponse(
    ScriptPromiseResolver* resolver) {
  resolver->Resolve(this);
}

void ComputedAccessibleNode::Trace(Visitor* visitor) {
  ScriptWrappable::Trace(visitor);
  visitor->Trace(cache_);
}

}  // namespace blink
