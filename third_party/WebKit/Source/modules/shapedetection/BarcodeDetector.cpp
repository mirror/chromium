// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "modules/shapedetection/BarcodeDetector.h"

#include "core/dom/DOMException.h"
#include "core/dom/DOMRect.h"
#include "core/frame/LocalDOMWindow.h"
#include "core/frame/LocalFrame.h"
#include "core/html/canvas/CanvasImageSource.h"
#include "modules/shapedetection/DetectedBarcode.h"
#include "public/platform/InterfaceProvider.h"

namespace blink {

BarcodeDetector* BarcodeDetector::create(Document& document) {
  return new BarcodeDetector(*document.frame());
}

BarcodeDetector::BarcodeDetector(LocalFrame& frame) : ShapeDetector(frame) {
  frame.interfaceProvider()->getInterface(mojo::GetProxy(&m_barcodeService));
  m_barcodeService.set_connection_error_handler(convertToBaseCallback(
      WTF::bind(&BarcodeDetector::onBarcodeServiceConnectionError,
                wrapWeakPersistent(this))));
}

ScriptPromise BarcodeDetector::doDetect(
    ScriptPromiseResolver* resolver,
    mojo::ScopedSharedBufferHandle sharedBufferHandle,
    int imageWidth,
    int imageHeight) {
  ScriptPromise promise = resolver->promise();
  if (!m_barcodeService) {
    resolver->reject(DOMException::create(
        NotSupportedError, "Barcode detection service unavailable."));
    return promise;
  }
  m_barcodeServiceRequests.add(resolver);
  m_barcodeService->Detect(
      std::move(sharedBufferHandle), imageWidth, imageHeight,
      convertToBaseCallback(WTF::bind(&BarcodeDetector::onDetectBarcodes,
                                      wrapPersistent(this),
                                      wrapPersistent(resolver))));
  return promise;
}

void BarcodeDetector::onDetectBarcodes(
    ScriptPromiseResolver* resolver,
    Vector<mojom::blink::BarcodeDetectionResultPtr> barcodeDetectionResults) {
  DCHECK(m_barcodeServiceRequests.contains(resolver));
  m_barcodeServiceRequests.remove(resolver);

  HeapVector<Member<DetectedBarcode>> detectedBarcodes;
  for (const auto& barcode : barcodeDetectionResults) {
    detectedBarcodes.append(DetectedBarcode::create(
        barcode->raw_value,
        DOMRect::create(barcode->bounding_box->x, barcode->bounding_box->y,
                        barcode->bounding_box->width,
                        barcode->bounding_box->height)));
  }

  resolver->resolve(detectedBarcodes);
}

void BarcodeDetector::onBarcodeServiceConnectionError() {
  for (const auto& request : m_barcodeServiceRequests) {
    request->reject(DOMException::create(NotSupportedError,
                                         "Barcode Detection not implemented."));
  }
  m_barcodeServiceRequests.clear();
  m_barcodeService.reset();
}

DEFINE_TRACE(BarcodeDetector) {
  ShapeDetector::trace(visitor);
  visitor->trace(m_barcodeServiceRequests);
}

}  // namespace blink
