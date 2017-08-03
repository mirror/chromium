// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_INSTALLER_ZUCCHINI_REL32_FINDER_H_
#define CHROME_INSTALLER_ZUCCHINI_REL32_FINDER_H_

#include <stddef.h>

#include <vector>

#include "base/logging.h"
#include "base/macros.h"
#include "base/optional.h"
#include "chrome/installer/zucchini/buffer_view.h"
#include "chrome/installer/zucchini/image_utils.h"

namespace zucchini {

// Utility class to find gap between abs32 references. This is a stateful
// class that keeps track of a position that is updated during each query.
class Abs32GapFinder {
 public:
  // |abs32_locations| is a sorted list of distinct abs32 reference locations in
  // |image|, each spanning |abs32_width| bytes. Gaps are searched in |region|,
  // which must be part of |image|.
  Abs32GapFinder(ConstBufferView image,
                 ConstBufferView region,
                 const std::vector<offset_t>& abs32_locations,
                 size_t abs32_width);

  // Returns the next available gap, or nullopt if exhausted.
  base::Optional<ConstBufferView> GetNext();

 private:
  const ConstBufferView::const_iterator base_;
  const ConstBufferView::const_iterator region_end_;
  ConstBufferView::const_iterator current_lo_;
  std::vector<offset_t>::const_iterator abs32_current_;
  std::vector<offset_t>::const_iterator abs32_end_;
  size_t abs32_width_;
};

// A class to parse executable bytes of an image to find rel32 locations.
// Architecture-specific parse details are delegated to inherited classes.
// This is typically used along with Abs32GapFinder to find search regions.
// The caller may filter rel32 locations, based on rel32 targets.
class Rel32Finder {
 public:
  // |region| is the region being scanned for rel32 references.
  explicit Rel32Finder(ConstBufferView region)
      : cursor_(region.begin()),
        next_cursor_(region.begin()),
        end_(region.end()) {}
  virtual ~Rel32Finder() = default;

  // Accept the last reference found. Next call to FindNext() will scan starting
  // beyond that reference, instead of the current search position.
  void Accept() { cursor_ = next_cursor_; }

  // Accessors for unittest.
  ConstBufferView::const_iterator cursor() const { return cursor_; }
  ConstBufferView::const_iterator next_cursor() const { return next_cursor_; }
  ConstBufferView::const_iterator end() const { return end_; }

 protected:
  // Scans for the next rel32 reference. If a reference is found, advances the
  // seach position beyond it and returns true. Otherwise, moves the search
  // position to the end of the region and returns false.
  bool FindNext() {
    next_cursor_ = Scan();
    if (cursor_ == end_)
      return false;
    ++cursor_;
    DCHECK_LE(next_cursor_, end_);
    DCHECK_GE(next_cursor_, cursor_);
    return true;
  }

  // Architecture-specific rel32 reference detection, which scans executable
  // bytes given by |[cursor_, last_)|. For each rel32 reference found, the
  // implementation should cache the necessary data to be retrieved via
  // accessors and return an iterator pointing beyond the reference that was
  // just found, or to the end of the image if no more reference is found, while
  // leaving |cursor_| at the current search position. By default, the next time
  // FindNext() is called, |cursor_| will be incremented by one, unless Accept()
  // was call, in which case |cursor_| will point beyond the last reference.
  virtual ConstBufferView::const_iterator Scan() = 0;

  ConstBufferView::const_iterator cursor_;
  const ConstBufferView::const_iterator end_;

 private:
  ConstBufferView::const_iterator next_cursor_;

  DISALLOW_COPY_AND_ASSIGN(Rel32Finder);
};

// Parsing for X86 or X64: we perform naive scan for opcodes that have rel32 as
// an argument, and disregard instruction alignment.
class Rel32FinderIntel : public Rel32Finder {
 public:
  // Struct to store GetNext() results.
  struct Result {
    ConstBufferView::const_iterator location;

    // Some references must have their target in the same section as location,
    // which we use this to heuristically reject rel32 reference candidates.
    // When true, this constraint is relaxed.
    bool can_point_outside_section;
  };

  using Rel32Finder::Rel32Finder;

  // Returns the next available Result, or nullopt if exhausted.
  base::Optional<Result> GetNext() {
    if (FindNext())
      return rel32_;
    return base::nullopt;
  }

 protected:
  // Cached results.
  Result rel32_;

  // Rel32Finder:
  ConstBufferView::const_iterator Scan() override = 0;

 private:
  DISALLOW_COPY_AND_ASSIGN(Rel32FinderIntel);
};

// X86 instructions.
class Rel32FinderX86 : public Rel32FinderIntel {
 public:
  using Rel32FinderIntel::Rel32FinderIntel;

 private:
  // Rel32Finder:
  ConstBufferView::const_iterator Scan() override;

  DISALLOW_COPY_AND_ASSIGN(Rel32FinderX86);
};

// X64 instructions.
class Rel32FinderX64 : public Rel32FinderIntel {
 public:
  using Rel32FinderIntel::Rel32FinderIntel;

 private:
  // Rel32Finder:
  ConstBufferView::const_iterator Scan() override;

  DISALLOW_COPY_AND_ASSIGN(Rel32FinderX64);
};

}  // namespace zucchini

#endif  // CHROME_INSTALLER_ZUCCHINI_REL32_FINDER_H_
