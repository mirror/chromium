/*
 * Copyright (C) 2010 Google, Inc. All Rights Reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * THIS SOFTWARE IS PROVIDED BY APPLE INC. ``AS IS'' AND ANY
 * EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
 * IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED.  IN NO EVENT SHALL APPLE INC. OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY
 * OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE
 * OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef HTMLInputStream_h
#define HTMLInputStream_h

#include "base/macros.h"
#include "core/html/parser/InputStreamPreprocessor.h"
#include "platform/text/SegmentedString.h"
#include "platform/wtf/Allocator.h"

namespace blink {

// The InputStream is made up of a sequence of SegmentedStrings:
//
// [--current--][--next--][--next--] ... [--next--]
//            /\                         (also called m_last)
//            L_ current insertion point
//
// The current segmented string is stored in InputStream.  Each of the
// afterInsertionPoint buffers are stored in InsertionPointRecords on the stack.
//
// We remove characters from the "current" string in the InputStream.
// document.write() will add characters at the current insertion point, which
// appends them to the "current" string.
//
// m_last is a pointer to the last of the afterInsertionPoint strings. The
// network adds data at the end of the InputStream, which appends them to the
// "last" string.
class HTMLInputStream {
  DISALLOW_NEW();

 public:
  HTMLInputStream() {
    first_ = &first_8_bit_;
    last_ = first_;
    new (first_) SegmentedStringImpl<false>();
  }

  ~HTMLInputStream() { first_->~SegmentedStringBase(); }

  void AppendToEnd(const String& string) {
    if (string.IsEmpty())
      return;

    if (!last_->Supports16Bit() && !string.Is8Bit())
      ConvertTo16Bit(last_);

    if (last_->Supports16Bit()) {
      static_cast<SegmentedStringImpl<true>*>(last_)->Append(
          SegmentedStringImpl<true>(string));
    } else {
      static_cast<SegmentedStringImpl<false>*>(last_)->Append(
          SegmentedStringImpl<false>(string));
    }
  }

  void InsertAtCurrentInsertionPoint(const String& string) {
    if (string.IsEmpty())
      return;

    if (!first_->Supports16Bit() && !string.Is8Bit())
      ConvertTo16Bit(first_);

    if (first_->Supports16Bit()) {
      SegmentedStringImpl<true> excluded_line_number_source(string);
      excluded_line_number_source.SetExcludeLineNumbers();
      static_cast<SegmentedStringImpl<true>*>(first_)->Append(
          excluded_line_number_source);
    } else {
      SegmentedStringImpl<false> excluded_line_number_source(string);
      excluded_line_number_source.SetExcludeLineNumbers();
      static_cast<SegmentedStringImpl<false>*>(first_)->Append(
          excluded_line_number_source);
    }
  }

  bool HasInsertionPoint() const { return first_ != last_; }

  void MarkEndOfFile() {
    if (last_->Supports16Bit()) {
      SegmentedStringImpl<true>* last_16_bit =
          static_cast<SegmentedStringImpl<true>*>(last_);
      last_16_bit->Append(
          SegmentedStringImpl<true>(String(&kEndOfFileMarker, 1)));
      last_16_bit->Close();
    } else {
      SegmentedStringImpl<false>* last_8_bit =
          static_cast<SegmentedStringImpl<false>*>(last_);
      last_8_bit->Append(
          SegmentedStringImpl<false>(String(&kEndOfFileMarker, 1)));
      last_8_bit->Close();
    }
  }

  void CloseWithoutMarkingEndOfFile() { last_->Close(); }

  bool HaveSeenEndOfFile() const { return last_->IsClosed(); }

  SegmentedStringBase* Current() { return first_; }
  const SegmentedStringBase* Current() const { return first_; }

  void SplitInto(SegmentedStringBase* next) {
    if (first_->Supports16Bit()) {
      new (next) SegmentedStringImpl<true>(
          *static_cast<SegmentedStringImpl<true>*>(first_));
    } else {
      new (next) SegmentedStringImpl<false>(
          *static_cast<SegmentedStringImpl<false>*>(first_));
    }
    first_->~SegmentedStringBase();
    new (first_) SegmentedStringImpl<false>();
    if (last_ == first_) {
      // We used to only have one SegmentedString in the InputStream but now we
      // have two.  That means m_first is no longer also the m_last string,
      // |next| is now the last one.
      last_ = next;
    }
  }

  void MergeFrom(SegmentedStringBase* next) {
    if (!first_->Supports16Bit() && next->Supports16Bit())
      ConvertTo16Bit(first_);

    if (first_->Supports16Bit()) {
      static_cast<SegmentedStringImpl<true>*>(first_)->Append(
          *static_cast<SegmentedStringImpl<true>*>(next));
    } else {
      static_cast<SegmentedStringImpl<false>*>(first_)->Append(
          *static_cast<SegmentedStringImpl<false>*>(next));
    }

    if (last_ == next) {
      // The string |next| used to be the last SegmentedString in
      // the InputStream.  Now that it's been merged into m_first,
      // that makes m_first the last one.
      last_ = first_;
    }
    if (next->IsClosed()) {
      // We also need to merge the "closed" state from next to m_first.
      // Arguably, this work could be done in append().
      first_->Close();
    }
  }

 private:
  // Converts input from a SegmentedStringImpl<false> to
  // SegmentedStringImpl<true>. Assumes the storage is large enough to hold both
  // classes.
  void ConvertTo16Bit(SegmentedStringBase* input) {
    SegmentedStringImpl<false>* string =
        static_cast<SegmentedStringImpl<false>*>(input);
    SegmentedStringImpl<true> string_16_bit(*string);
    new (input) SegmentedStringImpl<true>(string_16_bit);
  }

  union {
    SegmentedStringImpl<false> first_8_bit_;
    SegmentedStringImpl<true> first_16_bit_;
  };
  SegmentedStringBase* first_;
  SegmentedStringBase* last_;

  DISALLOW_COPY_AND_ASSIGN(HTMLInputStream);
};

class InsertionPointRecord {
  STACK_ALLOCATED();

 public:
  explicit InsertionPointRecord(HTMLInputStream& input_stream)
      : input_stream_(&input_stream) {
    next_ = &next_8_bit_;
    auto create_insertion_point = [this](auto& current) {
      line_ = current.CurrentLine();
      column_ = current.CurrentColumn();
      input_stream_->SplitInto(next_);
      // We 'fork' current position and use it for the generated script part.
      // This is a bit weird, because generated part does not have positions
      // within an HTML document.
      current.SetCurrentPosition(line_, column_, 0);
    };

    SegmentedStringBase* current_base = input_stream_->Current();
    if (current_base->Supports16Bit()) {
      create_insertion_point(
          *static_cast<SegmentedStringImpl<true>*>(current_base));
    } else {
      create_insertion_point(
          *static_cast<SegmentedStringImpl<false>*>(current_base));
    }
  }

  ~InsertionPointRecord() {
    auto destroy_insertion_point = [this](auto& current) {
      // Some inserted text may have remained in input stream. E.g. if script
      // has written "&amp" or "<table", it stays in buffer because it cannot be
      // properly tokenized before we see next part.
      int unparsed_remainder_length = current.length();
      input_stream_->MergeFrom(next_);
      // We restore position for the character that goes right after unparsed
      // remainder.
      current.SetCurrentPosition(line_, column_, unparsed_remainder_length);

      next_->~SegmentedStringBase();
    };

    SegmentedStringBase* current_base = input_stream_->Current();
    if (current_base->Supports16Bit()) {
      destroy_insertion_point(
          *static_cast<SegmentedStringImpl<true>*>(current_base));
    } else {
      destroy_insertion_point(
          *static_cast<SegmentedStringImpl<false>*>(current_base));
    }
  }

 private:
  HTMLInputStream* input_stream_;

  SegmentedStringBase* next_;
  union {
    SegmentedStringImpl<false> next_8_bit_;
    SegmentedStringImpl<true> next_16_bit_;
  };

  OrdinalNumber line_;
  OrdinalNumber column_;

  DISALLOW_COPY_AND_ASSIGN(InsertionPointRecord);
};

}  // namespace blink

#endif  // HTMLInputStream_h
