// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef SynchronousMutationObserver_h
#define SynchronousMutationObserver_h

#include "base/macros.h"
#include "core/CoreExport.h"
#include "core/dom/SynchronousMutationNotifier.h"
#include "platform/heap/Member.h"

namespace blink {

class CharacterData;
class ContainerNode;
class Document;
class NodeWithIndex;
class Text;

// This class is a base class for classes which observe DOM tree mutation
// synchronously. If you want to observe DOM tree mutation asynchronously see
// MutationObserver Web API.
// Note: if you only need to observe Document shutdown,
// DocumentShutdownObserver provides this same functionality more efficiently
// (since it doesn't observe the other events).
//
// It is recommended that subclasses declare themselves final, to allow
// uninteresting notifications to be dropped at compile time.
//
// TODO(yosin): Following classes should be derived from this class to
// simplify Document class.
//  - DragCaret
//  - DocumentMarkerController
//  - EventHandler
//  - FrameCaret
//  - InputMethodController
//  - SelectionController
//  - Range set
//  - NodeIterator set
class CORE_EXPORT SynchronousMutationObserverBase
    : public GarbageCollectedMixin {
 public:
  SynchronousMutationNotifier* GetSynchronousMutationNotifier() const {
    return notifier_.Get();
  }

  // For internal use by SynchronousMutationNotifier.
  void ForceClearSynchronousMutationNotifier() { notifier_ = nullptr; }

  // TODO(yosin): We will have following member functions:
  //  - dataWillBeChanged(const CharacterData&);
  //  - didMoveTreeToNewDocument(const Node& root);
  //  - didInsertText(Node*, unsigned offset, unsigned length);
  //  - didRemoveText(Node*, unsigned offset, unsigned length);

  // Called after child nodes changed.
  virtual void DidChangeChildren(const ContainerNode&) {}

  // Called after characters in |nodeToBeRemoved| is appended into |mergedNode|.
  // |oldLength| holds length of |mergedNode| before merge.
  virtual void DidMergeTextNodes(
      const Text& merged_node,
      const NodeWithIndex& node_to_be_removed_with_index,
      unsigned old_length) {}

  // Called just after node tree |root| is moved to new document.
  virtual void DidMoveTreeToNewDocument(const Node& root) {}

  // Called when |Text| node is split, next sibling |oldNode| contains
  // characters after split point.
  virtual void DidSplitTextNode(const Text& old_node) {}

  // Called when |CharacterData| is updated at |offset|, |oldLength| is a
  // number of deleted character and |newLength| is a number of added
  // characters.
  virtual void DidUpdateCharacterData(CharacterData*,
                                      unsigned offset,
                                      unsigned old_length,
                                      unsigned new_length) {}

  // Called before removing container node.
  virtual void NodeChildrenWillBeRemoved(ContainerNode&) {}

  // Called before removing node.
  virtual void NodeWillBeRemoved(Node&) {}

  // Called when detaching document.
  virtual void ContextDestroyed(Document*) {}

 protected:
  SynchronousMutationObserverBase() {}

  void Trace(Visitor* visitor) { visitor->Trace(notifier_); }

  WeakMember<SynchronousMutationNotifier> notifier_;

 private:
  DISALLOW_COPY_AND_ASSIGN(SynchronousMutationObserverBase);
};

template <typename Derived>
class SynchronousMutationObserver : public SynchronousMutationObserverBase {
 protected:
  void StartObserving(SynchronousMutationNotifier& notifier) {
    notifier_ = &notifier;
    notifier.AddObserver(static_cast<Derived*>(this));
  }

  void StopObserving() {
    if (!notifier_)
      return;
    notifier_->RemoveObserver(static_cast<Derived*>(this));
    notifier_ = nullptr;
  }

  void SetContext(SynchronousMutationNotifier* notifier) {
    if (notifier == notifier_)
      return;

    StopObserving();
    if (notifier)
      StartObserving(*notifier);
  }
};

// For use in unit tests. Must be declared in production to reserve a slot in
// SynchronousMutationNotifier.
class TestSynchronousMutationObserverBase
    : public SynchronousMutationObserver<TestSynchronousMutationObserverBase> {
};

}  // namespace blink

#endif  // SynchronousMutationObserver_h
