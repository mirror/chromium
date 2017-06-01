/*
 * Copyright (C) 1999 Lars Knoll (knoll@kde.org)
 *           (C) 1999 Antti Koivisto (koivisto@kde.org)
 *           (C) 2001 Dirk Mueller (mueller@kde.org)
 *           (C) 2006 Alexey Proskuryakov (ap@webkit.org)
 * Copyright (C) 2004, 2005, 2006, 2007, 2008, 2009, 2010, 2012 Apple Inc. All
 * rights reserved.
 * Copyright (C) 2008, 2009 Torch Mobile Inc. All rights reserved.
 * (http://www.torchmobile.com/)
 * Copyright (C) 2010 Nokia Corporation and/or its subsidiary(-ies)
 * Copyright (C) 2013 Google Inc. All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 *
 */

#include "core/dom/Fullscreen.h"

#include "core/HTMLElementTypeHelpers.h"
#include "core/dom/Document.h"
#include "core/dom/ElementTraversal.h"
#include "core/dom/StyleEngine.h"
#include "core/dom/TaskRunnerHelper.h"
#include "core/dom/UserGestureIndicator.h"
#include "core/events/Event.h"
#include "core/frame/HostsUsingFeatures.h"
#include "core/frame/LocalFrame.h"
#include "core/frame/LocalFrameView.h"
#include "core/frame/Settings.h"
#include "core/frame/UseCounter.h"
#include "core/html/HTMLIFrameElement.h"
#include "core/input/EventHandler.h"
#include "core/inspector/ConsoleMessage.h"
#include "core/layout/LayoutBlockFlow.h"
#include "core/layout/LayoutFullScreen.h"
#include "core/layout/api/LayoutFullScreenItem.h"
#include "core/page/ChromeClient.h"
#include "core/svg/SVGSVGElement.h"
#include "platform/ScopedOrientationChangeIndicator.h"
#include "platform/feature_policy/FeaturePolicy.h"

namespace blink {

namespace {

// https://html.spec.whatwg.org/multipage/embedded-content.html#allowed-to-use
bool AllowedToUseFullscreen(const Frame* frame) {
  // To determine whether a Document object |document| is allowed to use the
  // feature indicated by attribute name |allowattribute|, run these steps:

  // 1. If |document| has no browsing context, then return false.
  if (!frame)
    return false;

  if (!IsSupportedInFeaturePolicy(WebFeaturePolicyFeature::kFullscreen)) {
    // 2. If |document|'s browsing context is a top-level browsing context, then
    // return true.
    if (frame->IsMainFrame())
      return true;

    // 3. If |document|'s browsing context has a browsing context container that
    // is an iframe element with an |allowattribute| attribute specified, and
    // whose node document is allowed to use the feature indicated by
    // |allowattribute|, then return true.
    if (frame->Owner() && frame->Owner()->AllowFullscreen())
      return AllowedToUseFullscreen(frame->Tree().Parent());

    // 4. Return false.
    return false;
  }

  // 2. If Feature Policy is enabled, return the policy for "fullscreen"
  // feature.
  return frame->IsFeatureEnabled(WebFeaturePolicyFeature::kFullscreen);
}

bool AllowedToRequestFullscreen(Document& document) {
  // An algorithm is allowed to request fullscreen if one of the following is
  // true:

  //  The algorithm is triggered by a user activation.
  if (UserGestureIndicator::ProcessingUserGesture())
    return true;

  //  The algorithm is triggered by a user generated orientation change.
  if (ScopedOrientationChangeIndicator::ProcessingOrientationChange()) {
    UseCounter::Count(document,
                      UseCounter::kFullscreenAllowedByOrientationChange);
    return true;
  }

  String message = ExceptionMessages::FailedToExecute(
      "requestFullscreen", "Element",
      "API can only be initiated by a user gesture.");
  document.AddConsoleMessage(
      ConsoleMessage::Create(kJSMessageSource, kWarningMessageLevel, message));

  return false;
}

// https://fullscreen.spec.whatwg.org/#fullscreen-is-supported
bool FullscreenIsSupported(const Document& document) {
  LocalFrame* frame = document.GetFrame();
  if (!frame)
    return false;

  // Fullscreen is supported if there is no previously-established user
  // preference, security risk, or platform limitation.
  return !document.GetSettings() ||
         document.GetSettings()->GetFullscreenSupported();
}

// https://fullscreen.spec.whatwg.org/#fullscreen-element-ready-check
bool FullscreenElementReady(const Element& element) {
  // A fullscreen element ready check for an element |element| returns true if
  // all of the following are true, and false otherwise:

  // |element| is in a document.
  if (!element.isConnected())
    return false;

  // |element|'s node document is allowed to use the feature indicated by
  // attribute name allowfullscreen.
  if (!AllowedToUseFullscreen(element.GetDocument().GetFrame()))
    return false;

  // |element|'s node document's fullscreen element stack is either empty or its
  // top element is an inclusive ancestor of |element|.
  if (const Element* top_element =
          Fullscreen::FullscreenElementFrom(element.GetDocument())) {
    if (!top_element->contains(&element))
      return false;
  }

  // |element| has no ancestor element whose local name is iframe and namespace
  // is the HTML namespace.
  if (Traversal<HTMLIFrameElement>::FirstAncestor(element))
    return false;

  // |element|'s node document's browsing context either has a browsing context
  // container and the fullscreen element ready check returns true for
  // |element|'s node document's browsing context's browsing context container,
  // or it has no browsing context container.
  if (const Element* owner = element.GetDocument().LocalOwner()) {
    if (!FullscreenElementReady(*owner))
      return false;
  }

  return true;
}

// https://fullscreen.spec.whatwg.org/#dom-element-requestfullscreen step 4:
bool RequestFullscreenConditionsMet(Element& pending, Document& document) {
  // |pending|'s namespace is the HTML namespace or |pending| is an SVG svg or
  // MathML math element. Note: MathML is not supported.
  if (!pending.IsHTMLElement() && !isSVGSVGElement(pending))
    return false;

  // The fullscreen element ready check for |pending| returns false.
  if (!FullscreenElementReady(pending))
    return false;

  // Fullscreen is supported.
  if (!FullscreenIsSupported(document))
    return false;

  // This algorithm is allowed to request fullscreen.
  if (!AllowedToRequestFullscreen(document))
    return false;

  return true;
}

// Walks the frame tree and returns the first local ancestor frame, if any.
LocalFrame* NextLocalAncestor(Frame& frame) {
  Frame* parent = frame.Tree().Parent();
  if (!parent)
    return nullptr;
  if (parent->IsLocalFrame())
    return ToLocalFrame(parent);
  return NextLocalAncestor(*parent);
}

// Walks the document's frame tree and returns the document of the first local
// ancestor frame, if any.
Document* NextLocalAncestor(Document& document) {
  LocalFrame* frame = document.GetFrame();
  if (!frame)
    return nullptr;
  LocalFrame* next = NextLocalAncestor(*frame);
  if (!next)
    return nullptr;
  DCHECK(next->GetDocument());
  return next->GetDocument();
}

// Helper to walk the ancestor chain and return the Document of the topmost
// local ancestor frame. Note that this is not the same as the topmost frame's
// Document, which might be unavailable in OOPIF scenarios. For example, with
// OOPIFs, when called on the bottom frame's Document in a A-B-C-B hierarchy in
// process B, this will skip remote frame C and return this frame: A-[B]-C-B.
Document& TopmostLocalAncestor(Document& document) {
  if (Document* next = NextLocalAncestor(document))
    return TopmostLocalAncestor(*next);
  return document;
}

// https://fullscreen.spec.whatwg.org/#collect-documents-to-unfullscreen
HeapVector<Member<Document>> CollectDocumentsToUnfullscreen(
    Document& doc,
    Fullscreen::ExitType exit_type) {
  DCHECK(Fullscreen::FullscreenElementFrom(doc));

  // 1. If |doc|'s top layer consists of more than a single element that has
  // its fullscreen flag set, return the empty set.
  // TODO(foolip): See TODO in |fullyExitFullscreen()|.
  if (exit_type != Fullscreen::ExitType::kFully &&
      Fullscreen::FullscreenElementStackSizeFrom(doc) > 1)
    return HeapVector<Member<Document>>();

  // 2. Let |docs| be an ordered set consisting of |doc|.
  HeapVector<Member<Document>> docs;
  docs.push_back(&doc);

  // 3. While |docs|'s last document ...
  //
  // OOPIF: Skip over remote frames, assuming that they have exactly one element
  // in their fullscreen element stacks, thereby erring on the side of exiting
  // fullscreen. TODO(alexmos): Deal with nested fullscreen cases, see
  // https://crbug.com/617369.
  for (Document* document = NextLocalAncestor(doc); document;
       document = NextLocalAncestor(*document)) {
    // ... has a browsing context container whose node document's top layer
    // consists of a single element that has its fullscreen flag set and does
    // not have its iframe fullscreen flag set (if any), append that node
    // document to |docs|.
    // TODO(foolip): Support the iframe fullscreen flag.
    // https://crbug.com/644695
    if (Fullscreen::FullscreenElementStackSizeFrom(*document) == 1)
      docs.push_back(document);
    else
      break;
  }

  // 4. Return |docs|.
  return docs;
}

// Creates a non-bubbling event with |document| as its target.
Event* CreateEvent(const AtomicString& type, Document& document) {
  DCHECK(type == EventTypeNames::fullscreenchange ||
         type == EventTypeNames::fullscreenerror);

  Event* event = Event::Create(type);
  event->SetTarget(&document);
  return event;
}

// Creates a bubbling event with |element| as its target. If |element| is not
// connected to |document|, then |document| is used as the target instead.
Event* CreatePrefixedEvent(const AtomicString& type,
                           Element& element,
                           Document& document) {
  DCHECK(type == EventTypeNames::webkitfullscreenchange ||
         type == EventTypeNames::webkitfullscreenerror);

  Event* event = Event::CreateBubble(type);
  if (element.isConnected() && element.GetDocument() == document)
    event->SetTarget(&element);
  else
    event->SetTarget(&document);
  return event;
}

Event* CreateChangeEvent(Document& document,
                         Element& element,
                         Fullscreen::RequestType request_type) {
  if (request_type == Fullscreen::RequestType::kUnprefixed)
    return CreateEvent(EventTypeNames::fullscreenchange, document);
  return CreatePrefixedEvent(EventTypeNames::webkitfullscreenchange, element,
                             document);
}

Event* CreateErrorEvent(Document& document,
                        Element& element,
                        Fullscreen::RequestType request_type) {
  if (request_type == Fullscreen::RequestType::kUnprefixed)
    return CreateEvent(EventTypeNames::fullscreenerror, document);
  return CreatePrefixedEvent(EventTypeNames::webkitfullscreenerror, element,
                             document);
}

void DispatchEvents(const HeapVector<Member<Event>>& events) {
  for (Event* event : events)
    event->target()->DispatchEvent(event);
}

}  // anonymous namespace

const char* Fullscreen::SupplementName() {
  return "Fullscreen";
}

Fullscreen& Fullscreen::From(Document& document) {
  Fullscreen* fullscreen = FromIfExists(document);
  if (!fullscreen) {
    fullscreen = new Fullscreen(document);
    Supplement<Document>::ProvideTo(document, SupplementName(), fullscreen);
  }

  return *fullscreen;
}

Fullscreen* Fullscreen::FromIfExistsSlow(Document& document) {
  return static_cast<Fullscreen*>(
      Supplement<Document>::From(document, SupplementName()));
}

Element* Fullscreen::FullscreenElementFrom(Document& document) {
  if (Fullscreen* found = FromIfExists(document))
    return found->FullscreenElement();
  return nullptr;
}

size_t Fullscreen::FullscreenElementStackSizeFrom(Document& document) {
  if (Fullscreen* found = FromIfExists(document))
    return found->fullscreen_element_stack_.size();
  return 0;
}

Element* Fullscreen::FullscreenElementForBindingFrom(TreeScope& scope) {
  Element* element = FullscreenElementFrom(scope.GetDocument());
  if (!element || !RuntimeEnabledFeatures::fullscreenUnprefixedEnabled())
    return element;

  // TODO(kochi): Once V0 code is removed, we can use the same logic for
  // Document and ShadowRoot.
  if (!scope.RootNode().IsShadowRoot()) {
    // For Shadow DOM V0 compatibility: We allow returning an element in V0
    // shadow tree, even though it leaks the Shadow DOM.
    if (element->IsInV0ShadowTree()) {
      UseCounter::Count(scope.GetDocument(),
                        UseCounter::kDocumentFullscreenElementInV0Shadow);
      return element;
    }
  } else if (!ToShadowRoot(scope.RootNode()).IsV1()) {
    return nullptr;
  }
  return scope.AdjustedElement(*element);
}

bool Fullscreen::IsInFullscreenElementStack(const Element& element) {
  const Fullscreen* found = FromIfExists(element.GetDocument());
  if (!found)
    return false;
  for (size_t i = 0; i < found->fullscreen_element_stack_.size(); ++i) {
    if (found->fullscreen_element_stack_[i].first.Get() == &element)
      return true;
  }
  return false;
}

Fullscreen::Fullscreen(Document& document)
    : Supplement<Document>(document),
      ContextLifecycleObserver(&document),
      full_screen_layout_object_(nullptr) {
  document.SetHasFullscreenSupplement();
}

Fullscreen::~Fullscreen() {}

Document* Fullscreen::GetDocument() {
  return ToDocument(LifecycleContext());
}

void Fullscreen::ContextDestroyed(ExecutionContext*) {
  if (full_screen_layout_object_)
    full_screen_layout_object_->Destroy();

  pending_requests_.clear();
  fullscreen_element_stack_.clear();
}

// https://fullscreen.spec.whatwg.org/#dom-element-requestfullscreen
void Fullscreen::RequestFullscreen(Element& pending) {
  // TODO(foolip): Make RequestType::Unprefixed the default when the unprefixed
  // API is enabled. https://crbug.com/383813
  RequestFullscreen(pending, RequestType::kPrefixed);
}

void Fullscreen::RequestFullscreen(Element& pending, RequestType request_type) {
  Document& document = pending.GetDocument();

  // Ignore this call if the document is not in a live frame.
  if (!document.IsActive() || !document.GetFrame())
    return;

  bool for_cross_process_descendant =
      request_type == RequestType::kPrefixedForCrossProcessDescendant;

  // Use counters only need to be incremented in the process of the actual
  // fullscreen element.
  if (!for_cross_process_descendant) {
    if (document.IsSecureContext()) {
      UseCounter::Count(document, UseCounter::kFullscreenSecureOrigin);
    } else {
      UseCounter::Count(document, UseCounter::kFullscreenInsecureOrigin);
      HostsUsingFeatures::CountAnyWorld(
          document, HostsUsingFeatures::Feature::kFullscreenInsecureHost);
    }
  }

  // 1. Let |pending| be the context object.

  // 2. Let |error| be false.
  bool error = false;

  // 3. Let |promise| be a new promise.
  // TODO(foolip): Promises. https://crbug.com/644637

  // 4. If any of the following conditions are false, then set |error| to true:
  //
  // OOPIF: If |requestFullscreen()| was already called in a descendant frame
  // and passed the checks, do not check again here.
  if (!for_cross_process_descendant &&
      !RequestFullscreenConditionsMet(pending, document))
    error = true;

  // 5. Return |promise|, and run the remaining steps in parallel.
  // TODO(foolip): Promises. https://crbug.com/644637

  // 6. If |error| is false: Resize pending's top-level browsing context's
  // document's viewport's dimensions to match the dimensions of the screen of
  // the output device. Optionally display a message how the end user can
  // revert this.
  if (!error) {
    if (From(document).pending_requests_.size()) {
      UseCounter::Count(document,
                        UseCounter::kFullscreenRequestWithPendingElement);
    }

    // TODO(foolip): In order to reinstate the hierarchy restrictions in the
    // spec, something has to prevent dialog elements from moving within top
    // layer. Either disallowing fullscreen for dialog elements entirely or just
    // preventing dialog elements from simultaneously being fullscreen and modal
    // are good candidates. See https://github.com/whatwg/fullscreen/pull/91
    if (isHTMLDialogElement(pending)) {
      UseCounter::Count(document,
                        UseCounter::kRequestFullscreenForDialogElement);
      if (pending.IsInTopLayer()) {
        UseCounter::Count(
            document, UseCounter::kRequestFullscreenForDialogElementInTopLayer);
      }
    }

    From(document).pending_requests_.push_back(
        std::make_pair(&pending, request_type));
    LocalFrame& frame = *document.GetFrame();
    frame.GetChromeClient().EnterFullscreen(frame);
  } else {
    EnqueueTaskForRequest(document, pending, request_type, true);
  }
}

void Fullscreen::DidEnterFullscreen() {
  if (!GetDocument())
    return;

  ElementStack requests;
  requests.swap(pending_requests_);
  for (const ElementStackEntry& request : requests) {
    EnqueueTaskForRequest(*GetDocument(), *request.first, request.second,
                          false);
  }
}

void Fullscreen::EnqueueTaskForRequest(Document& document,
                                       Element& pending,
                                       RequestType request_type,
                                       bool error) {
  // 7. As part of the next animation frame task, run these substeps:
  document.EnqueueAnimationFrameTask(
      WTF::Bind(&RunTaskForRequest, WrapPersistent(&document),
                WrapPersistent(&pending), request_type, error));
}

void Fullscreen::RunTaskForRequest(Document* document,
                                   Element* element,
                                   RequestType request_type,
                                   bool error) {
  DCHECK(document);
  DCHECK(document->IsActive());
  DCHECK(document->GetFrame());
  DCHECK(element);

  Document& pending_doc = *document;
  Element& pending = *element;

  // TODO(foolip): Spec something like: If |pending|'s node document is not
  // |pendingDoc|, then set |error| to true.
  // https://github.com/whatwg/fullscreen/issues/33
  if (pending.GetDocument() != pending_doc)
    error = true;

  // 7.1. If either |error| is true or the fullscreen element ready check for
  // |pending| returns false, fire an event named fullscreenerror on
  // |pending|'s node document, reject |promise| with a TypeError exception,
  // and terminate these steps.
  // TODO(foolip): Promises. https://crbug.com/644637
  if (error || !FullscreenElementReady(pending)) {
    Event* event = CreateErrorEvent(pending_doc, pending, request_type);
    event->target()->DispatchEvent(event);
    return;
  }

  // 7.2. Let |fullscreenElements| be an ordered set initially consisting of
  // |pending|.
  HeapDeque<Member<Element>> fullscreen_elements;
  fullscreen_elements.push_back(pending);

  // 7.3. While the first element in |fullscreenElements| is in a nested
  // browsing context, prepend its browsing context container to
  // |fullscreenElements|.
  //
  // OOPIF: |fullscreenElements| will only contain elements for local ancestors,
  // and remote ancestors will be processed in their respective processes. This
  // preserves the spec's event firing order for local ancestors, but not for
  // remote ancestors. However, that difference shouldn't be observable in
  // practice: a fullscreenchange event handler would need to postMessage a
  // frame in another renderer process, where the message should be queued up
  // and processed after the IPC that dispatches fullscreenchange.
  for (Frame* frame = pending.GetDocument().GetFrame(); frame;
       frame = frame->Tree().Parent()) {
    if (!frame->Owner() || !frame->Owner()->IsLocal())
      continue;
    Element* element = ToHTMLFrameOwnerElement(frame->Owner());
    fullscreen_elements.push_front(element);
  }

  // 7.4. Let |eventDocs| be an empty list.
  // Note: For prefixed requests, the event target is an element, so instead
  // let |events| be a list of events to dispatch.
  HeapVector<Member<Event>> events;

  // 7.5. For each |element| in |fullscreenElements|, in order, run these
  // subsubsteps:
  for (Element* element : fullscreen_elements) {
    // 7.5.1. Let |doc| be |element|'s node document.
    Document& doc = element->GetDocument();

    // 7.5.2. If |element| is |doc|'s fullscreen element, terminate these
    // subsubsteps.
    if (element == FullscreenElementFrom(doc))
      continue;

    // 7.5.3. Otherwise, append |doc| to |eventDocs|.
    events.push_back(CreateChangeEvent(doc, *element, request_type));

    // 7.5.4. If |element| is |pending| and |pending| is an iframe element,
    // set |element|'s iframe fullscreen flag.
    // TODO(foolip): Support the iframe fullscreen flag.
    // https://crbug.com/644695

    // 7.5.5. Fullscreen |element| within |doc|.
    // TODO(foolip): Merge fullscreen element stack into top layer.
    // https://crbug.com/627790
    From(doc).PushFullscreenElementStack(*element, request_type);
  }

  // 7.6. For each |doc| in |eventDocs|, in order, fire an event named
  // fullscreenchange on |doc|.
  DispatchEvents(events);

  // 7.7. Fulfill |promise| with undefined.
  // TODO(foolip): Promises. https://crbug.com/644637
}

// https://fullscreen.spec.whatwg.org/#fully-exit-fullscreen
void Fullscreen::FullyExitFullscreen(Document& document) {
  // 1. If |document|'s fullscreen element is null, terminate these steps.

  // 2. Unfullscreen elements whose fullscreen flag is set, within
  // |document|'s top layer, except for |document|'s fullscreen element.

  // 3. Exit fullscreen |document|.

  // TODO(foolip): Change the spec. To remove elements from |document|'s top
  // layer as in step 2 could leave descendant frames in fullscreen. It may work
  // to give the "exit fullscreen" algorithm a |fully| flag that's used in the
  // animation frame task after exit. Here, retain the old behavior of fully
  // exiting fullscreen for the topmost local ancestor:
  ExitFullscreen(TopmostLocalAncestor(document), ExitType::kFully);
}

// https://fullscreen.spec.whatwg.org/#exit-fullscreen
void Fullscreen::ExitFullscreen(Document& doc, ExitType exit_type) {
  if (!doc.IsActive() || !doc.GetFrame())
    return;

  // 1. Let |promise| be a new promise.
  // 2. If |doc|'s fullscreen element is null, reject |promise| with a
  // TypeError exception, and return |promise|.
  // TODO(foolip): Promises. https://crbug.com/644637
  if (!FullscreenElementFrom(doc))
    return;

  // 3. Let |resize| be false.
  bool resize = false;

  // 4. Let |docs| be the result of collecting documents to unfullscreen given
  // |doc|.
  HeapVector<Member<Document>> docs =
      CollectDocumentsToUnfullscreen(doc, exit_type);

  // 5. Let |topLevelDoc| be |doc|'s top-level browsing context's document.
  //
  // OOPIF: Let |topLevelDoc| be the topmost local ancestor instead. If the main
  // frame is in another process, we will still fully exit fullscreen even
  // though that's wrong if the main frame was in nested fullscreen.
  // TODO(alexmos): Deal with nested fullscreen cases, see
  // https://crbug.com/617369.
  Document& top_level_doc = TopmostLocalAncestor(doc);

  // 6. If |topLevelDoc| is in |docs|, set |resize| to true.
  if (!docs.IsEmpty() && docs.back() == &top_level_doc)
    resize = true;

  // 7. Return |promise|, and run the remaining steps in parallel.
  // TODO(foolip): Promises. https://crbug.com/644637

  // Note: |ExitType::Fully| is only used together with the topmost local
  // ancestor in |fullyExitFullscreen()|, and so implies that |resize| is true.
  // This would change if matching the spec for "fully exit fullscreen".
  if (exit_type == ExitType::kFully)
    DCHECK(resize);

  // 8. If |resize| is true, resize |topLevelDoc|'s viewport to its "normal"
  // dimensions.
  if (resize) {
    LocalFrame& frame = *doc.GetFrame();
    frame.GetChromeClient().ExitFullscreen(frame);
  } else {
    EnqueueTaskForExit(doc, exit_type);
  }
}

void Fullscreen::DidExitFullscreen() {
  if (!GetDocument())
    return;

  DCHECK_EQ(GetDocument(), &TopmostLocalAncestor(*GetDocument()));

  EnqueueTaskForExit(*GetDocument(), ExitType::kFully);
}

void Fullscreen::EnqueueTaskForExit(Document& document, ExitType exit_type) {
  // 9. As part of the next animation frame task, run these substeps:
  document.EnqueueAnimationFrameTask(
      WTF::Bind(&RunTaskForExit, WrapPersistent(&document), exit_type));
}

void Fullscreen::RunTaskForExit(Document* document, ExitType exit_type) {
  DCHECK(document);
  DCHECK(document->IsActive());
  DCHECK(document->GetFrame());

  Document& doc = *document;

  if (!FullscreenElementFrom(doc))
    return;

  // 9.1. Let |exitDocs| be the result of collecting documents to unfullscreen
  // given |doc|.

  // 9.2. If |resize| is true and |topLevelDoc| is not in |exitDocs|, fully
  // exit fullscreen |topLevelDoc|, reject promise with a TypeError exception,
  // and terminate these steps.

  // TODO(foolip): See TODO in |fullyExitFullscreen()|. Instead of using "fully
  // exit fullscreen" in step 9.2 (which is async), give "exit fullscreen" a
  // |fully| flag which is always true if |resize| was true.

  HeapVector<Member<Document>> exit_docs =
      CollectDocumentsToUnfullscreen(doc, exit_type);

  // 9.3. If |exitDocs| is the empty set, append |doc| to |exitDocs|.
  if (exit_docs.IsEmpty())
    exit_docs.push_back(&doc);

  // 9.4. If |exitDocs|'s last document has a browsing context container,
  // append that browsing context container's node document to |exitDocs|.
  //
  // OOPIF: Skip over remote frames, assuming that they have exactly one element
  // in their fullscreen element stacks, thereby erring on the side of exiting
  // fullscreen. TODO(alexmos): Deal with nested fullscreen cases, see
  // https://crbug.com/617369.
  if (Document* document = NextLocalAncestor(*exit_docs.back()))
    exit_docs.push_back(document);

  // 9.5. Let |descendantDocs| be an ordered set consisting of |doc|'s
  // descendant browsing contexts' documents whose fullscreen element is
  // non-null, if any, in *reverse* tree order.
  HeapDeque<Member<Document>> descendant_docs;
  for (Frame* descendant = doc.GetFrame()->Tree().FirstChild(); descendant;
       descendant = descendant->Tree().TraverseNext(doc.GetFrame())) {
    if (!descendant->IsLocalFrame())
      continue;
    DCHECK(ToLocalFrame(descendant)->GetDocument());
    if (FullscreenElementFrom(*ToLocalFrame(descendant)->GetDocument()))
      descendant_docs.push_front(ToLocalFrame(descendant)->GetDocument());
  }

  // Note: For prefixed requests, the event target is an element, so let
  // |events| be a list of events to dispatch.
  HeapVector<Member<Event>> events;

  // 9.6. For each |descendantDoc| in |descendantDocs|, in order, unfullscreen
  // |descendantDoc|.
  for (Document* descendant_doc : descendant_docs) {
    Fullscreen& fullscreen = From(*descendant_doc);
    ElementStack& stack = fullscreen.fullscreen_element_stack_;
    DCHECK(!stack.IsEmpty());
    events.push_back(CreateChangeEvent(*descendant_doc, *stack.back().first,
                                       stack.back().second));
    while (!stack.IsEmpty())
      fullscreen.PopFullscreenElementStack();
  }

  // 9.7. For each |exitDoc| in |exitDocs|, in order, unfullscreen |exitDoc|'s
  // fullscreen element.
  for (Document* exit_doc : exit_docs) {
    Fullscreen& fullscreen = From(*exit_doc);
    ElementStack& stack = fullscreen.fullscreen_element_stack_;
    DCHECK(!stack.IsEmpty());
    events.push_back(
        CreateChangeEvent(*exit_doc, *stack.back().first, stack.back().second));
    fullscreen.PopFullscreenElementStack();

    // TODO(foolip): See TODO in |fullyExitFullscreen()|.
    if (exit_doc == &doc && exit_type == ExitType::kFully) {
      while (!stack.IsEmpty())
        fullscreen.PopFullscreenElementStack();
    }
  }

  // 9.8. For each |descendantDoc| in |descendantDocs|, in order, fire an
  // event named fullscreenchange on |descendantDoc|.
  // 9.9. For each |exitDoc| in |exitDocs|, in order, fire an event named
  // fullscreenchange on |exitDoc|.
  DispatchEvents(events);

  // 9.10. Fulfill |promise| with undefined.
  // TODO(foolip): Promises. https://crbug.com/644637
}

// https://fullscreen.spec.whatwg.org/#dom-document-fullscreenenabled
bool Fullscreen::FullscreenEnabled(Document& document) {
  // The fullscreenEnabled attribute's getter must return true if the context
  // object is allowed to use the feature indicated by attribute name
  // allowfullscreen and fullscreen is supported, and false otherwise.
  return AllowedToUseFullscreen(document.GetFrame()) &&
         FullscreenIsSupported(document);
}

void Fullscreen::SetFullScreenLayoutObject(LayoutFullScreen* layout_object) {
  if (layout_object == full_screen_layout_object_)
    return;

  if (layout_object && saved_placeholder_computed_style_) {
    layout_object->CreatePlaceholder(
        std::move(saved_placeholder_computed_style_),
        saved_placeholder_frame_rect_);
  } else if (layout_object && full_screen_layout_object_ &&
             full_screen_layout_object_->Placeholder()) {
    LayoutBlockFlow* placeholder = full_screen_layout_object_->Placeholder();
    layout_object->CreatePlaceholder(
        ComputedStyle::Clone(placeholder->StyleRef()),
        placeholder->FrameRect());
  }

  if (full_screen_layout_object_)
    full_screen_layout_object_->UnwrapLayoutObject();
  DCHECK(!full_screen_layout_object_);

  full_screen_layout_object_ = layout_object;
}

void Fullscreen::FullScreenLayoutObjectDestroyed() {
  full_screen_layout_object_ = nullptr;
}

void Fullscreen::ElementRemoved(Element& node) {
  DCHECK_EQ(GetDocument(), &node.GetDocument());

  // |Fullscreen::ElementRemoved| is called for each removed element, so only
  // the body of the spec "removing steps" loop appears here:

  // 2.1. If |node| is its node document's fullscreen element, exit fullscreen
  // that document.
  if (FullscreenElement() == &node) {
    ExitFullscreen(node.GetDocument());
    return;
  }

  // 2.2. Otherwise, unfullscreen |node| within its node document.
  for (size_t i = 0; i < fullscreen_element_stack_.size(); ++i) {
    if (fullscreen_element_stack_[i].first.Get() == &node) {
      fullscreen_element_stack_.erase(i);
      return;
    }
  }

  // Note: |node| was not in the fullscreen element stack.
}

void Fullscreen::PopFullscreenElementStack() {
  DCHECK(!fullscreen_element_stack_.IsEmpty());

  Element* previous_element = FullscreenElement();
  fullscreen_element_stack_.pop_back();

  // Note: |requestType| is only used if |fullscreenElement()| is non-null.
  RequestType request_type = fullscreen_element_stack_.IsEmpty()
                                 ? RequestType::kUnprefixed
                                 : fullscreen_element_stack_.back().second;
  FullscreenElementChanged(previous_element, FullscreenElement(), request_type);
}

void Fullscreen::PushFullscreenElementStack(Element& element,
                                            RequestType request_type) {
  Element* previous_element = FullscreenElement();
  fullscreen_element_stack_.push_back(std::make_pair(&element, request_type));

  FullscreenElementChanged(previous_element, &element, request_type);
}

void Fullscreen::FullscreenElementChanged(Element* from_element,
                                          Element* to_element,
                                          RequestType to_request_type) {
  DCHECK_NE(from_element, to_element);

  if (!GetDocument())
    return;

  GetDocument()->GetStyleEngine().EnsureUAStyleForFullscreen();

  if (full_screen_layout_object_)
    full_screen_layout_object_->UnwrapLayoutObject();
  DCHECK(!full_screen_layout_object_);

  if (from_element) {
    DCHECK_NE(from_element, FullscreenElement());

    from_element->PseudoStateChanged(CSSSelector::kPseudoFullScreen);

    from_element->SetContainsFullScreenElement(false);
    from_element
        ->SetContainsFullScreenElementOnAncestorsCrossingFrameBoundaries(false);
  }

  if (to_element) {
    DCHECK_EQ(to_element, FullscreenElement());

    to_element->PseudoStateChanged(CSSSelector::kPseudoFullScreen);

    // OOPIF: For RequestType::PrefixedForCrossProcessDescendant, |toElement| is
    // the iframe element for the out-of-process frame that contains the
    // fullscreen element. Hence, it must match :-webkit-full-screen-ancestor.
    if (to_request_type == RequestType::kPrefixedForCrossProcessDescendant) {
      DCHECK(isHTMLIFrameElement(to_element));
      to_element->SetContainsFullScreenElement(true);
    }
    to_element->SetContainsFullScreenElementOnAncestorsCrossingFrameBoundaries(
        true);

    // Create a placeholder block for the fullscreen element, to keep the page
    // from reflowing when the element is removed from the normal flow. Only do
    // this for a LayoutBox, as only a box will have a frameRect. The
    // placeholder will be created in setFullScreenLayoutObject() during layout.
    LayoutObject* layout_object = to_element->GetLayoutObject();
    bool should_create_placeholder = layout_object && layout_object->IsBox();
    if (should_create_placeholder) {
      saved_placeholder_frame_rect_ = ToLayoutBox(layout_object)->FrameRect();
      saved_placeholder_computed_style_ =
          ComputedStyle::Clone(layout_object->StyleRef());
    }

    if (to_element != GetDocument()->documentElement()) {
      LayoutFullScreen::WrapLayoutObject(
          layout_object, layout_object ? layout_object->Parent() : 0,
          GetDocument());
    }
  }

  if (LocalFrame* frame = GetDocument()->GetFrame()) {
    // TODO(foolip): Synchronize hover state changes with animation frames.
    // https://crbug.com/668758
    frame->GetEventHandler().ScheduleHoverStateUpdate();
    frame->GetChromeClient().FullscreenElementChanged(from_element, to_element);

    if (RuntimeEnabledFeatures::slimmingPaintInvalidationEnabled() &&
        !RuntimeEnabledFeatures::rootLayerScrollingEnabled()) {
      // Fullscreen status affects scroll paint properties through
      // LocalFrameView::UserInputScrollable().
      if (LocalFrameView* frame_view = frame->View())
        frame_view->SetNeedsPaintPropertyUpdate();
    }
  }

  // TODO(foolip): This should not call updateStyleAndLayoutTree.
  GetDocument()->UpdateStyleAndLayoutTree();
}

DEFINE_TRACE(Fullscreen) {
  visitor->Trace(pending_requests_);
  visitor->Trace(fullscreen_element_stack_);
  Supplement<Document>::Trace(visitor);
  ContextLifecycleObserver::Trace(visitor);
}

}  // namespace blink
