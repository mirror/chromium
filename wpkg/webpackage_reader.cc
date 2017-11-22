// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "wpkg/webpackage_reader.h"

#include "base/logging.h"
#include "content/browser/webauth/cbor/cbor_binary.h"
#include "content/browser/webauth/cbor/cbor_reader.h"
#include "net/http2/hpack/decoder/hpack_decoder.h"

namespace wpkg {

class WpkHpackDecoderListener : public net::HpackDecoderNoOpListener {
 public:
  ~WpkHpackDecoderListener() override {}

  void OnHeader(net::HpackEntryType type,
                const net::HpackString& name,
                const net::HpackString& value) override {
    LOG(INFO) << "hpack entry type: " << type;  // do we care?
    LOG(INFO) << "name: " << name.ToString() << ", value: " << value.ToString();
    map_.insert(std::make_pair(name.ToString(), value.ToString()));
  }

  const HttpHeaders& map() const { return map_; };

 private:
  HttpHeaders map_;
};

HttpHeaders DecodeHPack(const void* data, size_t len) {
  using namespace net;

  WpkHpackDecoderListener collector;
  constexpr size_t kMaxSize = 4096;
  HpackDecoder decoder(&collector, kMaxSize);

  DecodeBuffer db(static_cast<const char*>(data), len);
  CHECK(decoder.StartDecodingBlock());
  CHECK(decoder.DecodeFragment(&db));
  CHECK(decoder.EndDecodingBlock());

  return collector.map();
}

using namespace content;  // TODO

const char* CBORValueTypeToString(CBORValue::Type type) {
  switch (type) {
#define RETURN_STRING(TYPE)   \
  case CBORValue::Type::TYPE: \
    return #TYPE;
    RETURN_STRING(UNSIGNED);
    RETURN_STRING(BYTE_STRING);
    RETURN_STRING(STRING);
    RETURN_STRING(ARRAY);
    RETURN_STRING(MAP);
    RETURN_STRING(TAG);
    RETURN_STRING(NONE);
#undef RETURN_STRING
  }
  NOTREACHED();
  return "";
}

std::ostream& operator<<(std::ostream& o, const CBORToken& token) {
  if (token.is_incomplete()) {
    o << "CBORToken[incomplete]";
  } else if (token.has_parse_error()) {
    o << "CBORToken[parse error]";
  } else {
    o << "CBORToken[type: " << CBORValueTypeToString(token.type())
      << ", value: " << token.value() << "]";
  }
  return o;
}

WebPackageReaderClient::~WebPackageReaderClient() {}

WebPackageReader::WebPackageReader(WebPackageReaderClient* client)
    : client_(client) {}

WebPackageReader::~WebPackageReader() {}

void WebPackageReader::ConsumeDataChunk(const void* bytes, size_t size) {
  const uint8_t* ubytes = static_cast<const uint8_t*>(bytes);
  unparsed_bytes_.insert(unparsed_bytes_.end(), ubytes, ubytes + size);

  while (TryConsume())
    ;
}

const char* WebPackageReader::StateToString(State state) {
  switch (state) {
#define RETURN_STRING(STATE) \
  case State::STATE:         \
    return #STATE;
    RETURN_STRING(kInitial);
    RETURN_STRING(kAfterHeader);
    RETURN_STRING(kExpectSectionOffsetsMapKey);
    RETURN_STRING(kExpectSectionOffsetsMapKeyText);
    RETURN_STRING(kExpectSectionOffsetsMapValue);
    RETURN_STRING(kAfterSectionOffsetMap);
    RETURN_STRING(kSkipUntilManifestSection);
    RETURN_STRING(kExpectManifestSectionHeader);
    RETURN_STRING(kSkipUntilIndexedContentSection);
    RETURN_STRING(kParseManifestSectionMap);
    RETURN_STRING(kExpectIndexedContentSectionHeader);
    RETURN_STRING(kExpectIndexArray);
    RETURN_STRING(kExpectIndexArrayEntry);
    RETURN_STRING(kExpectIndexArrayEntryHttpHeader);
    RETURN_STRING(kExpectIndexArrayEntryHttpHeaderBytes);
    RETURN_STRING(kExpectIndexArrayEntryOffset);
    RETURN_STRING(kAfterIndexArray);
    RETURN_STRING(kSkipUntilNextResponse);
    RETURN_STRING(kExpectResponseArray);
    RETURN_STRING(kExpectResponseHeader);
    RETURN_STRING(kExpectResponseHeaderBytes);
    RETURN_STRING(kExpectResponseBody);
    RETURN_STRING(kExpectResponseBodyBytes);
    RETURN_STRING(kDone);
    // RETURN_STRING(kExpectSectionMapKey);
    // RETURN_STRING(kExpectSectionMapKeyText);
    // RETURN_STRING(kExpectSectionMapValue);
    RETURN_STRING(kError);
#undef RETURN_STRING
  }
  NOTREACHED();
  return "";
}

void WebPackageReader::AdvanceState(State new_state) {
  LOG(INFO) << "Update state: " << StateToString(state_) << " -> "
            << StateToString(new_state);
  state_ = new_state;
}

void WebPackageReader::AdvanceErrorState(const std::string& message) {
  error_string_ = message;
  AdvanceState(State::kError);
}

base::Optional<uint8_t> WebPackageReader::PeekByte() {
  if (unparsed_bytes_.empty())
    return base::nullopt;

  return base::Optional<uint8_t>(unparsed_bytes_.front());
}

const uint8_t* WebPackageReader::TryConsumeBytes(size_t size) {
  if (size == 0u)
    return reinterpret_cast<uint8_t*>(0x1);  // TODO AAAaargh

  if (unparsed_bytes_.size() < size)
    return nullptr;

  contiguous_bytes_.clear();
  contiguous_bytes_.reserve(size);
  auto it = unparsed_bytes_.begin();
  for (size_t i = 0; i < size; ++i) {
    contiguous_bytes_.push_back(*it++);
  }
  unparsed_bytes_.erase(unparsed_bytes_.begin(), it);
  consumed_bytes_ += size;
  return contiguous_bytes_.data();
}

bool WebPackageReader::TrySkipBytes(size_t size) {
  // TODO optimize
  return TryConsumeBytes(size) != nullptr;
}

CBORToken WebPackageReader::ParseCBORToken() {
  auto maybe_item_start = PeekByte();
  if (!maybe_item_start.has_value())
    return CBORToken(CBORToken::incomplete_tag_t{});

  uint8_t item_start = *maybe_item_start;
  CBORValue::Type type =
      static_cast<CBORValue::Type>(item_start >> impl::kMajorTypeBitShift);
  // TODO bound chk

  const size_t additional_length_encoded =
      (item_start & impl::kAdditionalInformationDataMask);
  if (additional_length_encoded < impl::kAdditionalInformation1Byte) {
    uint64_t inline_value = static_cast<uint64_t>(additional_length_encoded);
    CHECK(TryConsumeBytes(1));  // must succeed as PeekByte succeeded.
    return CBORToken(type, inline_value);
  }

  size_t additional_length;
  switch (additional_length_encoded) {
    case impl::kAdditionalInformation1Byte:
      additional_length = 1;
      break;
    case impl::kAdditionalInformation2Bytes:
      additional_length = 2;
      break;
    case impl::kAdditionalInformation4Bytes:
      additional_length = 4;
      break;
    case impl::kAdditionalInformation8Bytes:
      additional_length = 8;
      break;
    default:
      LOG(INFO) << "invalid length encoding";
      return CBORToken(CBORToken::error_tag_t{});
  }

  constexpr size_t kItemStartByteLength = 1;
  size_t token_length = kItemStartByteLength + additional_length;
  DCHECK_GT(token_length, 0u);
  DCHECK_LE(token_length, 8u);
  const uint8_t* token_bytes = TryConsumeBytes(token_length);
  if (!token_bytes)
    return CBORToken(CBORToken::incomplete_tag_t{});

  uint64_t value = 0;
  for (size_t i = 0; i < additional_length; ++i) {
    value = (value << 8) | token_bytes[i + kItemStartByteLength];
  }
  return CBORToken(type, value);
}

bool WebPackageReader::TryConsume() {
  switch (state_) {
    case State::kInitial: {
      constexpr uint8_t kHeaderBytes[] = {0x85, 0x48, 0xf0, 0x9f, 0x8c,
                                          0x90, 0xf0, 0x9f, 0x93, 0xa6};
      const uint8_t* actual = TryConsumeBytes(sizeof(kHeaderBytes));
      if (!actual)
        return false;

      if (memcmp(actual, kHeaderBytes, sizeof(kHeaderBytes)) != 0) {
        error_string_ = "Unexpected header bytes";
        AdvanceState(State::kError);
        return false;
      }

      AdvanceState(State::kAfterHeader);
      return true;
    }
    case State::kAfterHeader: {
      CBORToken token = ParseCBORToken();
      if (token.is_incomplete())
        return false;

      LOG(INFO) << "after header token: " << token;

      if (token.has_parse_error()) {
        AdvanceErrorState("Failed to parse token");
        return false;
      }

      if (token.type() != CBORValue::Type::MAP) {
        AdvanceErrorState("Expected section-offsets map");
        return false;
      }

      entries_left_ = token.value();
      // TODO bound chk
      AdvanceState(State::kExpectSectionOffsetsMapKey);
      return true;
    }
    case State::kExpectSectionOffsetsMapKey: {
      CBORToken token = ParseCBORToken();
      if (token.is_incomplete())
        return false;

      if (token.type() != CBORValue::Type::STRING) {
        AdvanceErrorState("Expected section-offsets map key string");
        return false;
      }

      expected_blob_len_ = token.value();
      AdvanceState(State::kExpectSectionOffsetsMapKeyText);
      return true;
    }
    case State::kExpectSectionOffsetsMapKeyText: {
      const uint8_t* text_bytes = TryConsumeBytes(expected_blob_len_);
      if (!text_bytes)
        return false;

      key_text_ = std::string(reinterpret_cast<const char*>(text_bytes),
                              expected_blob_len_);
      AdvanceState(State::kExpectSectionOffsetsMapValue);
      return true;
    }
    case State::kExpectSectionOffsetsMapValue: {
      CBORToken token = ParseCBORToken();
      if (token.is_incomplete())
        return false;

      uint64_t offset = token.value();
      LOG(INFO) << "section: " << key_text_ << " offset: " << offset;
      if (key_text_ == "manifest")
        manifest_section_offset_ = token.value();
      else if (key_text_ == "indexed-content")
        indexed_content_section_offset_ = token.value();

      --entries_left_;
      AdvanceState(entries_left_ ? State::kExpectSectionOffsetsMapKey
                                 : State::kAfterSectionOffsetMap);
      return true;
    }
    case State::kAfterSectionOffsetMap: {
      CBORToken token = ParseCBORToken();
      if (token.is_incomplete())
        return false;

      if (token.has_parse_error()) {
        AdvanceErrorState("Failed to parse token");
        return false;
      }

      if (token.type() != CBORValue::Type::MAP) {
        AdvanceErrorState("Expected sections map");
        return false;
      }

      CHECK_GT(manifest_section_offset_, 0u);
      CHECK_GT(indexed_content_section_offset_, 0u);

      sections_origin_ = consumed_bytes_ - 1;
      AdvanceState(State::kSkipUntilManifestSection);
      return true;
    }
    case State::kSkipUntilManifestSection: {
      size_t bytes_to_skip =
          manifest_section_offset_ - (consumed_bytes_ - sections_origin_);
      if (!TrySkipBytes(bytes_to_skip))
        return false;

      AdvanceState(State::kExpectManifestSectionHeader);
      return true;
    }
    case State::kExpectManifestSectionHeader: {
      constexpr uint8_t kManifestSectionHeaderBytes[] = {
          0x68,  // CBORToken[type: kString, value: strlen("indexed-content")]
          'm',  'a', 'n', 'i', 'f', 'e', 's', 't',
      };
      const uint8_t* actual =
          TryConsumeBytes(sizeof(kManifestSectionHeaderBytes));
      if (!actual)
        return false;

      if (memcmp(actual, kManifestSectionHeaderBytes,
                 sizeof(kManifestSectionHeaderBytes)) != 0) {
        error_string_ = "Unexpected header bytes";
        AdvanceState(State::kError);
        return false;
      }

      AdvanceState(State::kParseManifestSectionMap);
      return true;
    }
    case State::kParseManifestSectionMap: {
      size_t bytes_to_read = indexed_content_section_offset_ -
                             (consumed_bytes_ - sections_origin_);
      if (!TryConsumeBytes(bytes_to_read))
        return false;

      CBORReader::CBORDecoderError error_code;
      auto maybe_value =
          CBORReader::Read(contiguous_bytes_ /* TODO */, &error_code);
      if (!maybe_value) {
        LOG(FATAL) << "cbor decode err: "
                   << CBORReader::ErrorCodeToString(error_code);
        CHECK(false);
      }
      const auto& value = *maybe_value;
      const auto& origin_value = value.GetMap()
                                     .find("manifest")
                                     ->second.GetMap()
                                     .find("metadata")
                                     ->second.GetMap()
                                     .find("origin")
                                     ->second;
      origin_ = origin_value.GetString();
      client_->OnOrigin(origin_);

      AdvanceState(State::kSkipUntilIndexedContentSection);
      return true;
    }
    case State::kSkipUntilIndexedContentSection: {
      size_t bytes_to_skip = indexed_content_section_offset_ -
                             (consumed_bytes_ - sections_origin_);
      if (!TrySkipBytes(bytes_to_skip))
        return false;

      AdvanceState(State::kExpectIndexedContentSectionHeader);
      return true;
    }
    case State::kExpectIndexedContentSectionHeader: {
      constexpr uint8_t kIndexedContentSectionHeaderBytes[] = {
          0x6f,  // CBORToken[type: kString, value: strlen("indexed-content")]
          'i',  'n', 'd', 'e', 'x', 'e', 'd', '-',
          'c',  'o', 'n', 't', 'e', 'n', 't',
          0x82,  // CBORToken[type: kArray, value: 2]
      };
      const uint8_t* actual =
          TryConsumeBytes(sizeof(kIndexedContentSectionHeaderBytes));
      if (!actual)
        return false;

      if (memcmp(actual, kIndexedContentSectionHeaderBytes,
                 sizeof(kIndexedContentSectionHeaderBytes)) != 0) {
        error_string_ = "Unexpected header bytes";
        AdvanceState(State::kError);
        return false;
      }

      AdvanceState(State::kExpectIndexArray);
      return true;
    }
    case State::kExpectIndexArray: {
      CBORToken token = ParseCBORToken();
      if (token.is_incomplete())
        return false;

      if (token.has_parse_error()) {
        AdvanceErrorState("Failed to parse token");
        return false;
      }

      if (token.type() != CBORValue::Type::ARRAY) {
        AdvanceErrorState("Expected index array");
        return false;
      }

      entries_left_ = token.value();
      parsed_index_entries_.reserve(entries_left_);
      // TODO bound check
      AdvanceState(State::kExpectIndexArrayEntry);
      return true;
    }
    case State::kExpectIndexArrayEntry: {
      CBORToken token = ParseCBORToken();
      if (token.is_incomplete())
        return false;

      if (token.has_parse_error()) {
        AdvanceErrorState("Failed to parse token");
        return false;
      }

      if (token.type() != CBORValue::Type::ARRAY) {
        AdvanceErrorState("Expected index array entry array");
        return false;
      }

      CHECK_NE(token.value(), 3u) << "Implement case where size is also stored";
      CHECK_EQ(token.value(), 2u);

      AdvanceState(State::kExpectIndexArrayEntryHttpHeader);
      return true;
    }
    case State::kExpectIndexArrayEntryHttpHeader: {
      CBORToken token = ParseCBORToken();
      if (token.is_incomplete())
        return false;

      if (token.has_parse_error()) {
        AdvanceErrorState("Failed to parse token");
        return false;
      }

      if (token.type() != CBORValue::Type::BYTE_STRING) {
        AdvanceErrorState("Expected index array entry http header bytestring");
        return false;
      }

      expected_blob_len_ = token.value();
      AdvanceState(State::kExpectIndexArrayEntryHttpHeaderBytes);
      return true;
    }
    case State::kExpectIndexArrayEntryHttpHeaderBytes: {
      const uint8_t* header_bytes = TryConsumeBytes(expected_blob_len_);
      if (!header_bytes)
        return false;

      pending_parsed_headers_ = DecodeHPack(header_bytes, expected_blob_len_);

      AdvanceState(State::kExpectIndexArrayEntryOffset);
      return true;
    }
    case State::kExpectIndexArrayEntryOffset: {
      CBORToken token = ParseCBORToken();
      if (token.is_incomplete())
        return false;

      if (token.has_parse_error()) {
        AdvanceErrorState("Failed to parse token");
        return false;
      }

      if (token.type() != CBORValue::Type::UNSIGNED) {
        AdvanceErrorState("Expected index array entry offset");
        return false;
      }

      uint64_t offset = token.value();
      parsed_index_entries_.emplace_back(std::move(pending_parsed_headers_),
                                         offset);

      --entries_left_;
      AdvanceState(entries_left_ ? State::kExpectIndexArrayEntry
                                 : State::kAfterIndexArray);
      return true;
    }
    case State::kAfterIndexArray: {
      std::sort(parsed_index_entries_.begin(), parsed_index_entries_.end(),
                [](const WebPackageReader::ParsedIndexEntry& a,
                   const WebPackageReader::ParsedIndexEntry& b) {
                  return a.second < b.second;
                });
      for (const auto& entry : parsed_index_entries_) {
        LOG(INFO) << "content path: " << entry.first.at(":path")
                  << " offset: " << entry.second;
      }
      CHECK(!parsed_index_entries_.empty());

      next_response_to_read_idx_ = 0u;
      response_array_offset_ = consumed_bytes_;
      AdvanceState(State::kSkipUntilNextResponse);
      return true;
    }
    case State::kSkipUntilNextResponse: {
      size_t bytes_to_skip =
          parsed_index_entries_[next_response_to_read_idx_].second -
          (consumed_bytes_ - response_array_offset_);
      if (!TrySkipBytes(bytes_to_skip))
        return false;

      AdvanceState(State::kExpectResponseArray);
      return true;
    }
    case State::kExpectResponseArray: {
      CBORToken token = ParseCBORToken();
      if (token.is_incomplete())
        return false;

      if (token.has_parse_error()) {
        AdvanceErrorState("Failed to parse token");
        return false;
      }

      if (token.type() != CBORValue::Type::ARRAY) {
        AdvanceErrorState("Expected response array");
        return false;
      }

      CHECK_EQ(token.value(), 2u);

      AdvanceState(State::kExpectResponseHeader);
      return true;
    }
    case State::kExpectResponseHeader: {
      CBORToken token = ParseCBORToken();
      if (token.is_incomplete())
        return false;

      if (token.has_parse_error()) {
        AdvanceErrorState("Failed to parse token");
        return false;
      }

      if (token.type() != CBORValue::Type::BYTE_STRING) {
        AdvanceErrorState("Expected response hpack bytestring");
        return false;
      }

      expected_blob_len_ = token.value();
      AdvanceState(State::kExpectResponseHeaderBytes);
      return true;
    }
    case State::kExpectResponseHeaderBytes: {
      const uint8_t* header_bytes = TryConsumeBytes(expected_blob_len_);
      if (!header_bytes)
        return false;

      pending_parsed_headers_ = DecodeHPack(header_bytes, expected_blob_len_);
      AdvanceState(State::kExpectResponseBody);
      return true;
    }
    case State::kExpectResponseBody: {
      CBORToken token = ParseCBORToken();
      if (token.is_incomplete())
        return false;

      if (token.has_parse_error()) {
        AdvanceErrorState("Failed to parse token");
        return false;
      }

      if (token.type() != CBORValue::Type::BYTE_STRING) {
        AdvanceErrorState("Expected response body bytestring");
        return false;
      }

      expected_blob_len_ = token.value();
      AdvanceState(State::kExpectResponseBodyBytes);
      return true;
    }
    case State::kExpectResponseBodyBytes: {
      const uint8_t* bytes = TryConsumeBytes(expected_blob_len_);
      if (!bytes)
        return false;

      const auto& request_headers =
          parsed_index_entries_[next_response_to_read_idx_].first;
      const auto& response_headers = pending_parsed_headers_;
      client_->OnResource(request_headers, response_headers, bytes,
                          expected_blob_len_);

      ++next_response_to_read_idx_;
      const bool unread_response_exists =
          next_response_to_read_idx_ < parsed_index_entries_.size();
      AdvanceState(unread_response_exists ? State::kSkipUntilNextResponse
                                          : State::kDone);
      return true;
    }
    case State::kDone: {
      LOG(INFO) << "****** DONE PARSING ******";
      return false;
    }
    case State::kError:
      NOTREACHED();
  }
  NOTREACHED();
  return false;
}

void WebPackageReader::ConsumeEOS() {
  client_->OnEnd();  // TODO fixme may be too early?
}

}  // namespace wpkg
