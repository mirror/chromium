// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef WPKG_WEBPACKAGE_READER_H_
#define WPKG_WEBPACKAGE_READER_H_

#include <deque>
#include <map>
#include <string>
#include <utility>
#include <vector>

#include "base/optional.h"
#include "content/browser/webauth/cbor/cbor_values.h"
#include "wpkg/wpkg_export.h"

namespace wpkg {

using HttpHeaders =
    std::map<std::string, std::string>;  // should be better structure for this.
                                         // need to allow dup entries at least

class WPKG_EXPORT WebPackageReaderClient {
 public:
  virtual ~WebPackageReaderClient() = 0;

  // Called when manifest section is done parsing, with manifest.metadata.origin
  // value.
  virtual void OnOrigin(const std::string& origin) = 0;

  // Called as we parse packaged requests in indexed-content section.
  // The client should keep track of request_id to match w/ the response/body
  // notifications called later.
  virtual void OnResourceRequest(const HttpHeaders& request_headers,
                                 int request_id) = 0;

  // Called when new resource response header is done parsing. The header
  // corresponds to the request previously notified in OnResourceRequest w/
  // |request_id|.
  virtual void OnResourceResponse(int request_id,
                                  const HttpHeaders& response_headers) = 0;

  // Called when new resource response body chunk has decoded. The body
  // corresponds to the request previously notified in OnResourceRequest w/
  // |request_id|.
  virtual void OnDataReceived(int request_id,
                              const void* data,
                              size_t size) = 0;

  // Called when new resource response body chunk is complete. The notification
  // corresponds to the request body previously notified in OnResourceRequest w/
  // |request_id|.
  virtual void OnNotifyFinished(int request_id) = 0;

  // Called when entire wpkg has done parsing.
  virtual void OnEnd() = 0;
};

using content::CBORValue;  // TODO

class CBORToken {
 public:
  struct incomplete_tag_t {};
  CBORToken(incomplete_tag_t) : is_incomplete_(true), has_parse_error_(false) {}

  struct error_tag_t {};
  CBORToken(error_tag_t) : is_incomplete_(false), has_parse_error_(true) {}

  CBORToken(CBORValue::Type type, uint64_t value)
      : is_incomplete_(false),
        has_parse_error_(false),
        type_(type),
        value_(value) {}

  CBORValue::Type type() const { return type_; }
  uint64_t value() const { return value_; }

  bool is_incomplete() const { return is_incomplete_; }
  bool has_parse_error() const { return has_parse_error_; }

 private:
  // TODO bitfield?
  const bool is_incomplete_;
  const bool has_parse_error_;
  const CBORValue::Type type_ = CBORValue::Type::UNSIGNED;
  const uint64_t value_ = 0;
};
std::ostream& operator<<(std::ostream&, const CBORToken&);

class WPKG_EXPORT WebPackageReader {
 public:
  WebPackageReader(WebPackageReaderClient* client);
  ~WebPackageReader();

  void ConsumeDataChunk(const void* bytes, size_t size);
  void ConsumeEOS();

 private:
  enum class State {
    kInitial = 0,
    kAfterHeader,
    kExpectSectionOffsetsMapKey,
    kExpectSectionOffsetsMapKeyText,
    kExpectSectionOffsetsMapValue,
    kAfterSectionOffsetMap,
    kSkipUntilManifestSection,
    kExpectManifestSectionHeader,
    kParseManifestSectionMap,
    kSkipUntilIndexedContentSection,
    kExpectIndexedContentSectionHeader,
    kExpectIndexArray,
    kExpectIndexArrayEntry,
    kExpectIndexArrayEntryHttpHeader,
    kExpectIndexArrayEntryHttpHeaderBytes,
    kExpectIndexArrayEntryOffset,
    kAfterIndexArray,
    kSkipUntilNextResponse,
    kExpectResponseArray,
    kExpectResponseHeader,
    kExpectResponseHeaderBytes,
    kExpectResponseBody,
    kExpectResponseBodyBytes,
    kDone,
    kError,
  };

  base::Optional<uint8_t> PeekByte();
  const uint8_t* TryConsumeBytes(size_t size);
  bool TrySkipBytes(size_t size);
  CBORToken ParseCBORToken();
  bool TryConsume();
  static const char* StateToString(State state);
  void AdvanceState(State new_state);
  void AdvanceErrorState(const std::string& message);  // TODO StringPiece

  WebPackageReaderClient* client_;

  State state_ = State::kInitial;
  std::string error_string_;
  std::deque<uint8_t> unparsed_bytes_;  // I really want a SegmentedString
  std::vector<uint8_t> contiguous_bytes_;
  size_t consumed_bytes_ = 0;

  size_t entries_left_ = 0;
  std::string key_text_;
  size_t expected_blob_len_;

  size_t sections_origin_ = 0;
  size_t manifest_section_offset_ = 0;
  size_t indexed_content_section_offset_ = 0;

  std::string origin_;

  HttpHeaders pending_parsed_headers_;
  using ParsedIndexEntry = std::pair<HttpHeaders, uint64_t>;
  std::vector<ParsedIndexEntry> parsed_index_entries_;

  size_t response_array_offset_ = 0;
  size_t next_response_to_read_idx_ = 0;

  DISALLOW_COPY_AND_ASSIGN(WebPackageReader);
};

}  // namespace wpkg

#endif  // WPKG_WEBPACKAGE_READER_H_
