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

class HTMLInputStreamBase {
 public:
  virtual bool Supports16Bit() = 0;

  virtual bool HasInsertionPoint() const = 0;
  virtual bool HaveSeenEndOfFile() const = 0;
  virtual bool CurrentIsEmpty() const = 0;
  virtual String CurrentToString() const = 0;
  virtual void CurrentClear() = 0;
  virtual void InsertAtCurrentInsertionPoint(const String&) = 0;
  virtual void AppendToEnd(const String&) = 0;
  virtual void CloseWithoutMarkingEndOfFile() = 0;
  virtual void MarkEndOfFile() = 0;
  virtual OrdinalNumber CurrentCurrentLine() = 0;
  virtual OrdinalNumber CurrentCurrentColumn() = 0;
};

template <bool supports16bit>
class HTMLInputStream : public HTMLInputStreamBase {
  DISALLOW_NEW_EXCEPT_PLACEMENT_NEW();

 public:
  HTMLInputStream() : last_(&first_) {}

  void AppendToEnd(const String& string) final {
    last_->Append(SegmentedStringImpl<supports16bit>(string));
  }

  void InsertAtCurrentInsertionPoint(const String& string) final {
    SegmentedStringImpl<supports16bit> excluded_line_number_source(string);
    excluded_line_number_source.SetExcludeLineNumbers();
    first_.Append(excluded_line_number_source);
  }

  bool HasInsertionPoint() const final { return &first_ != last_; }

  void MarkEndOfFile() final {
    last_->Append(
        SegmentedStringImpl<supports16bit>(String(&kEndOfFileMarker, 1)));
    last_->Close();
  }

  void CloseWithoutMarkingEndOfFile() final { last_->Close(); }

  bool HaveSeenEndOfFile() const final { return last_->IsClosed(); }

  SegmentedStringImpl<supports16bit>& Current() { return first_; }
  const SegmentedStringImpl<supports16bit>& Current() const { return first_; }

  void SplitInto(SegmentedStringImpl<supports16bit>& next) {
    next = first_;
    first_ = SegmentedStringImpl<supports16bit>();
    if (last_ == &first_) {
      // We used to only have one SegmentedString in the InputStream but now we
      // have two.  That means m_first is no longer also the m_last string,
      // |next| is now the last one.
      last_ = &next;
    }
  }

  void SplitInto(SegmentedStringImpl<!supports16bit>& next) { NOTREACHED(); }

  void MergeFrom(SegmentedStringImpl<supports16bit>& next) {
    first_.Append(next);
    if (last_ == &next) {
      // The string |next| used to be the last SegmentedString in
      // the InputStream.  Now that it's been merged into m_first,
      // that makes m_first the last one.
      last_ = &first_;
    }
    if (next.IsClosed()) {
      // We also need to merge the "closed" state from next to m_first.
      // Arguably, this work could be done in append().
      first_.Close();
    }
  }

  void MergeFrom(SegmentedStringImpl<!supports16bit>& next) { NOTREACHED(); }

  bool Supports16Bit() final { return supports16bit; }

  bool CurrentIsEmpty() const final { return first_.IsEmpty(); }
  String CurrentToString() const final { return first_.ToString(); }
  void CurrentClear() final { return first_.Clear(); }
  OrdinalNumber CurrentCurrentLine() final { return first_.CurrentLine(); }
  OrdinalNumber CurrentCurrentColumn() final { return first_.CurrentColumn(); }

 private:
  SegmentedStringImpl<supports16bit> first_;
  SegmentedStringImpl<supports16bit>* last_;

  DISALLOW_COPY_AND_ASSIGN(HTMLInputStream);
};

class InsertionPointRecord {
  STACK_ALLOCATED();

 public:
  explicit InsertionPointRecord(HTMLInputStreamBase* input_stream)
      : input_stream_(input_stream) {
    if (input_stream_) {
      if (input_stream_->Supports16Bit())
        Init<true>();
      else
        Init<false>();
    }
  }

  template <bool supports16bit>
  void Init() {
    HTMLInputStream<supports16bit>* input_stream =
        static_cast<HTMLInputStream<supports16bit>*>(input_stream_);

    line_ = input_stream->Current().CurrentLine();
    column_ = input_stream->Current().CurrentColumn();

    if (supports16bit) {
      new (&next_16_bit_) SegmentedStringImpl<true>();
      input_stream->SplitInto(next_16_bit_);
    } else {
      new (&next_8_bit_) SegmentedStringImpl<false>();
      input_stream->SplitInto(next_8_bit_);
    }

    // We 'fork' current position and use it for the generated script part. This
    // is a bit weird, because generated part does not have positions within an
    // HTML document.
    input_stream->Current().SetCurrentPosition(line_, column_, 0);
  }

  ~InsertionPointRecord() {
    if (input_stream_) {
      if (input_stream_->Supports16Bit())
        Destroy<true>();
      else
        Destroy<false>();
    }
  }

  template <bool supports16bit>
  void Destroy() {
    HTMLInputStream<supports16bit>* input_stream =
        static_cast<HTMLInputStream<supports16bit>*>(input_stream_);

    // Some inserted text may have remained in input stream. E.g. if script has
    // written "&amp" or "<table", it stays in buffer because it cannot be
    // properly tokenized before we see next part.
    int unparsed_remainder_length = input_stream->Current().length();
    if (supports16bit) {
      input_stream->MergeFrom(next_16_bit_);
      next_16_bit_.~SegmentedStringImpl<true>();
    } else {
      input_stream->MergeFrom(next_8_bit_);
      next_8_bit_.~SegmentedStringImpl<false>();
    }
    // We restore position for the character that goes right after unparsed
    // remainder.
    input_stream->Current().SetCurrentPosition(line_, column_,
                                               unparsed_remainder_length);
  }

 private:
  HTMLInputStreamBase* input_stream_;
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
