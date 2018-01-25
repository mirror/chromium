// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/shape_detection/face_detection_impl_mac_vision.h"

#include <utility>

#include "base/logging.h"
#include "base/strings/sys_string_conversions.h"
#include "services/shape_detection/detection_utils_mac.h"

namespace shape_detection {

namespace {

gfx::RectF ConvertCoordinate(CGRect scaled_bounds, int width, int height) {
  // The result of bounding box is a ratio of image size which is less than one.
  CGRect bounds = CGRectMake(
      scaled_bounds.origin.x * width, scaled_bounds.origin.y * height,
      scaled_bounds.size.width * width, scaled_bounds.size.height * height);

  // In the default Core Graphics coordinate space, the origin is located
  // in the lower-left corner, and thus |ci_image| is flipped vertically.
  // We need to adjust |y| coordinate of bounding box before sending it.
  return gfx::RectF(bounds.origin.x,
                    height - bounds.origin.y - bounds.size.height,
                    bounds.size.width, bounds.size.height);
}
}

FaceDetectionImplMacVision::FaceDetectionImplMacVision() {
  Class rectangle_request_class =
      NSClassFromString(@"VNDetectFaceRectanglesRequest");
  if (rectangle_request_class)
    rectangle_request_.reset([[rectangle_request_class alloc] init]);
}

FaceDetectionImplMacVision::~FaceDetectionImplMacVision() = default;

void FaceDetectionImplMacVision::Detect(const SkBitmap& bitmap,
                                        DetectCallback callback) {
  if (!PerformFaceRequest(bitmap)) {
    std::move(callback).Run({});
    return;
  }

  std::vector<mojom::FaceDetectionResultPtr> results;
  for (VNFaceObservation* const observation in rectangle_request_.get()
           .results) {
    auto face = shape_detection::mojom::FaceDetectionResult::New();
    face->bounding_box = ConvertCoordinate(observation.boundingBox,
                                           bitmap.width(), bitmap.height());

    results.push_back(std::move(face));
  }
  std::move(callback).Run(std::move(results));
}

bool FaceDetectionImplMacVision::PerformFaceRequest(const SkBitmap& bitmap) {
  Class image_handler_class = NSClassFromString(@"VNImageRequestHandler");
  if (!image_handler_class) {
    DLOG(ERROR) << "Failed to create VNImageRequestHandler";
    return false;
  }

  base::scoped_nsobject<CIImage> ci_image = CreateCIImageFromSkBitmap(bitmap);
  if (!ci_image) {
    DLOG(ERROR) << "Failed to create image from SkBitmap";
    return false;
  }

  base::scoped_nsobject<VNImageRequestHandler> image_handler(
      [[image_handler_class alloc] initWithCIImage:ci_image options:@{}]);
  if (!image_handler) {
    DLOG(ERROR) << "Failed to create image request handler";
    return false;
  }

  NSError* ns_error = nil;
  if (!
      [image_handler performRequests:@[ rectangle_request_ ] error:&ns_error]) {
    DLOG(ERROR) << base::SysNSStringToUTF8([ns_error localizedDescription]);
    return false;
  }
  return true;
}

}  // namespace shape_detection
