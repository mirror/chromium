#include <stdint.h>
#include <stdio.h>
#include <deque>
#include <vector>
#include <string>

#include "base/logging.h"

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

  const uint8_t* TryConsumeBytes(size_t size);
  bool TryConsume();
  static const char* StateToString(State state);
  void AdvanceState(State new_state);

  WebPackageReaderClient* client_;

  State state_ = State::kInitial;
  std::string error_string_;
  std::deque<uint8_t> unparsed_bytes_; // I really want a SegmentedString
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
  switch(state) {
#define RETURN_STRING(STATE) case State::STATE: return #STATE; 
RETURN_STRING(kInitial);
RETURN_STRING(kAfterHeader);
RETURN_STRING(kError);
#undef RETURN_STRING
  }
}

void WebPackageReader::AdvanceState(State new_state) {
  printf("Update state: %s -> %s\n", StateToString(state_), StateToString(new_state));
  state_ = new_state;
}

const uint8_t* WebPackageReader::TryConsumeBytes(size_t size) {
  if (unparsed_bytes_.size() < size)
    return nullptr;

  contiguous_bytes_.clear();
  contiguous_bytes_.reserve(size);
  auto it = unparsed_bytes_.begin();
  for (size_t i = 0; i < size; ++ i) {
    contiguous_bytes_.push_back(*it++);
  }
  unparsed_bytes_.erase(unparsed_bytes_.begin(), it);
  return contiguous_bytes_.data();
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
        AdvanceState(State::kError);
        error_string_ = "Unexpected header bytes";
        return false;
      }

      AdvanceState(State::kAfterHeader);
      return true;
    }
    case State::kAfterHeader: {
      const uint8_t* arrayStart = TryConsumeBytes(1);
      if (!arrayStart)
        return false;

      *arrayStart;
      
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
  TestWebPackageReaderClient client;
  WebPackageReader reader(&client);

  FILE* fp = fopen(argv[1], "r");
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
