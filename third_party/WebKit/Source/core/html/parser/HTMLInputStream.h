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
  virtual void SplitInto(SegmentedStringImpl<true>& next) = 0;
  virtual void MergeFrom(SegmentedStringImpl<true>& next) = 0;
  virtual void SplitInto(SegmentedStringImpl<false>& next) = 0;
  virtual void MergeFrom(SegmentedStringImpl<false>& next) = 0;
  virtual void SetCurrentPosition(OrdinalNumber, OrdinalNumber, int) = 0;
  virtual unsigned Length() = 0;
  virtual OrdinalNumber CurrentLine() = 0;
  virtual OrdinalNumber CurrentColumn() = 0;
  virtual bool Is8Bit() = 0;
};

template <bool supports16bit>
class HTMLInputStream : public HTMLInputStreamBase {
  DISALLOW_NEW();
  WTF_MAKE_NONCOPYABLE(HTMLInputStream);

 public:
  HTMLInputStream() : last_(&first_) {}

  void AppendToEnd(const String& string) {
    last_->Append(SegmentedStringImpl<supports16bit>(string));
  }

  void InsertAtCurrentInsertionPoint(const String& string) {
    first_.Append(SegmentedStringImpl<supports16bit>(string));
  }

  bool HasInsertionPoint() const { return &first_ != last_; }

  void MarkEndOfFile() {
    last_->Append(
        SegmentedStringImpl<supports16bit>(String(&kEndOfFileMarker, 1)));
    last_->Close();
  }

  void CloseWithoutMarkingEndOfFile() { last_->Close(); }

  bool HaveSeenEndOfFile() const { return last_->IsClosed(); }

  SegmentedStringImpl<supports16bit>& Current() { return first_; }
  const SegmentedStringImpl<supports16bit>& Current() const { return first_; }

  void SplitInto(SegmentedStringImpl<supports16bit>& next) final {
    next = first_;
    first_ = SegmentedStringImpl<supports16bit>();
    if (last_ == &first_) {
      // We used to only have one SegmentedString in the InputStream but now we
      // have two.  That means m_first is no longer also the m_last string,
      // |next| is now the last one.
      last_ = &next;
    }
  }

  void SplitInto(SegmentedStringImpl<!supports16bit>& next) final {
    NOTREACHED();
  }

  void MergeFrom(SegmentedStringImpl<supports16bit>& next) final {
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

  void MergeFrom(SegmentedStringImpl<!supports16bit>& next) final {
    NOTREACHED();
  }

  void SetCurrentPosition(OrdinalNumber line,
                          OrdinalNumber column_aftre_prolog,
                          int prolog_length) final {
    first_.SetCurrentPosition(line, column_aftre_prolog, prolog_length);
  }

  unsigned Length() final { return first_.length(); }

  OrdinalNumber CurrentLine() final { return first_.CurrentLine(); }

  OrdinalNumber CurrentColumn() final { return first_.CurrentColumn(); }

  bool Is8Bit() final { return !supports16bit; }

 private:
  SegmentedStringImpl<supports16bit> first_;
  SegmentedStringImpl<supports16bit>* last_;
};

template <bool supports16bit>
class InsertionPointRecord {
  STACK_ALLOCATED();
  WTF_MAKE_NONCOPYABLE(InsertionPointRecord);

 public:
  explicit InsertionPointRecord(HTMLInputStreamBase* input_stream)
      : input_stream_(input_stream) {
    line_ = input_stream_->CurrentLine();
    column_ = input_stream_->CurrentColumn();
    input_stream_->SplitInto(next_);
    // We 'fork' current position and use it for the generated script part. This
    // is a bit weird, because generated part does not have positions within an
    // HTML document.
    input_stream_->SetCurrentPosition(line_, column_, 0);
  }

  ~InsertionPointRecord() {
    // Some inserted text may have remained in input stream. E.g. if script has
    // written "&amp" or "<table", it stays in buffer because it cannot be
    // properly tokenized before we see next part.
    int unparsed_remainder_length = input_stream_->Length();
    input_stream_->MergeFrom(next_);
    // We restore position for the character that goes right after unparsed
    // remainder.
    input_stream_->SetCurrentPosition(line_, column_,
                                      unparsed_remainder_length);
  }

 private:
  HTMLInputStreamBase* input_stream_;
  SegmentedStringImpl<supports16bit> next_;
  OrdinalNumber line_;
  OrdinalNumber column_;
};

}  // namespace blink

#endif  // HTMLInputStream_h
