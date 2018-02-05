// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/loader/signed_exchange_handler.h"

#include "base/feature_list.h"
#include "components/cbor/cbor_reader.h"
#include "content/browser/loader/merkle_integrity_source_stream.h"
#include "content/public/common/content_features.h"
#include "mojo/public/cpp/system/string_data_pipe_producer.h"
#include "net/base/io_buffer.h"
#include "net/http/http_response_headers.h"
#include "net/http/http_util.h"
#include "services/network/public/cpp/resource_response.h"
#include "services/network/public/cpp/url_loader_completion_status.h"

namespace content {

namespace {

constexpr size_t kBufferSizeForRead = 65536;
constexpr char kHtxg[] = "htxg";
constexpr char kRequest[] = "request";
constexpr char kResponse[] = "response";
constexpr char kPayload[] = "payload";
constexpr char kUrlKey[] = ":url";
constexpr char kMethodKey[] = ":method";
constexpr char kStatusKey[] = ":status";
constexpr char kMiHeader[] = "MI";

cbor::CBORValue BytestringFromString(base::StringPiece in_string) {
  return cbor::CBORValue(
      std::vector<uint8_t>(in_string.begin(), in_string.end()));
}

// TODO(https://crbug.com/803774): Just for now, until we implement streaming
// CBOR parser.
class BufferSourceStream : public net::SourceStream {
 public:
  BufferSourceStream(const std::vector<uint8_t>& bytes)
      : net::SourceStream(SourceStream::TYPE_NONE), buf_(bytes), ptr_(0u) {}
  int Read(net::IOBuffer* dest_buffer,
           int buffer_size,
           const net::CompletionCallback& callback) override {
    int bytes = std::min(static_cast<int>(buf_.size() - ptr_), buffer_size);
    memcpy(dest_buffer->data(), &buf_[ptr_], bytes);
    ptr_ += bytes;
    return bytes;
  }
  std::string Description() const override { return "buffer"; }

 private:
  std::vector<uint8_t> buf_;
  size_t ptr_;
};

}  // namespace

SignedExchangeHandler::SignedExchangeHandler(
    std::unique_ptr<net::SourceStream> upstream,
    ExchangeHeadersCallback headers_callback)
    : headers_callback_(std::move(headers_callback)),
      upstream_(std::move(upstream)),
      weak_factory_(this) {
  DCHECK(base::FeatureList::IsEnabled(features::kSignedHTTPExchange));

  // Triggering the first read (asynchronously) for CBOR parsing.
  read_buf_ = base::MakeRefCounted<net::IOBufferWithSize>(kBufferSizeForRead);
  base::SequencedTaskRunnerHandle::Get()->PostTask(
      FROM_HERE, base::BindOnce(&SignedExchangeHandler::ReadLoop,
                                weak_factory_.GetWeakPtr()));
}

SignedExchangeHandler::~SignedExchangeHandler() = default;

void SignedExchangeHandler::ReadLoop() {
  DCHECK(headers_callback_);
  DCHECK(read_buf_);
  int rv = upstream_->Read(
      read_buf_.get(), read_buf_->size(),
      base::BindRepeating(&SignedExchangeHandler::DidRead,
                          base::Unretained(this), false /* sync */));
  if (rv != net::ERR_IO_PENDING)
    DidRead(true /* sync */, rv);
}

void SignedExchangeHandler::DidRead(bool completed_syncly, int result) {
  CHECK(result >= 0) << "SignedExchangeHandler: read failed: " << result;

  if (result == 0) {
    RunHeadersCallback();
    return;
  }

  original_body_string_.append(read_buf_->data(), result);

  if (completed_syncly) {
    base::SequencedTaskRunnerHandle::Get()->PostTask(
        FROM_HERE, base::BindOnce(&SignedExchangeHandler::ReadLoop,
                                  weak_factory_.GetWeakPtr()));
  } else {
    ReadLoop();
  }
}

bool SignedExchangeHandler::RunHeadersCallback() {
  CHECK(headers_callback_);

  cbor::CBORReader::DecoderError error;
  base::Optional<cbor::CBORValue> root = cbor::CBORReader::Read(
      base::span<const uint8_t>(
          reinterpret_cast<const uint8_t*>(original_body_string_.data()),
          original_body_string_.size()),
      &error);
  CHECK(root) << "CBOR parsing failed: "
              << cbor::CBORReader::ErrorCodeToString(error);
  original_body_string_.clear();

  const auto& root_array = root->GetArray();
  CHECK_EQ(kHtxg, root_array[0].GetString());
  CHECK_EQ(kRequest, root_array[1].GetString());
  const auto& request_map = root_array[2].GetMap();
  // TODO(https://crbug.com/803774): request payload may come here.
  CHECK_EQ(kResponse, root_array[3].GetString());
  const auto& response_map = root_array[4].GetMap();
  CHECK_EQ(kPayload, root_array[5].GetString());
  const auto& payload_bytes = root_array[6].GetBytestring();

  const cbor::CBORValue url_key_value = BytestringFromString(kUrlKey);
  request_url_ =
      GURL(request_map.find(url_key_value)->second.GetBytestringAsString());

  const cbor::CBORValue method_key_value = BytestringFromString(kMethodKey);
  request_method_ = std::string(
      request_map.find(url_key_value)->second.GetBytestringAsString());

  const cbor::CBORValue status_key_value = BytestringFromString(kStatusKey);
  base::StringPiece status_code_str =
      response_map.find(status_key_value)->second.GetBytestringAsString();
  std::string fake_header_str("HTTP/1.1 ");
  status_code_str.AppendToString(&fake_header_str);
  fake_header_str.append(" OK\r\n");
  for (const auto& it : response_map) {
    base::StringPiece name = it.first.GetBytestringAsString();
    base::StringPiece value = it.second.GetBytestringAsString();
    if (name == kMethodKey)
      continue;

    name.AppendToString(&fake_header_str);
    fake_header_str.append(": ");
    value.AppendToString(&fake_header_str);
    fake_header_str.append("\r\n");
  }
  fake_header_str.append("\r\n");
  response_head_.headers = base::MakeRefCounted<net::HttpResponseHeaders>(
      net::HttpUtil::AssembleRawHeaders(fake_header_str.c_str(),
                                        fake_header_str.size()));
  // TODO(https://crbug.com/803774): |mime_type| should be derived from
  // "Content-Type" header.
  response_head_.mime_type = "text/html";

  // TODO(https://crbug.com/803774): Check that the Signature header entry has
  // integrity="mi".
  std::string mi_header_value;
  if (!response_head_.headers->EnumerateHeader(nullptr, kMiHeader,
                                               &mi_header_value)) {
    CHECK(false) << "Signed exchange has no MI: header";
  }
  auto payload_stream = std::make_unique<BufferSourceStream>(payload_bytes);
  auto mi_stream = std::make_unique<MerkleIntegritySourceStream>(
      mi_header_value, std::move(payload_stream));

  std::move(headers_callback_)
      .Run(request_url_, request_method_, response_head_, std::move(mi_stream),
           ssl_info_);

  return true;
}

}  // namespace content
