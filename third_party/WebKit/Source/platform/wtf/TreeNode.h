/*
 * Copyright (C) 2013 Google Inc. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are
 * met:
 *
 *     * Redistributions of source code must retain the above copyright
 * notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above
 * copyright notice, this list of conditions and the following disclaimer
 * in the documentation and/or other materials provided with the
 * distribution.
 *     * Neither the name of Google Inc. nor the names of its
 * contributors may be used to endorse or promote products derived from
 * this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 * LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
 * A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
 * OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT
 * LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef TreeNode_h
#define TreeNode_h

#include "platform/wtf/Assertions.h"

namespace WTF {

//
// TreeNode is generic, ContainerNode-like linked tree data structure.
// There are a few notable difference between TreeNode and Node:
//
//  * Each TreeNode node is NOT ref counted. The user have to retain its
//    lifetime somehow.
//    FIXME: lifetime management could be parameterized so that ref counted
//    implementations can be used.
//  * It checks invalid input. The callers have to ensure that given
//    parameter is sound.
//  * There is no branch-leaf difference. Every node can be a parent of other
//    node.
//
// FIXME: oilpan: Trace tree node edges to ensure we don't have dangling
// pointers.
// As it is used in HTMLImport it is safe since they all die together.
template <class T>
class TreeNode {
 public:
  using NodeType = T;
  TreeNode() { next_ = previous_ = Here(); }

  NodeType* Next() const {
    if (!HasSibling())
      return nullptr;
    if (parent_ && next_ == parent_->first_child_)
      return nullptr;
    return next_;
  }
  NodeType* Previous() const {
    if (!HasSibling())
      return nullptr;
    if (parent_ && Here() == parent_->first_child_)
      return nullptr;
    return previous_;
  }
  NodeType* RawNext() const { return next_; }
  NodeType* RawPrevious() const { return previous_; }
  NodeType* Parent() const { return parent_; }
  NodeType* FirstChild() const { return first_child_; }
  NodeType* LastChild() const {
    return first_child_ ? first_child_->previous_ : nullptr;
  }
  bool HasSibling() const { return next_ != Here(); }
  NodeType* Here() const {
    return static_cast<NodeType*>(const_cast<TreeNode*>(this));
  }

  // TODO: rename IsOrphan()
  bool Orphan() const { return !parent_ && !HasSibling() && !first_child_; }
  bool HasChildren() const { return first_child_; }

  void InsertBefore(NodeType* new_child, NodeType* ref_child) {
    DCHECK(!new_child->Parent());
    DCHECK(!new_child->HasSibling());
    DCHECK(!ref_child || Here() == ref_child->Parent());

    if (!ref_child) {
      AppendChild(new_child);
      return;
    }

    NodeType* new_previous = ref_child->RawPrevious();
    DCHECK(new_previous);
    new_child->parent_ = Here();
    new_child->next_ = ref_child;
    new_child->previous_ = new_previous;
    ref_child->previous_ = new_child;
    new_previous->next_ = new_child;
    if (first_child_ == ref_child)
      first_child_ = new_child;
  }

  void AppendChild(NodeType* child) {
    DCHECK(!child->Parent());
    DCHECK(!child->HasSibling());

    child->parent_ = Here();

    if (!first_child_) {
      first_child_ = child;
      return;
    }

    NodeType* old_last = first_child_->previous_;
    DCHECK(old_last);
    first_child_->previous_ = child;
    child->next_ = first_child_;
    child->previous_ = old_last;
    old_last->next_ = child;
  }

  NodeType* RemoveChild(NodeType* child) {
    DCHECK_EQ(child->Parent(), this);

    child->parent_ = nullptr;

    if (!child->HasSibling()) {
      first_child_ = nullptr;
      return child;
    }

    if (first_child_ == child)
      first_child_ = child->RawNext();

    NodeType* old_next = child->RawNext();
    NodeType* old_previous = child->RawPrevious();

    DCHECK(old_next);
    old_next->previous_ = old_previous;
    DCHECK(old_previous);
    old_previous->next_ = old_next;

    child->next_ = child->previous_ = child;

    return child;
  }

  void TakeChildrenFrom(NodeType* old_parent) {
    DCHECK_NE(old_parent, this);
    while (old_parent->HasChildren()) {
      NodeType* child = old_parent->FirstChild();
      old_parent->RemoveChild(child);
      AppendChild(child);
    }
  }

 private:
  NodeType* next_ = nullptr;
  NodeType* previous_ = nullptr;
  NodeType* parent_ = nullptr;
  NodeType* first_child_ = nullptr;
};

template <class T>
inline typename TreeNode<T>::NodeType* TraverseNext(
    const TreeNode<T>* current,
    const TreeNode<T>* stay_within = nullptr) {
  if (typename TreeNode<T>::NodeType* next = current->FirstChild())
    return next;
  if (current == stay_within)
    return nullptr;
  typename TreeNode<T>::NodeType* next = current->RawNext();
  if (current->HasSibling() && next != current->Parent()->FirstChild())
    return next;
  for (typename TreeNode<T>::NodeType* parent = current->Parent(); parent;
       parent = parent->Parent()) {
    if (parent == stay_within)
      return nullptr;
    typename TreeNode<T>::NodeType* next = parent->RawNext();
    if (parent->HasSibling() && next != parent->Parent()->FirstChild())
      return next;
  }
  return nullptr;
}

template <class T>
inline typename TreeNode<T>::NodeType* TraverseFirstPostOrder(
    const TreeNode<T>* current) {
  typename TreeNode<T>::NodeType* first = current->Here();
  while (first->FirstChild())
    first = first->FirstChild();
  return first;
}

template <class T>
inline typename TreeNode<T>::NodeType* TraverseNextPostOrder(
    const TreeNode<T>* current,
    const TreeNode<T>* stay_within = nullptr) {
  if (current == stay_within)
    return nullptr;

  typename TreeNode<T>::NodeType* next = current->RawNext();
  if (!current->HasSibling() || next == current->Parent()->FirstChild())
    return current->Parent();
  while (next->FirstChild())
    next = next->FirstChild();
  return next;
}

}  // namespace WTF

using WTF::TreeNode;
using WTF::TraverseNext;
using WTF::TraverseNextPostOrder;

#endif
