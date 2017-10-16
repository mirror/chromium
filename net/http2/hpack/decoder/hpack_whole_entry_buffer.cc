// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "net/http2/hpack/decoder/hpack_whole_entry_buffer.h"

#include "base/logging.h"
#include "base/trace_event/memory_usage_estimator.h"

#include <string.h>
#include <algorithm>

namespace net {

namespace {

// Convenience class to dump binary data as human-friendly lines of hexadecimal
// dump, which look like:
//
// 62 3a 90 c1 64 62 33 d9 33 05 91 88 ea 43 04 a0 |b:..db3.3....C..|
// c5 a9 2b 5b 5c 83 49 58 25 06 2d 49 b6 b9 06 92 |..+[\.IX%.-I....|
// ad 5d 87 91 61 da 85 c7 6a 4f 6d 72 0d 25 41 6c |.]..a...jOmr.%Al|
// ee 5b 18 10 1d fa 2d 14 32 0b 8d 34 cf          |.[....-.2..4.   |
//
// Usage is the following:
//
//  1/ Create new HexDumper instance, passing a pointer to the data and
//     its size to the constructor.
//
//  2/ Optional: Call GetLineCount() to get the number of lines of output
//     that will be generated.
//
//  3/ Call HasNextLine() and GetNextLine() to iterate over the generated
//     output.
//
// E.g.:
//      HexDumper dumper(data, len);
//      while (dumper.HasNextLine()) {
//         std::cout << dumper.GetNextLine() << "\n";
//      }
//
class HexDumper {
 public:
  // Default constructor.
  HexDumper() = default;

  // Constructor with explicit |data| and |data_len| parameters.
  HexDumper(const void* data, size_t data_len)
      : data_(static_cast<const char*>(data)),
        data_end_(static_cast<const char*>(data) + data_len) {}

  // Constructor that takes a base::StringPiece, or anything that converts
  // to it.
  HexDumper(base::StringPiece str) : HexDumper(str.data(), str.size()) {}

  // Reset the dumper to another piece of data.
  void Reset(const void* data, size_t data_len) {
    data_ = static_cast<const char*>(data);
    data_end_ = data_ + data_len;
  }

  // Return true iff there is another line of output.
  bool HasNextLine() const { return data_ < data_end_; }

  // Return the number of lines of output remaining.
  size_t GetLineCount() const {
    return (data_end_ - data_ + kMaxDumpWidth - 1) / kMaxDumpWidth;
  }

  // Return the next line as a base::StringPiece. The result will be empty
  // if there is nothing to output, otherwise, it will point to an internal
  // buffer whose content might change on the next GetNextLine() call.
  base::StringPiece GetNextLine() {
    size_t avail =
        std::min(static_cast<size_t>(data_end_ - data_), kMaxDumpWidth);
    if (avail == 0) {
      return {nullptr, 0};
    }
    char* hex_line = line_buffer_;
    char* char_line = line_buffer_ + kMaxDumpWidth * 3 + 1;
    for (size_t n = 0; n < avail; ++n) {
      static const char kHexDigits[] = "0123456789abcdef";
      uint8_t val = static_cast<uint8_t>(data_[n]);
      hex_line[n * 3] = kHexDigits[(val >> 4) & 15];
      hex_line[n * 3 + 1] = kHexDigits[val & 15];
      hex_line[n * 3 + 2] = ' ';
      char_line[n] = (val >= 32 && val < 127) ? static_cast<char>(val) : '.';
    }
    for (size_t n = avail; n < kMaxDumpWidth; ++n) {
      hex_line[n * 3] = ' ';
      hex_line[n * 3 + 1] = ' ';
      hex_line[n * 3 + 2] = ' ';
      char_line[n] = ' ';
    }
    char_line[-1] = '|';
    char_line[kMaxDumpWidth] = '|';
    char_line[kMaxDumpWidth + 1] = '\0';

    data_ += avail;

    return {line_buffer_, kMaxDumpWidth * 4 + 2};
  }

 private:
  static constexpr size_t kMaxDumpWidth = 16;
  const char* data_ = nullptr;
  const char* data_end_ = nullptr;
  char line_buffer_[kMaxDumpWidth * 4 + 3];
};

// Perform a human-friendly hexadecimal dump of an array of bytes.
// |prefix| will be pre-pended to every output line, and cannot be nullptr.
// |data| and |len| describe the input bytes to be dumped.
// This generates a new string that looks like the following (using \n as
// a line separator):
//
// <prefix>62 3a 90 c1 64 62 33 d9 33 05 91 88 ea 43 04 a0 |b:..db3.3....C..|
// <prefix>c5 a9 2b 5b 5c 83 49 58 25 06 2d 49 b6 b9 06 92 |..+[\.IX%.-I....|
// <prefix>ad 5d 87 91 61 da 85 c7 6a 4f 6d 72 0d 25 41 6c |.]..a...jOmr.%Al|
// <prefix>ee 5b 18 10 1d fa 2d 14 32 0b 8d 34 cf          |.[....-.2..4.   |
//
static std::string HexDump(const char* prefix, const char* data, size_t len) {
  std::string result;
  HexDumper dumper(data, len);
  while (dumper.HasNextLine()) {
    auto line = dumper.GetNextLine();
    if (!result.empty()) {
      result.append(1, '\n');
    }
    result.append(prefix);
    result.append(line.data(), line.size());
  }
  return result;
}

}  // namespace

HpackWholeEntryBuffer::HpackWholeEntryBuffer(HpackWholeEntryListener* listener,
                                             size_t max_string_size_bytes)
    : max_string_size_bytes_(max_string_size_bytes) {
  set_listener(listener);
}
HpackWholeEntryBuffer::~HpackWholeEntryBuffer() {}

void HpackWholeEntryBuffer::set_listener(HpackWholeEntryListener* listener) {
  CHECK(listener);
  listener_ = listener;
}

void HpackWholeEntryBuffer::set_max_string_size_bytes(
    size_t max_string_size_bytes) {
  max_string_size_bytes_ = max_string_size_bytes;
}

void HpackWholeEntryBuffer::BufferStringsIfUnbuffered() {
  name_.BufferStringIfUnbuffered();
  value_.BufferStringIfUnbuffered();
}

size_t HpackWholeEntryBuffer::EstimateMemoryUsage() const {
  return base::trace_event::EstimateMemoryUsage(name_) +
         base::trace_event::EstimateMemoryUsage(value_);
}

void HpackWholeEntryBuffer::OnIndexedHeader(size_t index) {
  DVLOG(2) << "HpackWholeEntryBuffer::OnIndexedHeader: index=" << index;
  listener_->OnIndexedHeader(index);
}

void HpackWholeEntryBuffer::OnStartLiteralHeader(HpackEntryType entry_type,
                                                 size_t maybe_name_index) {
  DVLOG(2) << "HpackWholeEntryBuffer::OnStartLiteralHeader: entry_type="
           << entry_type << ",  maybe_name_index=" << maybe_name_index;
  entry_type_ = entry_type;
  maybe_name_index_ = maybe_name_index;
}

void HpackWholeEntryBuffer::OnNameStart(bool huffman_encoded, size_t len) {
  DVLOG(2) << "HpackWholeEntryBuffer::OnNameStart: huffman_encoded="
           << (huffman_encoded ? "true" : "false") << ",  len=" << len;
  DCHECK_EQ(maybe_name_index_, 0u);
  if (!error_detected_) {
    if (len > max_string_size_bytes_) {
      DVLOG(1) << "Name length (" << len << ") is longer than permitted ("
               << max_string_size_bytes_ << ")";
      ReportError("HPACK entry name size is too long.");
      return;
    }
    name_.OnStart(huffman_encoded, len);
  }
}

void HpackWholeEntryBuffer::OnNameData(const char* data, size_t len) {
  DVLOG(2) << "HpackWholeEntryBuffer::OnNameData: len=" << len << "\n"
           << HexDump("data: ", data, len);
  DCHECK_EQ(maybe_name_index_, 0u);
  if (!error_detected_ && !name_.OnData(data, len)) {
    ReportError("Error decoding HPACK entry name.");
  }
}

void HpackWholeEntryBuffer::OnNameEnd() {
  DVLOG(2) << "HpackWholeEntryBuffer::OnNameEnd";
  DCHECK_EQ(maybe_name_index_, 0u);
  if (!error_detected_ && !name_.OnEnd()) {
    ReportError("Error decoding HPACK entry name.");
  }
}

void HpackWholeEntryBuffer::OnValueStart(bool huffman_encoded, size_t len) {
  DVLOG(2) << "HpackWholeEntryBuffer::OnValueStart: huffman_encoded="
           << (huffman_encoded ? "true" : "false") << ",  len=" << len;
  if (!error_detected_) {
    if (len > max_string_size_bytes_) {
      DVLOG(1) << "Value length (" << len << ") is longer than permitted ("
               << max_string_size_bytes_ << ")";
      ReportError("HPACK entry value size is too long.");
      return;
    }
    value_.OnStart(huffman_encoded, len);
  }
}

void HpackWholeEntryBuffer::OnValueData(const char* data, size_t len) {
  DVLOG(2) << "HpackWholeEntryBuffer::OnValueData: len=" << len << "\n"
           << HexDump("data: ", data, len);
  if (!error_detected_ && !value_.OnData(data, len)) {
    ReportError("Error decoding HPACK entry value.");
  }
}

void HpackWholeEntryBuffer::OnValueEnd() {
  DVLOG(2) << "HpackWholeEntryBuffer::OnValueEnd";
  if (error_detected_) {
    return;
  }
  if (!value_.OnEnd()) {
    ReportError("Error decoding HPACK entry value.");
    return;
  }
  if (maybe_name_index_ == 0) {
    listener_->OnLiteralNameAndValue(entry_type_, &name_, &value_);
    name_.Reset();
  } else {
    listener_->OnNameIndexAndLiteralValue(entry_type_, maybe_name_index_,
                                          &value_);
  }
  value_.Reset();
}

void HpackWholeEntryBuffer::OnDynamicTableSizeUpdate(size_t size) {
  DVLOG(2) << "HpackWholeEntryBuffer::OnDynamicTableSizeUpdate: size=" << size;
  listener_->OnDynamicTableSizeUpdate(size);
}

void HpackWholeEntryBuffer::ReportError(Http2StringPiece error_message) {
  if (!error_detected_) {
    DVLOG(1) << "HpackWholeEntryBuffer::ReportError: " << error_message;
    error_detected_ = true;
    listener_->OnHpackDecodeError(error_message);
    listener_ = HpackWholeEntryNoOpListener::NoOpListener();
  }
}

}  // namespace net
