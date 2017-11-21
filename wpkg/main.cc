#include <stdint.h>
#include <stdio.h>
#include <deque>
#include <iostream>
#include <string>
#include <vector>

#include "base/command_line.h"
#include "base/logging.h"
#include "base/optional.h"
#include "content/browser/webauth/cbor/cbor_writer.h"
using namespace content;  // TODO

class CBORToken {
 public:
  struct incomplete_tag_t {};
  CBORToken(incomplete_tag_t) : is_incomplete_(true), has_parse_error_(false) {}

  struct error_tag_t {};
  CBORToken(error_tag_t) : is_incomplete_(false), has_parse_error_(true) {}

  CBORToken(CborMajorType type, uint64_t value)
      : is_incomplete_(false),
        has_parse_error_(false),
        type_(type),
        value_(value) {}

  CborMajorType type() const { return type_; }
  uint64_t value() const { return value_; }

  bool is_incomplete() const { return is_incomplete_; }
  bool has_parse_error() const { return has_parse_error_; }

 private:
  // TODO bitfield?
  const bool is_incomplete_;
  const bool has_parse_error_;
  const CborMajorType type_ = CborMajorType::kUnsigned;
  const uint64_t value_ = 0;
};
std::ostream& operator<<(std::ostream&, const CBORToken&);

const char* CborMajorTypeToString(CborMajorType type) {
  switch (type) {
#define RETURN_STRING(TYPE) \
  case CborMajorType::TYPE: \
    return #TYPE;
    RETURN_STRING(kUnsigned);
    RETURN_STRING(kNegative);
    RETURN_STRING(kByteString);
    RETURN_STRING(kString);
    RETURN_STRING(kArray);
    RETURN_STRING(kMap);
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
    o << "CBORToken[type: " << CborMajorTypeToString(token.type())
      << ", value: " << token.value() << "]";
  }
  return o;
}

class WebPackageReaderClient {
 public:
  virtual ~WebPackageReaderClient();
  virtual void OnEnd() = 0;
};

WebPackageReaderClient::~WebPackageReaderClient() {}

class TestWebPackageReaderClient : public WebPackageReaderClient {
 public:
  TestWebPackageReaderClient() = default;

  void OnEnd() override { printf("==END\n"); }
};

class WebPackageReader {
 public:
  WebPackageReader(WebPackageReaderClient* client);

  void ConsumeDataChunk(const void* bytes, size_t size);
  void ConsumeEOS();

 private:
  enum class State {
    kInitial = 0,
    kAfterHeader,
    kError,
  };

  base::Optional<uint8_t> PeekByte();
  const uint8_t* TryConsumeBytes(size_t size);
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
};

WebPackageReader::WebPackageReader(WebPackageReaderClient* client)
    : client_(client) {}

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
    RETURN_STRING(kError);
#undef RETURN_STRING
  }
  NOTREACHED();
  return "";
}

void WebPackageReader::AdvanceState(State new_state) {
  printf("Update state: %s -> %s\n", StateToString(state_),
         StateToString(new_state));
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
  if (unparsed_bytes_.size() < size)
    return nullptr;

  contiguous_bytes_.clear();
  contiguous_bytes_.reserve(size);
  auto it = unparsed_bytes_.begin();
  for (size_t i = 0; i < size; ++i) {
    contiguous_bytes_.push_back(*it++);
  }
  unparsed_bytes_.erase(unparsed_bytes_.begin(), it);
  return contiguous_bytes_.data();
}

CBORToken WebPackageReader::ParseCBORToken() {
  auto maybe_item_start = PeekByte();
  if (!maybe_item_start.has_value())
    return CBORToken(CBORToken::incomplete_tag_t{});

  uint8_t item_start = *maybe_item_start;
  constexpr unsigned int kCborMajorTypeBitOffset = 5;
  CborMajorType type =
      static_cast<CborMajorType>(item_start >> kCborMajorTypeBitOffset);
  // TODO bound chk

  const size_t additional_length_encoded =
      (item_start & kAdditionalInformationDataMask);
  if (additional_length_encoded < kAdditionalInformation1Byte) {
    uint64_t inline_value = static_cast<uint64_t>(additional_length_encoded);
    CHECK(TryConsumeBytes(1));  // must succeed as PeekByte succeeded.
    return CBORToken(type, inline_value);
  }

  size_t additional_length;
  switch (additional_length_encoded) {
    case kAdditionalInformation1Byte:
      additional_length = 1;
      break;
    case kAdditionalInformation2Bytes:
      additional_length = 2;
      break;
    case kAdditionalInformation4Bytes:
      additional_length = 4;
      break;
    case kAdditionalInformation8Bytes:
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

      if (token.type() != CborMajorType::kMap) {
        AdvanceErrorState("Expected section-offsets map");
        return false;
      }

      return false;
    }
    case State::kError:
      return false;
  }
  NOTREACHED();
}

void WebPackageReader::ConsumeEOS() {
  printf("EOS\n");
  client_->OnEnd();
}

int main(int argc, char* argv[]) {
  base::CommandLine::Init(argc, argv);
  base::CommandLine* cmd = base::CommandLine::ForCurrentProcess();
  base::CommandLine::StringVector args = cmd->GetArgs();
  if (args.size() != 1) {
    std::cerr << argv[0] << " [wpk file]" << std::endl;
    return EXIT_FAILURE;
  }

  TestWebPackageReaderClient client;
  WebPackageReader reader(&client);

  FILE* fp = fopen(args[0].c_str(), "r");
  for (;;) {
    uint8_t buf[4096];
    size_t nread = fread(buf, 1, sizeof(buf), fp);
    if (nread == 0)
      break;
    reader.ConsumeDataChunk(buf, nread);
  }
  reader.ConsumeEOS();
  fclose(fp);
  return 0;
}
