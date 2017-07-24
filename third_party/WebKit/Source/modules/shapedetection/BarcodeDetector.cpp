// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "modules/shapedetection/BarcodeDetector.h"

#include "core/dom/DOMException.h"
#include "core/frame/LocalFrame.h"
#include "core/geometry/DOMRect.h"
#include "core/html/canvas/CanvasImageSource.h"
#include "core/workers/WorkerThread.h"
#include "modules/imagecapture/Point2D.h"
#include "modules/shapedetection/BarcodeDetectorOptions.h"
#include "modules/shapedetection/DetectedBarcode.h"
#include "services/service_manager/public/cpp/interface_provider.h"

namespace blink {

namespace {

shape_detection::mojom::blink::BarcodeFormat ParseBarcodeFormat(
    const String& blink_format) {
  if (blink_format == "aztec")
    return shape_detection::mojom::blink::BarcodeFormat::AZTEC;
  if (blink_format == "code_128")
    return shape_detection::mojom::blink::BarcodeFormat::CODE_128;
  if (blink_format == "code_39")
    return shape_detection::mojom::blink::BarcodeFormat::CODE_39;
  if (blink_format == "code_93")
    return shape_detection::mojom::blink::BarcodeFormat::CODE_93;
  if (blink_format == "codabar")
    return shape_detection::mojom::blink::BarcodeFormat::CODABAR;
  if (blink_format == "data_matrix")
    return shape_detection::mojom::blink::BarcodeFormat::DATA_MATRIX;
  if (blink_format == "ean_13")
    return shape_detection::mojom::blink::BarcodeFormat::EAN_13;
  if (blink_format == "ean_8")
    return shape_detection::mojom::blink::BarcodeFormat::EAN_8;
  if (blink_format == "itf")
    return shape_detection::mojom::blink::BarcodeFormat::ITF;
  if (blink_format == "pdf417")
    return shape_detection::mojom::blink::BarcodeFormat::PDF417;
  if (blink_format == "qr_code")
    return shape_detection::mojom::blink::BarcodeFormat::QR_CODE;
  if (blink_format == "upc_a")
    return shape_detection::mojom::blink::BarcodeFormat::UPC_A;
  if (blink_format == "upc_e")
    return shape_detection::mojom::blink::BarcodeFormat::UPC_E;
  NOTREACHED();
  return shape_detection::mojom::blink::BarcodeFormat::UNKNOWN;
}

WebString ToString(shape_detection::mojom::blink::BarcodeFormat format) {
  switch (format) {
    case shape_detection::mojom::blink::BarcodeFormat::AZTEC:
      return WebString::FromUTF8("aztec");
      break;
    case shape_detection::mojom::blink::BarcodeFormat::CODE_128:
      return WebString::FromUTF8("code_18");
      break;
    case shape_detection::mojom::blink::BarcodeFormat::CODE_39:
      return WebString::FromUTF8("code_39");
      break;
    case shape_detection::mojom::blink::BarcodeFormat::CODE_93:
      return WebString::FromUTF8("code_93");
      break;
    case shape_detection::mojom::blink::BarcodeFormat::CODABAR:
      return WebString::FromUTF8("codabar");
      break;
    case shape_detection::mojom::blink::BarcodeFormat::DATA_MATRIX:
      return WebString::FromUTF8("data_matrix");
      break;
    case shape_detection::mojom::blink::BarcodeFormat::EAN_13:
      return WebString::FromUTF8("ean_13");
      break;
    case shape_detection::mojom::blink::BarcodeFormat::EAN_8:
      return WebString::FromUTF8("ean_8");
      break;
    case shape_detection::mojom::blink::BarcodeFormat::ITF:
      return WebString::FromUTF8("itf");
      break;
    case shape_detection::mojom::blink::BarcodeFormat::PDF417:
      return WebString::FromUTF8("pdf417");
      break;
    case shape_detection::mojom::blink::BarcodeFormat::QR_CODE:
      return WebString::FromUTF8("qr_code");
      break;
    case shape_detection::mojom::blink::BarcodeFormat::UNKNOWN:
      return WebString::FromUTF8("unknown");
      break;
    case shape_detection::mojom::blink::BarcodeFormat::UPC_A:
      return WebString::FromUTF8("upc_a");
      break;
    case shape_detection::mojom::blink::BarcodeFormat::UPC_E:
      return WebString::FromUTF8("upc_e");
      break;
    default:
      NOTREACHED() << "Unknown BarcodeFormat";
  }
  return WebString();
}

}  // anonymous namespace

BarcodeDetector* BarcodeDetector::Create(ExecutionContext* context) {
  return new BarcodeDetector(context, BarcodeDetectorOptions());
}

BarcodeDetector* BarcodeDetector::Create(
    ExecutionContext* context,
    const BarcodeDetectorOptions& options) {
  return new BarcodeDetector(context, options);
}

BarcodeDetector::BarcodeDetector(
    ExecutionContext* context,
    const BarcodeDetectorOptions& constructor_options)
    : ShapeDetector() {
  auto options = shape_detection::mojom::blink::BarcodeDetectorOptions::New();
  if (constructor_options.hasFormats()) {
    for (const auto& barcode_format : constructor_options.formats()) {
      options->formats.push_back(ParseBarcodeFormat(barcode_format));
    }
  }

  auto request = mojo::MakeRequest(&provider_);
  if (context->IsDocument()) {
    LocalFrame* frame = ToDocument(context)->GetFrame();
    if (frame)
      frame->GetInterfaceProvider().GetInterface(std::move(request));
  } else {
    WorkerThread* thread = ToWorkerGlobalScope(context)->GetThread();
    thread->GetInterfaceProvider().GetInterface(std::move(request));
  }

  provider_->EnumerateSupportedFormats(ConvertToBaseCallback(WTF::Bind(
      &BarcodeDetector::OnEnumerateSupportedFormats, WrapPersistent(this))));

  provider_->CreateBarcodeDetection(mojo::MakeRequest(&barcode_service_),
                                    std::move(options));

  barcode_service_.set_connection_error_handler(ConvertToBaseCallback(
      WTF::Bind(&BarcodeDetector::OnBarcodeServiceConnectionError,
                WrapWeakPersistent(this))));
}

ScriptPromise BarcodeDetector::DoDetect(ScriptPromiseResolver* resolver,
                                        skia::mojom::blink::BitmapPtr bitmap) {
  ScriptPromise promise = resolver->Promise();
  if (!barcode_service_) {
    resolver->Reject(DOMException::Create(
        kNotSupportedError, "Barcode detection service unavailable."));
    return promise;
  }
  barcode_service_requests_.insert(resolver);
  barcode_service_->Detect(
      std::move(bitmap), ConvertToBaseCallback(WTF::Bind(
                             &BarcodeDetector::OnDetectBarcodes,
                             WrapPersistent(this), WrapPersistent(resolver))));
  return promise;
}

void BarcodeDetector::OnDetectBarcodes(
    ScriptPromiseResolver* resolver,
    Vector<shape_detection::mojom::blink::BarcodeDetectionResultPtr>
        barcode_detection_results) {
  DCHECK(barcode_service_requests_.Contains(resolver));
  barcode_service_requests_.erase(resolver);

  HeapVector<Member<DetectedBarcode>> detected_barcodes;
  for (const auto& barcode : barcode_detection_results) {
    HeapVector<Point2D> corner_points;
    for (const auto& corner_point : barcode->corner_points) {
      Point2D point;
      point.setX(corner_point.x);
      point.setY(corner_point.y);
      corner_points.push_back(point);
    }
    detected_barcodes.push_back(DetectedBarcode::Create(
        barcode->raw_value,
        DOMRect::Create(barcode->bounding_box.x, barcode->bounding_box.y,
                        barcode->bounding_box.width,
                        barcode->bounding_box.height),
        corner_points));
  }

  resolver->Resolve(detected_barcodes);
}

void BarcodeDetector::OnEnumerateSupportedFormats(
    const Vector<shape_detection::mojom::blink::BarcodeFormat>& formats) {
  for (const auto& format : formats)
    supported_formats_.push_back(ToString(format));
}

void BarcodeDetector::OnBarcodeServiceConnectionError() {
  for (const auto& request : barcode_service_requests_) {
    request->Reject(DOMException::Create(kNotSupportedError,
                                         "Barcode Detection not implemented."));
  }
  barcode_service_requests_.clear();
  barcode_service_.reset();
}

DEFINE_TRACE(BarcodeDetector) {
  ShapeDetector::Trace(visitor);
  visitor->Trace(barcode_service_requests_);
}

}  // namespace blink
