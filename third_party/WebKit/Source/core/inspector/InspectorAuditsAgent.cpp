// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "core/inspector/InspectorAuditsAgent.h"

#include "core/inspector/InspectorNetworkAgent.h"
#include "platform/graphics/ImageBuffer.h"
#include "platform/wtf/text/Base64.h"
#include "public/platform/WebData.h"
#include "public/platform/WebImage.h"
#include "public/platform/WebSize.h"

namespace blink {

using protocol::Response;

namespace {

namespace RequestedEncoding {
static const char kJPEG[] = "jpeg";
static const char kWebP[] = "webp";
}  // namespace RequestedEncoding

// 10000 pixels
static int g_maximum_encode_image_width = 10000;

// 10000 pixels
static int g_maximum_encode_image_height = 10000;

bool EncodeAsImage(char* body,
                   size_t size,
                   const String& encoding,
                   const double quality,
                   Vector<unsigned char>* output) {
  const WebSize maximum_size =
      WebSize(g_maximum_encode_image_width, g_maximum_encode_image_height);
  const WebImage& image = WebImage::FromData(WebData(body, size), maximum_size);
  SkBitmap bitmap = image.GetSkBitmap();
  SkImageInfo info =
      SkImageInfo::Make(bitmap.width(), bitmap.height(), kRGBA_8888_SkColorType,
                        kUnpremul_SkAlphaType);
  size_t row_bytes = info.minRowBytes();
  Vector<char> pixel_storage(info.getSafeSize(row_bytes));
  SkPixmap pixmap(info, pixel_storage.data(), row_bytes);

  if (!SkImage::MakeFromBitmap(bitmap)->readPixels(pixmap, 0, 0)) {
    return false;
  }

  ImageDataBuffer image_to_encode = ImageDataBuffer(
      IntSize(bitmap.width(), bitmap.height()),
      reinterpret_cast<const unsigned char*>(pixel_storage.data()));

  String mime_type = "image/";
  mime_type.append(encoding);
  return image_to_encode.EncodeImage(mime_type, quality, output);
}

}  // namespace

DEFINE_TRACE(InspectorAuditsAgent) {
  visitor->Trace(network_agent_);
  InspectorBaseAgent::Trace(visitor);
}

InspectorAuditsAgent::InspectorAuditsAgent(InspectorNetworkAgent* network_agent)
    : network_agent_(network_agent) {}

InspectorAuditsAgent::~InspectorAuditsAgent() {}

protocol::Response InspectorAuditsAgent::getEncodedResponse(
    const String& request_id,
    const String& encoding,
    const double quality,
    const protocol::Maybe<bool> size_only,
    String* out_body,
    int* out_original_size,
    int* out_encoded_size) {
  DCHECK(encoding == RequestedEncoding::kWebP ||
         encoding == RequestedEncoding::kJPEG);

  String body;
  bool is_base64_encoded;
  String failure_reason;
  if (!network_agent_->FetchResourceContent(
          request_id, &body, &is_base64_encoded, &failure_reason)) {
    return Response::Error(failure_reason);
  }

  Vector<char> base64_decoded_buffer;
  if (!is_base64_encoded || !Base64Decode(body, base64_decoded_buffer) ||
      base64_decoded_buffer.size() == 0) {
    return Response::Error("No image available to encode");
  }

  Vector<unsigned char> encoded_image;
  if (!EncodeAsImage(reinterpret_cast<char*>(base64_decoded_buffer.data()),
                     base64_decoded_buffer.size(), encoding, quality,
                     &encoded_image)) {
    return Response::Error("Could not encode image with given settings");
  }

  *out_body = size_only.fromMaybe(false) ? "" : Base64Encode(encoded_image);
  *out_original_size = static_cast<int>(base64_decoded_buffer.size());
  *out_encoded_size = static_cast<int>(encoded_image.size());
  return Response::OK();
}

}  // namespace blink
