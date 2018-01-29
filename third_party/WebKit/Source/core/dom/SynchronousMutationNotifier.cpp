// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/dom/SynchronousMutationNotifier.h"

#include "core/dom/Document.h"
#include "core/dom/SynchronousMutationObserver.h"
#include "core/editing/DragCaret.h"
#include "core/editing/FrameSelection.h"
#include "core/editing/SelectionEditor.h"
#include "core/editing/markers/DocumentMarkerController.h"
#include "core/input/MouseEventManager.h"

namespace blink {

namespace {

template <typename... T>
struct IndexSequenceForTuple;

template <typename... T>
struct IndexSequenceForTuple<std::tuple<T...>> {
  using type = std::index_sequence_for<T...>;
};

template <typename Functor>
void VisitSynchronousMutationObserverSetImpl(
    SynchronousMutationObserverSet& observers,
    const Functor& f,
    std::index_sequence<>) {}

template <typename Functor, size_t Index, size_t... Indices>
void VisitSynchronousMutationObserverSetImpl(
    SynchronousMutationObserverSet& observers,
    const Functor& f,
    std::index_sequence<Index, Indices...>) {
  f(std::get<Index>(observers));
  VisitSynchronousMutationObserverSetImpl(observers, f,
                                          std::index_sequence<Indices...>());
}

// Visits every WeakMember in the observer set.
template <typename Functor>
void VisitSynchronousMutationObserverSet(
    SynchronousMutationObserverSet& observers,
    const Functor& f) {
  VisitSynchronousMutationObserverSetImpl(
      observers, f,
      IndexSequenceForTuple<SynchronousMutationObserverSet>::type());
}

// TODO(jbroman): These can be generic lambdas, once we allow them.
struct TraceFunctor {
  STACK_ALLOCATED();

 public:
  explicit TraceFunctor(Visitor* visitor) : visitor(visitor) {}

  template <typename T>
  void operator()(const WeakMember<T>& member) const {
    visitor->Trace(member);
  }

  Visitor* visitor;
};

struct DestroyFunctor {
  STACK_ALLOCATED();

 public:
  explicit DestroyFunctor(Document* document) : document(document) {}

  template <typename T>
  void operator()(WeakMember<T>& member) const {
    if (!member)
      return;
    auto& observer = static_cast<SynchronousMutationObserverBase&>(*member);
    DCHECK_EQ(observer.GetSynchronousMutationNotifier(), document);
    observer.ContextDestroyed(document);
    observer.ForceClearSynchronousMutationNotifier();
  }

  Member<Document> document;
};

}  // namespace

SynchronousMutationNotifier::SynchronousMutationNotifier() = default;

void SynchronousMutationNotifier::Trace(Visitor* visitor) {
  VisitSynchronousMutationObserverSet(observers_, TraceFunctor(visitor));
}

template <typename Functor>
void SynchronousMutationNotifier::ForEachObserver(const Functor& f) {
  VisitSynchronousMutationObserverSet(observers_, [&f](const auto& observer) {
    if (observer)
      f(observer);
  });
}

void SynchronousMutationNotifier::NotifyContextDestroyed() {
  // This imitates the destruction logic implemented by LifecycleNotifier.
  SynchronousMutationObserverSet observers;
  std::swap(observers, observers_);
  VisitSynchronousMutationObserverSet(
      observers, DestroyFunctor(static_cast<Document*>(this)));
  observers_ = SynchronousMutationObserverSet();
}

void SynchronousMutationNotifier::NotifyChangeChildren(
    const ContainerNode& container) {
  ForEachObserver(
      [&](const auto& observer) { observer->DidChangeChildren(container); });
}

void SynchronousMutationNotifier::NotifyMergeTextNodes(
    const Text& node,
    const NodeWithIndex& node_to_be_removed_with_index,
    unsigned old_length) {
  ForEachObserver([&](const auto& observer) {
    observer->DidMergeTextNodes(node, node_to_be_removed_with_index,
                                old_length);
  });
}

void SynchronousMutationNotifier::NotifyMoveTreeToNewDocument(
    const Node& root) {
  ForEachObserver(
      [&](const auto& observer) { observer->DidMoveTreeToNewDocument(root); });
}

void SynchronousMutationNotifier::NotifySplitTextNode(const Text& node) {
  ForEachObserver(
      [&](const auto& observer) { observer->DidSplitTextNode(node); });
}

void SynchronousMutationNotifier::NotifyUpdateCharacterData(
    CharacterData* character_data,
    unsigned offset,
    unsigned old_length,
    unsigned new_length) {
  ForEachObserver([&](const auto& observer) {
    observer->DidUpdateCharacterData(character_data, offset, old_length,
                                     new_length);
  });
}

void SynchronousMutationNotifier::NotifyNodeChildrenWillBeRemoved(
    ContainerNode& container) {
  ForEachObserver([&](const auto& observer) {
    observer->NodeChildrenWillBeRemoved(container);
  });
}

void SynchronousMutationNotifier::NotifyNodeWillBeRemoved(Node& node) {
  ForEachObserver(
      [&](const auto& observer) { observer->NodeWillBeRemoved(node); });
}

}  // namespace blink
