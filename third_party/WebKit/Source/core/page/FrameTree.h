/*
 * Copyright (C) 2006 Apple Computer, Inc.
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
 */

#ifndef FrameTree_h
#define FrameTree_h

#include "core/CoreExport.h"
#include "platform/heap/Handle.h"
#include "platform/wtf/Functional.h"
#include "platform/wtf/text/AtomicString.h"

namespace blink {

class Frame;
class LocalFrame;

class CORE_EXPORT FrameTree final {
  WTF_MAKE_NONCOPYABLE(FrameTree);
  DISALLOW_NEW();

 public:
  explicit FrameTree(Frame* this_frame);
  ~FrameTree();

  const AtomicString& GetName() const;
  void SetName(const AtomicString&);

  // TODO(andypaicu): remove this once we have gathered the data
  void ExperimentalSetNulledName();

  Frame* Parent() const;
  Frame& Top() const;
  Frame* NextSibling() const;
  Frame* FirstChild() const;

  bool IsDescendantOf(const Frame* ancestor) const;
  Frame* TraverseNext(const Frame* stay_within = nullptr) const;

  Frame* Find(const AtomicString& name) const;
  unsigned ChildCount() const;

  Frame* ScopedChild(unsigned index) const;
  Frame* ScopedChild(const AtomicString& name) const;
  unsigned ScopedChildCount() const;
  void InvalidateScopedChildCount();

  // Iterates over the Frame subtree starting at |root|.
  class CORE_EXPORT Iterator {
    STACK_ALLOCATED();

   public:
    Iterator(Frame* root,
             std::unique_ptr<WTF::Function<bool(Frame*)>> should_skip_frame =
                 nullptr)
        : root_(root),
          current_frame_(root),
          should_skip_frame_(std::move(should_skip_frame)) {}

    Iterator& operator++();
    bool operator==(const Iterator&) const;
    bool operator!=(const Iterator&) const;
    operator bool() const;

    Frame* operator*() { return current_frame_; }
    Frame* operator->() { return current_frame_; }

   protected:
    Member<const Frame> root_;
    Member<Frame> current_frame_;
    std::unique_ptr<WTF::Function<bool(Frame*)>> should_skip_frame_;
  };

  // Specializes iterator to only iterate over local frames.
  class CORE_EXPORT LocalFrameIterator : public Iterator {
    STACK_ALLOCATED();

   public:
    LocalFrameIterator(Frame* root,
                       std::unique_ptr<WTF::Function<bool(Frame*)>>
                           should_skip_frame = nullptr)
        : Iterator(root, std::move(should_skip_frame)) {}

    LocalFrameIterator& operator++();
    LocalFrame* operator*();
    LocalFrame* operator->();
  };

  class CORE_EXPORT LocalRootRange {
    STACK_ALLOCATED();

   public:
    LocalFrameIterator begin();
    LocalFrameIterator end();

   private:
    friend class FrameTree;
    LocalRootRange(LocalFrame* root) : root_(root) {}

    Member<LocalFrame> root_;
  };

  // Provides an iterator for |this_frame_|'s subtree.
  Iterator begin();
  Iterator end();

  // Provides a range for LocalFrames within this subtree's local root.
  LocalRootRange GetLocalRootRange();

  DECLARE_TRACE();

 private:
  Member<Frame> this_frame_;

  AtomicString name_;  // The actual frame name (may be empty).

  mutable unsigned scoped_child_count_;

  // TODO(andypaicu): remove this once we have gathered the data
  bool experimental_set_nulled_name_;
};

}  // namespace blink

#ifndef NDEBUG
// Outside the WebCore namespace for ease of invocation from gdb.
void showFrameTree(const blink::Frame*);
#endif

#endif  // FrameTree_h
