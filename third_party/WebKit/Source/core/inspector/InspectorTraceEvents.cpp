// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/inspector/InspectorTraceEvents.h"

#include <inttypes.h>

#include <memory>

#include "bindings/core/v8/ScriptSourceCode.h"
#include "bindings/core/v8/SourceLocation.h"
#include "core/animation/Animation.h"
#include "core/animation/KeyframeEffectReadOnly.h"
#include "core/css/invalidation/InvalidationSet.h"
#include "core/dom/DOMNodeIds.h"
#include "core/dom/StyleChangeReason.h"
#include "core/events/Event.h"
#include "core/frame/FrameView.h"
#include "core/frame/LocalFrame.h"
#include "core/html/HTMLFrameOwnerElement.h"
#include "core/html/parser/HTMLDocumentParser.h"
#include "core/inspector/IdentifiersFactory.h"
#include "core/layout/HitTestResult.h"
#include "core/layout/LayoutImage.h"
#include "core/layout/LayoutObject.h"
#include "core/loader/resource/CSSStyleSheetResource.h"
#include "core/page/Page.h"
#include "core/paint/PaintLayer.h"
#include "core/probe/CoreProbes.h"
#include "core/workers/WorkerGlobalScope.h"
#include "core/workers/WorkerThread.h"
#include "core/xmlhttprequest/XMLHttpRequest.h"
#include "platform/InstanceCounters.h"
#include "platform/graphics/GraphicsLayer.h"
#include "platform/instrumentation/tracing/TracedValue.h"
#include "platform/loader/fetch/ResourceLoadPriority.h"
#include "platform/loader/fetch/ResourceRequest.h"
#include "platform/loader/fetch/ResourceResponse.h"
#include "platform/weborigin/KURL.h"
#include "v8/include/v8-profiler.h"
#include "v8/include/v8.h"
#include "wtf/Vector.h"
#include "wtf/text/TextPosition.h"

namespace blink {

namespace {

void* AsyncId(unsigned long identifier) {
  return reinterpret_cast<void*>((identifier << 1) | 1);
}

std::unique_ptr<TracedValue> InspectorParseHtmlBeginData(Document* document,
                                                         unsigned start_line) {
  std::unique_ptr<TracedValue> value = TracedValue::Create();
  value->SetInteger("startLine", start_line);
  value->SetString("frame", ToHexString(document->GetFrame()));
  value->SetString("url", document->Url().GetString());
  SetCallStack(value.get());
  return value;
}

std::unique_ptr<TracedValue> InspectorParseHtmlEndData(unsigned end_line) {
  std::unique_ptr<TracedValue> value = TracedValue::Create();
  value->SetInteger("endLine", end_line);
  return value;
}

}  //  namespace

String ToHexString(const void* p) {
  return String::Format("0x%" PRIx64,
                        static_cast<uint64_t>(reinterpret_cast<uintptr_t>(p)));
}

void SetCallStack(TracedValue* value) {
  static const unsigned char* trace_category_enabled = 0;
  WTF_ANNOTATE_BENIGN_RACE(&trace_category_enabled, "trace_event category");
  if (!trace_category_enabled)
    trace_category_enabled = TRACE_EVENT_API_GET_CATEGORY_GROUP_ENABLED(
        TRACE_DISABLED_BY_DEFAULT("devtools.timeline.stack"));
  if (!*trace_category_enabled)
    return;
  // The CPU profiler stack trace does not include call site line numbers.
  // So we collect the top frame with SourceLocation::capture() to get the
  // binding call site info.
  SourceLocation::Capture()->ToTracedValue(value, "stackTrace");
  v8::Isolate::GetCurrent()->GetCpuProfiler()->CollectSample();
}

void InspectorTraceEvents::Init(CoreProbeSink* instrumenting_agents,
                                protocol::UberDispatcher*,
                                protocol::DictionaryValue*) {
  instrumenting_agents_ = instrumenting_agents;
  instrumenting_agents_->addInspectorTraceEvents(this);
}

void InspectorTraceEvents::Dispose() {
  instrumenting_agents_->removeInspectorTraceEvents(this);
  instrumenting_agents_ = nullptr;
}

DEFINE_TRACE(InspectorTraceEvents) {
  visitor->Trace(instrumenting_agents_);
  InspectorAgent::Trace(visitor);
}

void InspectorTraceEvents::WillSendRequest(
    LocalFrame* frame,
    unsigned long identifier,
    DocumentLoader*,
    ResourceRequest& request,
    const ResourceResponse& redirect_response,
    const FetchInitiatorInfo&) {
  TRACE_EVENT_INSTANT1(
      "devtools.timeline", "ResourceSendRequest", TRACE_EVENT_SCOPE_THREAD,
      "data", InspectorSendRequestEvent::Data(identifier, frame, request));
  probe::AsyncTaskScheduled(frame->GetDocument(), "SendRequest",
                            AsyncId(identifier));
}

void InspectorTraceEvents::DidReceiveResourceResponse(
    LocalFrame* frame,
    unsigned long identifier,
    DocumentLoader*,
    const ResourceResponse& response,
    Resource*) {
  TRACE_EVENT_INSTANT1(
      "devtools.timeline", "ResourceReceiveResponse", TRACE_EVENT_SCOPE_THREAD,
      "data", InspectorReceiveResponseEvent::Data(identifier, frame, response));
  probe::AsyncTask async_task(frame->GetDocument(), AsyncId(identifier),
                             "response");
}

void InspectorTraceEvents::DidReceiveData(LocalFrame* frame,
                                          unsigned long identifier,
                                          const char* data,
                                          int encoded_data_length) {
  TRACE_EVENT_INSTANT1(
      "devtools.timeline", "ResourceReceivedData", TRACE_EVENT_SCOPE_THREAD,
      "data",
      InspectorReceiveDataEvent::Data(identifier, frame, encoded_data_length));
  probe::AsyncTask async_task(frame->GetDocument(), AsyncId(identifier), "data");
}

void InspectorTraceEvents::DidFinishLoading(LocalFrame* frame,
                                            unsigned long identifier,
                                            double finish_time,
                                            int64_t encoded_data_length,
                                            int64_t decoded_body_length) {
  TRACE_EVENT_INSTANT1(
      "devtools.timeline", "ResourceFinish", TRACE_EVENT_SCOPE_THREAD, "data",
      InspectorResourceFinishEvent::Data(identifier, finish_time, false,
                                         encoded_data_length, decoded_body_length));
  probe::AsyncTask async_task(frame->GetDocument(), AsyncId(identifier));
}

void InspectorTraceEvents::DidFailLoading(unsigned long identifier,
                                          const ResourceError&) {
  TRACE_EVENT_INSTANT1(
      "devtools.timeline", "ResourceFinish", TRACE_EVENT_SCOPE_THREAD, "data",
      InspectorResourceFinishEvent::Data(identifier, 0, true, 0, 0));
}

void InspectorTraceEvents::Will(const probe::ExecuteScript&) {}

void InspectorTraceEvents::Did(const probe::ExecuteScript&) {
  TRACE_EVENT_INSTANT1(TRACE_DISABLED_BY_DEFAULT("devtools.timeline"),
                       "UpdateCounters", TRACE_EVENT_SCOPE_THREAD, "data",
                       InspectorUpdateCountersEvent::Data());
}

void InspectorTraceEvents::Will(const probe::ParseHTML& probe) {
  // FIXME: Pass in current input length.
  TRACE_EVENT_BEGIN1(
      "devtools.timeline", "ParseHTML", "beginData",
      InspectorParseHtmlBeginData(probe.parser->GetDocument(),
                                  probe.parser->LineNumber().ZeroBasedInt()));
}

void InspectorTraceEvents::Did(const probe::ParseHTML& probe) {
  TRACE_EVENT_END1(
      "devtools.timeline", "ParseHTML", "endData",
      InspectorParseHtmlEndData(probe.parser->LineNumber().ZeroBasedInt() - 1));
  TRACE_EVENT_INSTANT1(TRACE_DISABLED_BY_DEFAULT("devtools.timeline"),
                       "UpdateCounters", TRACE_EVENT_SCOPE_THREAD, "data",
                       InspectorUpdateCountersEvent::Data());
}

void InspectorTraceEvents::Will(const probe::CallFunction& probe) {
  if (probe.depth)
    return;
  TRACE_EVENT_BEGIN1(
      "devtools.timeline", "FunctionCall", "data",
      InspectorFunctionCallEvent::Data(probe.context, probe.function));
}

void InspectorTraceEvents::Did(const probe::CallFunction& probe) {
  if (probe.depth)
    return;
  TRACE_EVENT_END0("devtools.timeline", "FunctionCall");
  TRACE_EVENT_INSTANT1(TRACE_DISABLED_BY_DEFAULT("devtools.timeline"),
                       "UpdateCounters", TRACE_EVENT_SCOPE_THREAD, "data",
                       InspectorUpdateCountersEvent::Data());
}

namespace {

void SetNodeInfo(TracedValue* value,
                 Node* node,
                 const char* id_field_name,
                 const char* name_field_name = nullptr) {
  value->SetInteger(id_field_name, DOMNodeIds::IdForNode(node));
  if (name_field_name)
    value->SetString(name_field_name, node->DebugName());
}

const char* PseudoTypeToString(CSSSelector::PseudoType pseudo_type) {
  switch (pseudo_type) {
#define DEFINE_STRING_MAPPING(pseudoType) \
  case CSSSelector::pseudoType:           \
    return #pseudoType;
    DEFINE_STRING_MAPPING(kPseudoUnknown)
    DEFINE_STRING_MAPPING(kPseudoEmpty)
    DEFINE_STRING_MAPPING(kPseudoFirstChild)
    DEFINE_STRING_MAPPING(kPseudoFirstOfType)
    DEFINE_STRING_MAPPING(kPseudoLastChild)
    DEFINE_STRING_MAPPING(kPseudoLastOfType)
    DEFINE_STRING_MAPPING(kPseudoOnlyChild)
    DEFINE_STRING_MAPPING(kPseudoOnlyOfType)
    DEFINE_STRING_MAPPING(kPseudoFirstLine)
    DEFINE_STRING_MAPPING(kPseudoFirstLetter)
    DEFINE_STRING_MAPPING(kPseudoNthChild)
    DEFINE_STRING_MAPPING(kPseudoNthOfType)
    DEFINE_STRING_MAPPING(kPseudoNthLastChild)
    DEFINE_STRING_MAPPING(kPseudoNthLastOfType)
    DEFINE_STRING_MAPPING(kPseudoLink)
    DEFINE_STRING_MAPPING(kPseudoVisited)
    DEFINE_STRING_MAPPING(kPseudoAny)
    DEFINE_STRING_MAPPING(kPseudoAnyLink)
    DEFINE_STRING_MAPPING(kPseudoAutofill)
    DEFINE_STRING_MAPPING(kPseudoHover)
    DEFINE_STRING_MAPPING(kPseudoDrag)
    DEFINE_STRING_MAPPING(kPseudoFocus)
    DEFINE_STRING_MAPPING(kPseudoActive)
    DEFINE_STRING_MAPPING(kPseudoChecked)
    DEFINE_STRING_MAPPING(kPseudoEnabled)
    DEFINE_STRING_MAPPING(kPseudoFullPageMedia)
    DEFINE_STRING_MAPPING(kPseudoDefault)
    DEFINE_STRING_MAPPING(kPseudoDisabled)
    DEFINE_STRING_MAPPING(kPseudoOptional)
    DEFINE_STRING_MAPPING(kPseudoPlaceholderShown)
    DEFINE_STRING_MAPPING(kPseudoRequired)
    DEFINE_STRING_MAPPING(kPseudoReadOnly)
    DEFINE_STRING_MAPPING(kPseudoReadWrite)
    DEFINE_STRING_MAPPING(kPseudoValid)
    DEFINE_STRING_MAPPING(kPseudoInvalid)
    DEFINE_STRING_MAPPING(kPseudoIndeterminate)
    DEFINE_STRING_MAPPING(kPseudoTarget)
    DEFINE_STRING_MAPPING(kPseudoBefore)
    DEFINE_STRING_MAPPING(kPseudoAfter)
    DEFINE_STRING_MAPPING(kPseudoBackdrop)
    DEFINE_STRING_MAPPING(kPseudoLang)
    DEFINE_STRING_MAPPING(kPseudoNot)
    DEFINE_STRING_MAPPING(kPseudoPlaceholder)
    DEFINE_STRING_MAPPING(kPseudoResizer)
    DEFINE_STRING_MAPPING(kPseudoRoot)
    DEFINE_STRING_MAPPING(kPseudoScope)
    DEFINE_STRING_MAPPING(kPseudoScrollbar)
    DEFINE_STRING_MAPPING(kPseudoScrollbarButton)
    DEFINE_STRING_MAPPING(kPseudoScrollbarCorner)
    DEFINE_STRING_MAPPING(kPseudoScrollbarThumb)
    DEFINE_STRING_MAPPING(kPseudoScrollbarTrack)
    DEFINE_STRING_MAPPING(kPseudoScrollbarTrackPiece)
    DEFINE_STRING_MAPPING(kPseudoWindowInactive)
    DEFINE_STRING_MAPPING(kPseudoCornerPresent)
    DEFINE_STRING_MAPPING(kPseudoDecrement)
    DEFINE_STRING_MAPPING(kPseudoIncrement)
    DEFINE_STRING_MAPPING(kPseudoHorizontal)
    DEFINE_STRING_MAPPING(kPseudoVertical)
    DEFINE_STRING_MAPPING(kPseudoStart)
    DEFINE_STRING_MAPPING(kPseudoEnd)
    DEFINE_STRING_MAPPING(kPseudoDoubleButton)
    DEFINE_STRING_MAPPING(kPseudoSingleButton)
    DEFINE_STRING_MAPPING(kPseudoNoButton)
    DEFINE_STRING_MAPPING(kPseudoSelection)
    DEFINE_STRING_MAPPING(kPseudoLeftPage)
    DEFINE_STRING_MAPPING(kPseudoRightPage)
    DEFINE_STRING_MAPPING(kPseudoFirstPage)
    DEFINE_STRING_MAPPING(kPseudoFullScreen)
    DEFINE_STRING_MAPPING(kPseudoFullScreenAncestor)
    DEFINE_STRING_MAPPING(kPseudoInRange)
    DEFINE_STRING_MAPPING(kPseudoOutOfRange)
    DEFINE_STRING_MAPPING(kPseudoWebKitCustomElement)
    DEFINE_STRING_MAPPING(kPseudoBlinkInternalElement)
    DEFINE_STRING_MAPPING(kPseudoCue)
    DEFINE_STRING_MAPPING(kPseudoFutureCue)
    DEFINE_STRING_MAPPING(kPseudoPastCue)
    DEFINE_STRING_MAPPING(kPseudoUnresolved)
    DEFINE_STRING_MAPPING(kPseudoDefined)
    DEFINE_STRING_MAPPING(kPseudoContent)
    DEFINE_STRING_MAPPING(kPseudoHost)
    DEFINE_STRING_MAPPING(kPseudoHostContext)
    DEFINE_STRING_MAPPING(kPseudoShadow)
    DEFINE_STRING_MAPPING(kPseudoSlotted)
    DEFINE_STRING_MAPPING(kPseudoSpatialNavigationFocus)
    DEFINE_STRING_MAPPING(kPseudoListBox)
    DEFINE_STRING_MAPPING(kPseudoHostHasAppearance)
    DEFINE_STRING_MAPPING(kPseudoVideoPersistent)
    DEFINE_STRING_MAPPING(kPseudoVideoPersistentAncestor)
#undef DEFINE_STRING_MAPPING
  }

  ASSERT_NOT_REACHED();
  return "";
}

String UrlForFrame(LocalFrame* frame) {
  KURL url = frame->GetDocument()->Url();
  url.RemoveFragmentIdentifier();
  return url.GetString();
}

}  // namespace

namespace InspectorScheduleStyleInvalidationTrackingEvent {
std::unique_ptr<TracedValue> FillCommonPart(
    ContainerNode& node,
    const InvalidationSet& invalidation_set,
    const char* invalidated_selector) {
  std::unique_ptr<TracedValue> value = TracedValue::Create();
  value->SetString("frame", ToHexString(node.GetDocument().GetFrame()));
  SetNodeInfo(value.get(), &node, "nodeId", "nodeName");
  value->SetString("invalidationSet",
                   DescendantInvalidationSetToIdString(invalidation_set));
  value->SetString("invalidatedSelectorId", invalidated_selector);
  SourceLocation::Capture()->ToTracedValue(value.get(), "stackTrace");
  return value;
}
}  // namespace InspectorScheduleStyleInvalidationTrackingEvent

const char InspectorScheduleStyleInvalidationTrackingEvent::kAttribute[] =
    "attribute";
const char InspectorScheduleStyleInvalidationTrackingEvent::kClass[] = "class";
const char InspectorScheduleStyleInvalidationTrackingEvent::kId[] = "id";
const char InspectorScheduleStyleInvalidationTrackingEvent::kPseudo[] = "pseudo";
const char InspectorScheduleStyleInvalidationTrackingEvent::kRuleSet[] =
    "ruleset";

const char* ResourcePriorityString(ResourceLoadPriority priority) {
  const char* priority_string = 0;
  switch (priority) {
    case kResourceLoadPriorityVeryLow:
      priority_string = "VeryLow";
      break;
    case kResourceLoadPriorityLow:
      priority_string = "Low";
      break;
    case kResourceLoadPriorityMedium:
      priority_string = "Medium";
      break;
    case kResourceLoadPriorityHigh:
      priority_string = "High";
      break;
    case kResourceLoadPriorityVeryHigh:
      priority_string = "VeryHigh";
      break;
    case kResourceLoadPriorityUnresolved:
      break;
  }
  return priority_string;
}

std::unique_ptr<TracedValue>
InspectorScheduleStyleInvalidationTrackingEvent::IdChange(
    Element& element,
    const InvalidationSet& invalidation_set,
    const AtomicString& id) {
  std::unique_ptr<TracedValue> value =
      FillCommonPart(element, invalidation_set, kId);
  value->SetString("changedId", id);
  return value;
}

std::unique_ptr<TracedValue>
InspectorScheduleStyleInvalidationTrackingEvent::ClassChange(
    Element& element,
    const InvalidationSet& invalidation_set,
    const AtomicString& class_name) {
  std::unique_ptr<TracedValue> value =
      FillCommonPart(element, invalidation_set, kClass);
  value->SetString("changedClass", class_name);
  return value;
}

std::unique_ptr<TracedValue>
InspectorScheduleStyleInvalidationTrackingEvent::AttributeChange(
    Element& element,
    const InvalidationSet& invalidation_set,
    const QualifiedName& attribute_name) {
  std::unique_ptr<TracedValue> value =
      FillCommonPart(element, invalidation_set, kAttribute);
  value->SetString("changedAttribute", attribute_name.ToString());
  return value;
}

std::unique_ptr<TracedValue>
InspectorScheduleStyleInvalidationTrackingEvent::PseudoChange(
    Element& element,
    const InvalidationSet& invalidation_set,
    CSSSelector::PseudoType pseudo_type) {
  std::unique_ptr<TracedValue> value =
      FillCommonPart(element, invalidation_set, kAttribute);
  value->SetString("changedPseudo", PseudoTypeToString(pseudo_type));
  return value;
}

std::unique_ptr<TracedValue>
InspectorScheduleStyleInvalidationTrackingEvent::RuleSetInvalidation(
    ContainerNode& root_node,
    const InvalidationSet& invalidation_set) {
  std::unique_ptr<TracedValue> value =
      FillCommonPart(root_node, invalidation_set, kRuleSet);
  return value;
}

String DescendantInvalidationSetToIdString(const InvalidationSet& set) {
  return ToHexString(&set);
}

const char InspectorStyleInvalidatorInvalidateEvent::
    kElementHasPendingInvalidationList[] =
        "Element has pending invalidation list";
const char InspectorStyleInvalidatorInvalidateEvent::kInvalidateCustomPseudo[] =
    "Invalidate custom pseudo element";
const char InspectorStyleInvalidatorInvalidateEvent::
    kInvalidationSetMatchedAttribute[] = "Invalidation set matched attribute";
const char
    InspectorStyleInvalidatorInvalidateEvent::kInvalidationSetMatchedClass[] =
        "Invalidation set matched class";
const char
    InspectorStyleInvalidatorInvalidateEvent::kInvalidationSetMatchedId[] =
        "Invalidation set matched id";
const char
    InspectorStyleInvalidatorInvalidateEvent::kInvalidationSetMatchedTagName[] =
        "Invalidation set matched tagName";
const char
    InspectorStyleInvalidatorInvalidateEvent::kPreventStyleSharingForParent[] =
        "Prevent style sharing for parent";

namespace InspectorStyleInvalidatorInvalidateEvent {
std::unique_ptr<TracedValue> FillCommonPart(ContainerNode& node,
                                            const char* reason) {
  std::unique_ptr<TracedValue> value = TracedValue::Create();
  value->SetString("frame", ToHexString(node.GetDocument().GetFrame()));
  SetNodeInfo(value.get(), &node, "nodeId", "nodeName");
  value->SetString("reason", reason);
  return value;
}
}  // namespace InspectorStyleInvalidatorInvalidateEvent

std::unique_ptr<TracedValue> InspectorStyleInvalidatorInvalidateEvent::Data(
    Element& element,
    const char* reason) {
  return FillCommonPart(element, reason);
}

std::unique_ptr<TracedValue>
InspectorStyleInvalidatorInvalidateEvent::SelectorPart(
    Element& element,
    const char* reason,
    const InvalidationSet& invalidation_set,
    const String& selector_part) {
  std::unique_ptr<TracedValue> value = FillCommonPart(element, reason);
  value->BeginArray("invalidationList");
  invalidation_set.ToTracedValue(value.get());
  value->EndArray();
  value->SetString("selectorPart", selector_part);
  return value;
}

std::unique_ptr<TracedValue>
InspectorStyleInvalidatorInvalidateEvent::InvalidationList(
    ContainerNode& node,
    const Vector<RefPtr<InvalidationSet>>& invalidation_list) {
  std::unique_ptr<TracedValue> value =
      FillCommonPart(node, kElementHasPendingInvalidationList);
  value->BeginArray("invalidationList");
  for (const auto& invalidation_set : invalidation_list)
    invalidation_set->ToTracedValue(value.get());
  value->EndArray();
  return value;
}

std::unique_ptr<TracedValue>
InspectorStyleRecalcInvalidationTrackingEvent::Data(
    Node* node,
    const StyleChangeReasonForTracing& reason) {
  ASSERT(node);

  std::unique_ptr<TracedValue> value = TracedValue::Create();
  value->SetString("frame", ToHexString(node->GetDocument().GetFrame()));
  SetNodeInfo(value.get(), node, "nodeId", "nodeName");
  value->SetString("reason", reason.ReasonString());
  value->SetString("extraData", reason.GetExtraData());
  SourceLocation::Capture()->ToTracedValue(value.get(), "stackTrace");
  return value;
}

std::unique_ptr<TracedValue> InspectorLayoutEvent::BeginData(
    FrameView* frame_view) {
  bool is_partial;
  unsigned needs_layout_objects;
  unsigned total_objects;
  LocalFrame& frame = frame_view->GetFrame();
  frame.View()->CountObjectsNeedingLayout(needs_layout_objects, total_objects,
                                          is_partial);

  std::unique_ptr<TracedValue> value = TracedValue::Create();
  value->SetInteger("dirtyObjects", needs_layout_objects);
  value->SetInteger("totalObjects", total_objects);
  value->SetBoolean("partialLayout", is_partial);
  value->SetString("frame", ToHexString(&frame));
  SetCallStack(value.get());
  return value;
}

static void CreateQuad(TracedValue* value,
                       const char* name,
                       const FloatQuad& quad) {
  value->BeginArray(name);
  value->PushDouble(quad.P1().X());
  value->PushDouble(quad.P1().Y());
  value->PushDouble(quad.P2().X());
  value->PushDouble(quad.P2().Y());
  value->PushDouble(quad.P3().X());
  value->PushDouble(quad.P3().Y());
  value->PushDouble(quad.P4().X());
  value->PushDouble(quad.P4().Y());
  value->EndArray();
}

static void SetGeneratingNodeInfo(TracedValue* value,
                                  const LayoutObject* layout_object,
                                  const char* id_field_name,
                                  const char* name_field_name = nullptr) {
  Node* node = nullptr;
  for (; layout_object && !node; layout_object = layout_object->Parent())
    node = layout_object->GeneratingNode();
  if (!node)
    return;

  SetNodeInfo(value, node, id_field_name, name_field_name);
}

std::unique_ptr<TracedValue> InspectorLayoutEvent::EndData(
    LayoutObject* root_for_this_layout) {
  Vector<FloatQuad> quads;
  root_for_this_layout->AbsoluteQuads(quads);

  std::unique_ptr<TracedValue> value = TracedValue::Create();
  if (quads.size() >= 1) {
    CreateQuad(value.get(), "root", quads[0]);
    SetGeneratingNodeInfo(value.get(), root_for_this_layout, "rootNode");
  } else {
    ASSERT_NOT_REACHED();
  }
  return value;
}

namespace LayoutInvalidationReason {
const char kUnknown[] = "Unknown";
const char kSizeChanged[] = "Size changed";
const char kAncestorMoved[] = "Ancestor moved";
const char kStyleChange[] = "Style changed";
const char kDomChanged[] = "DOM changed";
const char kTextChanged[] = "Text changed";
const char kPrintingChanged[] = "Printing changed";
const char kAttributeChanged[] = "Attribute changed";
const char kColumnsChanged[] = "Attribute changed";
const char kChildAnonymousBlockChanged[] = "Child anonymous block changed";
const char kAnonymousBlockChange[] = "Anonymous block change";
const char kFullscreen[] = "Fullscreen change";
const char kChildChanged[] = "Child changed";
const char kListValueChange[] = "List value change";
const char kImageChanged[] = "Image changed";
const char kLineBoxesChanged[] = "Line boxes changed";
const char kSliderValueChanged[] = "Slider value changed";
const char kAncestorMarginCollapsing[] = "Ancestor margin collapsing";
const char kFieldsetChanged[] = "Fieldset changed";
const char kTextAutosizing[] = "Text autosizing (font boosting)";
const char kSvgResourceInvalidated[] = "SVG resource invalidated";
const char kFloatDescendantChanged[] = "Floating descendant changed";
const char kCountersChanged[] = "Counters changed";
const char kGridChanged[] = "Grid changed";
const char kMenuOptionsChanged[] = "Menu options changed";
const char kRemovedFromLayout[] = "Removed from layout";
const char kAddedToLayout[] = "Added to layout";
const char kTableChanged[] = "Table changed";
const char kPaddingChanged[] = "Padding changed";
const char kTextControlChanged[] = "Text control changed";
const char kSvgChanged[] = "SVG changed";
const char kScrollbarChanged[] = "Scrollbar changed";
}  // namespace LayoutInvalidationReason

std::unique_ptr<TracedValue> InspectorLayoutInvalidationTrackingEvent::Data(
    const LayoutObject* layout_object,
    LayoutInvalidationReasonForTracing reason) {
  ASSERT(layout_object);
  std::unique_ptr<TracedValue> value = TracedValue::Create();
  value->SetString("frame", ToHexString(layout_object->GetFrame()));
  SetGeneratingNodeInfo(value.get(), layout_object, "nodeId", "nodeName");
  value->SetString("reason", reason);
  SourceLocation::Capture()->ToTracedValue(value.get(), "stackTrace");
  return value;
}

std::unique_ptr<TracedValue> InspectorPaintInvalidationTrackingEvent::Data(
    const LayoutObject* layout_object,
    const LayoutObject& paint_container) {
  ASSERT(layout_object);
  std::unique_ptr<TracedValue> value = TracedValue::Create();
  value->SetString("frame", ToHexString(layout_object->GetFrame()));
  SetGeneratingNodeInfo(value.get(), &paint_container, "paintId");
  SetGeneratingNodeInfo(value.get(), layout_object, "nodeId", "nodeName");
  return value;
}

std::unique_ptr<TracedValue> InspectorScrollInvalidationTrackingEvent::Data(
    const LayoutObject& layout_object) {
  static const char kScrollInvalidationReason[] =
      "Scroll with viewport-constrained element";

  std::unique_ptr<TracedValue> value = TracedValue::Create();
  value->SetString("frame", ToHexString(layout_object.GetFrame()));
  value->SetString("reason", kScrollInvalidationReason);
  SetGeneratingNodeInfo(value.get(), &layout_object, "nodeId", "nodeName");
  SourceLocation::Capture()->ToTracedValue(value.get(), "stackTrace");
  return value;
}

std::unique_ptr<TracedValue> InspectorChangeResourcePriorityEvent::Data(
    unsigned long identifier,
    const ResourceLoadPriority& load_priority) {
  String request_id = IdentifiersFactory::RequestId(identifier);

  std::unique_ptr<TracedValue> value = TracedValue::Create();
  value->SetString("requestId", request_id);
  value->SetString("priority", ResourcePriorityString(load_priority));
  return value;
}

std::unique_ptr<TracedValue> InspectorSendRequestEvent::Data(
    unsigned long identifier,
    LocalFrame* frame,
    const ResourceRequest& request) {
  String request_id = IdentifiersFactory::RequestId(identifier);

  std::unique_ptr<TracedValue> value = TracedValue::Create();
  value->SetString("requestId", request_id);
  value->SetString("frame", ToHexString(frame));
  value->SetString("url", request.Url().GetString());
  value->SetString("requestMethod", request.HttpMethod());
  const char* priority = ResourcePriorityString(request.Priority());
  if (priority)
    value->SetString("priority", priority);
  SetCallStack(value.get());
  return value;
}

namespace {
void RecordTiming(const ResourceLoadTiming& timing, TracedValue* value) {
  value->SetDouble("requestTime", timing.RequestTime());
  value->SetDouble("proxyStart",
                   timing.CalculateMillisecondDelta(timing.ProxyStart()));
  value->SetDouble("proxyEnd",
                   timing.CalculateMillisecondDelta(timing.ProxyEnd()));
  value->SetDouble("dnsStart",
                   timing.CalculateMillisecondDelta(timing.DnsStart()));
  value->SetDouble("dnsEnd", timing.CalculateMillisecondDelta(timing.DnsEnd()));
  value->SetDouble("connectStart",
                   timing.CalculateMillisecondDelta(timing.ConnectStart()));
  value->SetDouble("connectEnd",
                   timing.CalculateMillisecondDelta(timing.ConnectEnd()));
  value->SetDouble("sslStart",
                   timing.CalculateMillisecondDelta(timing.SslStart()));
  value->SetDouble("sslEnd", timing.CalculateMillisecondDelta(timing.SslEnd()));
  value->SetDouble("workerStart",
                   timing.CalculateMillisecondDelta(timing.WorkerStart()));
  value->SetDouble("workerReady",
                   timing.CalculateMillisecondDelta(timing.WorkerReady()));
  value->SetDouble("sendStart",
                   timing.CalculateMillisecondDelta(timing.SendStart()));
  value->SetDouble("sendEnd",
                   timing.CalculateMillisecondDelta(timing.SendEnd()));
  value->SetDouble("receiveHeadersEnd", timing.CalculateMillisecondDelta(
                                            timing.ReceiveHeadersEnd()));
  value->SetDouble("pushStart", timing.PushStart());
  value->SetDouble("pushEnd", timing.PushEnd());
}
}  // namespace

std::unique_ptr<TracedValue> InspectorReceiveResponseEvent::Data(
    unsigned long identifier,
    LocalFrame* frame,
    const ResourceResponse& response) {
  String request_id = IdentifiersFactory::RequestId(identifier);

  std::unique_ptr<TracedValue> value = TracedValue::Create();
  value->SetString("requestId", request_id);
  value->SetString("frame", ToHexString(frame));
  value->SetInteger("statusCode", response.HttpStatusCode());
  value->SetString("mimeType", response.MimeType().GetString().IsolatedCopy());
  value->SetDouble("encodedDataLength", response.EncodedDataLength());
  value->SetBoolean("fromCache", response.WasCached());
  value->SetBoolean("fromServiceWorker", response.WasFetchedViaServiceWorker());
  if (response.GetResourceLoadTiming()) {
    value->BeginDictionary("timing");
    RecordTiming(*response.GetResourceLoadTiming(), value.get());
    value->EndDictionary();
  }
  if (response.WasFetchedViaServiceWorker())
    value->SetBoolean("fromServiceWorker", true);
  return value;
}

std::unique_ptr<TracedValue> InspectorReceiveDataEvent::Data(
    unsigned long identifier,
    LocalFrame* frame,
    int encoded_data_length) {
  String request_id = IdentifiersFactory::RequestId(identifier);

  std::unique_ptr<TracedValue> value = TracedValue::Create();
  value->SetString("requestId", request_id);
  value->SetString("frame", ToHexString(frame));
  value->SetInteger("encodedDataLength", encoded_data_length);
  return value;
}

std::unique_ptr<TracedValue> InspectorResourceFinishEvent::Data(
    unsigned long identifier,
    double finish_time,
    bool did_fail,
    int64_t encoded_data_length,
    int64_t decoded_body_length) {
  String request_id = IdentifiersFactory::RequestId(identifier);

  std::unique_ptr<TracedValue> value = TracedValue::Create();
  value->SetString("requestId", request_id);
  value->SetBoolean("didFail", did_fail);
  value->SetDouble("encodedDataLength", encoded_data_length);
  value->SetDouble("decodedBodyLength", decoded_body_length);
  if (finish_time)
    value->SetDouble("finishTime", finish_time);
  return value;
}

static LocalFrame* FrameForExecutionContext(ExecutionContext* context) {
  LocalFrame* frame = nullptr;
  if (context->IsDocument())
    frame = ToDocument(context)->GetFrame();
  return frame;
}

static std::unique_ptr<TracedValue> GenericTimerData(ExecutionContext* context,
                                                     int timer_id) {
  std::unique_ptr<TracedValue> value = TracedValue::Create();
  value->SetInteger("timerId", timer_id);
  if (LocalFrame* frame = FrameForExecutionContext(context))
    value->SetString("frame", ToHexString(frame));
  return value;
}

std::unique_ptr<TracedValue> InspectorTimerInstallEvent::Data(
    ExecutionContext* context,
    int timer_id,
    int timeout,
    bool single_shot) {
  std::unique_ptr<TracedValue> value = GenericTimerData(context, timer_id);
  value->SetInteger("timeout", timeout);
  value->SetBoolean("singleShot", single_shot);
  SetCallStack(value.get());
  return value;
}

std::unique_ptr<TracedValue> InspectorTimerRemoveEvent::Data(
    ExecutionContext* context,
    int timer_id) {
  std::unique_ptr<TracedValue> value = GenericTimerData(context, timer_id);
  SetCallStack(value.get());
  return value;
}

std::unique_ptr<TracedValue> InspectorTimerFireEvent::Data(
    ExecutionContext* context,
    int timer_id) {
  return GenericTimerData(context, timer_id);
}

std::unique_ptr<TracedValue> InspectorAnimationFrameEvent::Data(
    ExecutionContext* context,
    int callback_id) {
  std::unique_ptr<TracedValue> value = TracedValue::Create();
  value->SetInteger("id", callback_id);
  if (context->IsDocument())
    value->SetString("frame", ToHexString(ToDocument(context)->GetFrame()));
  else if (context->IsWorkerGlobalScope())
    value->SetString("worker", ToHexString(ToWorkerGlobalScope(context)));
  SetCallStack(value.get());
  return value;
}

std::unique_ptr<TracedValue> GenericIdleCallbackEvent(ExecutionContext* context,
                                                      int id) {
  std::unique_ptr<TracedValue> value = TracedValue::Create();
  value->SetInteger("id", id);
  if (LocalFrame* frame = FrameForExecutionContext(context))
    value->SetString("frame", ToHexString(frame));
  SetCallStack(value.get());
  return value;
}

std::unique_ptr<TracedValue> InspectorIdleCallbackRequestEvent::Data(
    ExecutionContext* context,
    int id,
    double timeout) {
  std::unique_ptr<TracedValue> value = GenericIdleCallbackEvent(context, id);
  value->SetInteger("timeout", timeout);
  return value;
}

std::unique_ptr<TracedValue> InspectorIdleCallbackCancelEvent::Data(
    ExecutionContext* context,
    int id) {
  return GenericIdleCallbackEvent(context, id);
}

std::unique_ptr<TracedValue> InspectorIdleCallbackFireEvent::Data(
    ExecutionContext* context,
    int id,
    double allotted_milliseconds,
    bool timed_out) {
  std::unique_ptr<TracedValue> value = GenericIdleCallbackEvent(context, id);
  value->SetDouble("allottedMilliseconds", allotted_milliseconds);
  value->SetBoolean("timedOut", timed_out);
  return value;
}

std::unique_ptr<TracedValue> InspectorParseAuthorStyleSheetEvent::Data(
    const CSSStyleSheetResource* cached_style_sheet) {
  std::unique_ptr<TracedValue> value = TracedValue::Create();
  value->SetString("styleSheetUrl", cached_style_sheet->Url().GetString());
  return value;
}

std::unique_ptr<TracedValue> InspectorXhrReadyStateChangeEvent::Data(
    ExecutionContext* context,
    XMLHttpRequest* request) {
  std::unique_ptr<TracedValue> value = TracedValue::Create();
  value->SetString("url", request->Url().GetString());
  value->SetInteger("readyState", request->readyState());
  if (LocalFrame* frame = FrameForExecutionContext(context))
    value->SetString("frame", ToHexString(frame));
  SetCallStack(value.get());
  return value;
}

std::unique_ptr<TracedValue> InspectorXhrLoadEvent::Data(
    ExecutionContext* context,
    XMLHttpRequest* request) {
  std::unique_ptr<TracedValue> value = TracedValue::Create();
  value->SetString("url", request->Url().GetString());
  if (LocalFrame* frame = FrameForExecutionContext(context))
    value->SetString("frame", ToHexString(frame));
  SetCallStack(value.get());
  return value;
}

static void LocalToPageQuad(const LayoutObject& layout_object,
                            const LayoutRect& rect,
                            FloatQuad* quad) {
  LocalFrame* frame = layout_object.GetFrame();
  FrameView* view = frame->View();
  FloatQuad absolute =
      layout_object.LocalToAbsoluteQuad(FloatQuad(FloatRect(rect)));
  quad->SetP1(view->ContentsToRootFrame(RoundedIntPoint(absolute.P1())));
  quad->SetP2(view->ContentsToRootFrame(RoundedIntPoint(absolute.P2())));
  quad->SetP3(view->ContentsToRootFrame(RoundedIntPoint(absolute.P3())));
  quad->SetP4(view->ContentsToRootFrame(RoundedIntPoint(absolute.P4())));
}

const char InspectorLayerInvalidationTrackingEvent::
    kSquashingLayerGeometryWasUpdated[] = "Squashing layer geometry was updated";
const char InspectorLayerInvalidationTrackingEvent::kAddedToSquashingLayer[] =
    "The layer may have been added to an already-existing squashing layer";
const char
    InspectorLayerInvalidationTrackingEvent::kRemovedFromSquashingLayer[] =
        "Removed the layer from a squashing layer";
const char InspectorLayerInvalidationTrackingEvent::kReflectionLayerChanged[] =
    "Reflection layer change";
const char InspectorLayerInvalidationTrackingEvent::kNewCompositedLayer[] =
    "Assigned a new composited layer";

std::unique_ptr<TracedValue> InspectorLayerInvalidationTrackingEvent::Data(
    const PaintLayer* layer,
    const char* reason) {
  const LayoutObject& paint_invalidation_container =
      layer->GetLayoutObject().ContainerForPaintInvalidation();

  std::unique_ptr<TracedValue> value = TracedValue::Create();
  value->SetString("frame", ToHexString(paint_invalidation_container.GetFrame()));
  SetGeneratingNodeInfo(value.get(), &paint_invalidation_container, "paintId");
  value->SetString("reason", reason);
  return value;
}

std::unique_ptr<TracedValue> InspectorPaintEvent::Data(
    LayoutObject* layout_object,
    const LayoutRect& clip_rect,
    const GraphicsLayer* graphics_layer) {
  std::unique_ptr<TracedValue> value = TracedValue::Create();
  value->SetString("frame", ToHexString(layout_object->GetFrame()));
  FloatQuad quad;
  LocalToPageQuad(*layout_object, clip_rect, &quad);
  CreateQuad(value.get(), "clip", quad);
  SetGeneratingNodeInfo(value.get(), layout_object, "nodeId");
  int graphics_layer_id =
      graphics_layer ? graphics_layer->PlatformLayer()->Id() : 0;
  value->SetInteger("layerId", graphics_layer_id);
  SetCallStack(value.get());
  return value;
}

std::unique_ptr<TracedValue> FrameEventData(LocalFrame* frame) {
  std::unique_ptr<TracedValue> value = TracedValue::Create();
  bool is_main_frame = frame && frame->IsMainFrame();
  value->SetBoolean("isMainFrame", is_main_frame);
  value->SetString("page", ToHexString(frame->LocalFrameRoot()));
  return value;
}

void FillCommonFrameData(TracedValue* frame_data, LocalFrame* frame) {
  frame_data->SetString("frame", ToHexString(frame));
  frame_data->SetString("url", UrlForFrame(frame));
  frame_data->SetString("name", frame->Tree().GetName());

  FrameOwner* owner = frame->Owner();
  if (owner && owner->IsLocal()) {
    frame_data->SetInteger(
        "nodeId", DOMNodeIds::IdForNode(ToHTMLFrameOwnerElement(owner)));
  }
  Frame* parent = frame->Tree().Parent();
  if (parent && parent->IsLocalFrame())
    frame_data->SetString("parent", ToHexString(parent));
}

std::unique_ptr<TracedValue> InspectorCommitLoadEvent::Data(LocalFrame* frame) {
  std::unique_ptr<TracedValue> frame_data = FrameEventData(frame);
  FillCommonFrameData(frame_data.get(), frame);
  return frame_data;
}

std::unique_ptr<TracedValue> InspectorMarkLoadEvent::Data(LocalFrame* frame) {
  std::unique_ptr<TracedValue> frame_data = FrameEventData(frame);
  frame_data->SetString("frame", ToHexString(frame));
  return frame_data;
}

std::unique_ptr<TracedValue> InspectorScrollLayerEvent::Data(
    LayoutObject* layout_object) {
  std::unique_ptr<TracedValue> value = TracedValue::Create();
  value->SetString("frame", ToHexString(layout_object->GetFrame()));
  SetGeneratingNodeInfo(value.get(), layout_object, "nodeId");
  return value;
}

std::unique_ptr<TracedValue> InspectorUpdateLayerTreeEvent::Data(
    LocalFrame* frame) {
  std::unique_ptr<TracedValue> value = TracedValue::Create();
  value->SetString("frame", ToHexString(frame));
  return value;
}

namespace {
std::unique_ptr<TracedValue> FillLocation(const String& url,
                                          const TextPosition& text_position) {
  std::unique_ptr<TracedValue> value = TracedValue::Create();
  value->SetString("url", url);
  value->SetInteger("lineNumber", text_position.line_.OneBasedInt());
  value->SetInteger("columnNumber", text_position.column_.OneBasedInt());
  return value;
}
}

std::unique_ptr<TracedValue> InspectorEvaluateScriptEvent::Data(
    LocalFrame* frame,
    const String& url,
    const TextPosition& text_position) {
  std::unique_ptr<TracedValue> value = FillLocation(url, text_position);
  value->SetString("frame", ToHexString(frame));
  SetCallStack(value.get());
  return value;
}

std::unique_ptr<TracedValue> InspectorParseScriptEvent::Data(
    unsigned long identifier,
    const String& url) {
  String request_id = IdentifiersFactory::RequestId(identifier);
  std::unique_ptr<TracedValue> value = TracedValue::Create();
  value->SetString("requestId", request_id);
  value->SetString("url", url);
  return value;
}

std::unique_ptr<TracedValue> InspectorCompileScriptEvent::Data(
    const String& url,
    const TextPosition& text_position) {
  return FillLocation(url, text_position);
}

std::unique_ptr<TracedValue> InspectorFunctionCallEvent::Data(
    ExecutionContext* context,
    const v8::Local<v8::Function>& function) {
  std::unique_ptr<TracedValue> value = TracedValue::Create();
  if (LocalFrame* frame = FrameForExecutionContext(context))
    value->SetString("frame", ToHexString(frame));

  if (function.IsEmpty())
    return value;

  v8::Local<v8::Function> original_function = GetBoundFunction(function);
  v8::Local<v8::Value> function_name = original_function->GetDebugName();
  if (!function_name.IsEmpty() && function_name->IsString())
    value->SetString("functionName",
                     ToCoreString(function_name.As<v8::String>()));
  std::unique_ptr<SourceLocation> location =
      SourceLocation::FromFunction(original_function);
  value->SetString("scriptId", String::Number(location->ScriptId()));
  value->SetString("url", location->Url());
  value->SetInteger("lineNumber", location->LineNumber());
  return value;
}

std::unique_ptr<TracedValue> InspectorPaintImageEvent::Data(
    const LayoutImage& layout_image) {
  std::unique_ptr<TracedValue> value = TracedValue::Create();
  SetGeneratingNodeInfo(value.get(), &layout_image, "nodeId");
  if (const ImageResourceContent* resource = layout_image.CachedImage())
    value->SetString("url", resource->Url().GetString());
  return value;
}

std::unique_ptr<TracedValue> InspectorPaintImageEvent::Data(
    const LayoutObject& owning_layout_object,
    const StyleImage& style_image) {
  std::unique_ptr<TracedValue> value = TracedValue::Create();
  SetGeneratingNodeInfo(value.get(), &owning_layout_object, "nodeId");
  if (const ImageResourceContent* resource = style_image.CachedImage())
    value->SetString("url", resource->Url().GetString());
  return value;
}

std::unique_ptr<TracedValue> InspectorPaintImageEvent::Data(
    const LayoutObject* owning_layout_object,
    const ImageResourceContent& image_resource) {
  std::unique_ptr<TracedValue> value = TracedValue::Create();
  SetGeneratingNodeInfo(value.get(), owning_layout_object, "nodeId");
  value->SetString("url", image_resource.Url().GetString());
  return value;
}

static size_t UsedHeapSize() {
  v8::HeapStatistics heap_statistics;
  v8::Isolate::GetCurrent()->GetHeapStatistics(&heap_statistics);
  return heap_statistics.used_heap_size();
}

std::unique_ptr<TracedValue> InspectorUpdateCountersEvent::Data() {
  std::unique_ptr<TracedValue> value = TracedValue::Create();
  if (IsMainThread()) {
    value->SetInteger("documents", InstanceCounters::CounterValue(
                                       InstanceCounters::kDocumentCounter));
    value->SetInteger(
        "nodes", InstanceCounters::CounterValue(InstanceCounters::kNodeCounter));
    value->SetInteger("jsEventListeners",
                      InstanceCounters::CounterValue(
                          InstanceCounters::kJSEventListenerCounter));
  }
  value->SetDouble("jsHeapSizeUsed", static_cast<double>(UsedHeapSize()));
  return value;
}

std::unique_ptr<TracedValue> InspectorInvalidateLayoutEvent::Data(
    LocalFrame* frame) {
  std::unique_ptr<TracedValue> value = TracedValue::Create();
  value->SetString("frame", ToHexString(frame));
  SetCallStack(value.get());
  return value;
}

std::unique_ptr<TracedValue> InspectorRecalculateStylesEvent::Data(
    LocalFrame* frame) {
  std::unique_ptr<TracedValue> value = TracedValue::Create();
  value->SetString("frame", ToHexString(frame));
  SetCallStack(value.get());
  return value;
}

std::unique_ptr<TracedValue> InspectorEventDispatchEvent::Data(
    const Event& event) {
  std::unique_ptr<TracedValue> value = TracedValue::Create();
  value->SetString("type", event.type());
  SetCallStack(value.get());
  return value;
}

std::unique_ptr<TracedValue> InspectorTimeStampEvent::Data(
    ExecutionContext* context,
    const String& message) {
  std::unique_ptr<TracedValue> value = TracedValue::Create();
  value->SetString("message", message);
  if (LocalFrame* frame = FrameForExecutionContext(context))
    value->SetString("frame", ToHexString(frame));
  return value;
}

std::unique_ptr<TracedValue> InspectorTracingSessionIdForWorkerEvent::Data(
    const String& session_id,
    const String& worker_id,
    WorkerThread* worker_thread) {
  std::unique_ptr<TracedValue> value = TracedValue::Create();
  value->SetString("sessionId", session_id);
  value->SetString("workerId", worker_id);
  value->SetDouble("workerThreadId", worker_thread->GetPlatformThreadId());
  return value;
}

std::unique_ptr<TracedValue> InspectorTracingStartedInFrame::Data(
    const String& session_id,
    LocalFrame* frame) {
  std::unique_ptr<TracedValue> value = TracedValue::Create();
  value->SetString("sessionId", session_id);
  value->SetString("page", ToHexString(frame->LocalFrameRoot()));
  value->BeginArray("frames");
  for (Frame* f = frame; f; f = f->Tree().TraverseNext(frame)) {
    if (!f->IsLocalFrame())
      continue;
    value->BeginDictionary();
    FillCommonFrameData(value.get(), ToLocalFrame(f));
    value->EndDictionary();
  }
  value->EndArray();
  return value;
}

std::unique_ptr<TracedValue> InspectorSetLayerTreeId::Data(
    const String& session_id,
    int layer_tree_id) {
  std::unique_ptr<TracedValue> value = TracedValue::Create();
  value->SetString("sessionId", session_id);
  value->SetInteger("layerTreeId", layer_tree_id);
  return value;
}

std::unique_ptr<TracedValue> InspectorAnimationEvent::Data(
    const Animation& animation) {
  std::unique_ptr<TracedValue> value = TracedValue::Create();
  value->SetString("id", String::Number(animation.SequenceNumber()));
  value->SetString("state", animation.playState());
  if (const AnimationEffectReadOnly* effect = animation.effect()) {
    value->SetString("name", animation.id());
    if (effect->IsKeyframeEffectReadOnly()) {
      if (Element* target = ToKeyframeEffectReadOnly(effect)->Target())
        SetNodeInfo(value.get(), target, "nodeId", "nodeName");
    }
  }
  return value;
}

std::unique_ptr<TracedValue> InspectorAnimationStateEvent::Data(
    const Animation& animation) {
  std::unique_ptr<TracedValue> value = TracedValue::Create();
  value->SetString("state", animation.playState());
  return value;
}

std::unique_ptr<TracedValue> InspectorHitTestEvent::EndData(
    const HitTestRequest& request,
    const HitTestLocation& location,
    const HitTestResult& result) {
  std::unique_ptr<TracedValue> value(TracedValue::Create());
  value->SetInteger("x", location.RoundedPoint().X());
  value->SetInteger("y", location.RoundedPoint().Y());
  if (location.IsRectBasedTest())
    value->SetBoolean("rect", true);
  if (location.IsRectilinear())
    value->SetBoolean("rectilinear", true);
  if (request.TouchEvent())
    value->SetBoolean("touch", true);
  if (request.Move())
    value->SetBoolean("move", true);
  if (request.ListBased())
    value->SetBoolean("listBased", true);
  else if (Node* node = result.InnerNode())
    SetNodeInfo(value.get(), node, "nodeId", "nodeName");
  return value;
}

std::unique_ptr<TracedValue> InspectorAsyncTask::Data(const String& name) {
  std::unique_ptr<TracedValue> value = TracedValue::Create();
  value->SetString("name", name);
  return value;
}

}  // namespace blink
