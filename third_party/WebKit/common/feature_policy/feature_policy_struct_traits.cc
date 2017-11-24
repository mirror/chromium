// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/WebKit/common/feature_policy/feature_policy_struct_traits.h"

#include "third_party/WebKit/common/feature_policy/feature_policy.h"
#include "url/mojo/origin_struct_traits.h"

namespace mojo {

blink::mojom::FeaturePolicyFeature EnumTraits<
    blink::mojom::FeaturePolicyFeature,
    blink::FeaturePolicyFeature>::ToMojom(blink::FeaturePolicyFeature feature) {
  switch (feature) {
    case blink::FeaturePolicyFeature::kNotFound:
      return blink::mojom::FeaturePolicyFeature::kNotFound;
    case blink::FeaturePolicyFeature::kCamera:
      return blink::mojom::FeaturePolicyFeature::kCamera;
    case blink::FeaturePolicyFeature::kEncryptedMedia:
      return blink::mojom::FeaturePolicyFeature::kEncryptedMedia;
    case blink::FeaturePolicyFeature::kFullscreen:
      return blink::mojom::FeaturePolicyFeature::kFullscreen;
    case blink::FeaturePolicyFeature::kGeolocation:
      return blink::mojom::FeaturePolicyFeature::kGeolocation;
    case blink::FeaturePolicyFeature::kMicrophone:
      return blink::mojom::FeaturePolicyFeature::kMicrophone;
    case blink::FeaturePolicyFeature::kMidiFeature:
      return blink::mojom::FeaturePolicyFeature::kMidiFeature;
    case blink::FeaturePolicyFeature::kPayment:
      return blink::mojom::FeaturePolicyFeature::kPayment;
    case blink::FeaturePolicyFeature::kSpeaker:
      return blink::mojom::FeaturePolicyFeature::kSpeaker;
    case blink::FeaturePolicyFeature::kVibrate:
      return blink::mojom::FeaturePolicyFeature::kVibrate;
    case blink::FeaturePolicyFeature::kDocumentCookie:
      return blink::mojom::FeaturePolicyFeature::kDocumentCookie;
    case blink::FeaturePolicyFeature::kDocumentDomain:
      return blink::mojom::FeaturePolicyFeature::kDocumentDomain;
    case blink::FeaturePolicyFeature::kDocumentWrite:
      return blink::mojom::FeaturePolicyFeature::kDocumentWrite;
    case blink::FeaturePolicyFeature::kSyncScript:
      return blink::mojom::FeaturePolicyFeature::kSyncScript;
    case blink::FeaturePolicyFeature::kSyncXHR:
      return blink::mojom::FeaturePolicyFeature::kSyncXHR;
    case blink::FeaturePolicyFeature::kUsb:
      return blink::mojom::FeaturePolicyFeature::kUsb;
    case blink::FeaturePolicyFeature::kAccessibilityEvents:
      return blink::mojom::FeaturePolicyFeature::kAccessibilityEvents;
    case blink::FeaturePolicyFeature::kWebVr:
      return blink::mojom::FeaturePolicyFeature::kWebVr;
  }
  NOTREACHED();
  return static_cast<blink::mojom::FeaturePolicyFeature>(feature);
}

bool EnumTraits<blink::mojom::FeaturePolicyFeature,
                blink::FeaturePolicyFeature>::
    FromMojom(blink::mojom::FeaturePolicyFeature in,
              blink::FeaturePolicyFeature* out) {
  switch (in) {
    case blink::mojom::FeaturePolicyFeature::kNotFound:
      *out = blink::FeaturePolicyFeature::kNotFound;
      return true;
    case blink::mojom::FeaturePolicyFeature::kCamera:
      *out = blink::FeaturePolicyFeature::kCamera;
      return true;
    case blink::mojom::FeaturePolicyFeature::kEncryptedMedia:
      *out = blink::FeaturePolicyFeature::kEncryptedMedia;
      return true;
    case blink::mojom::FeaturePolicyFeature::kFullscreen:
      *out = blink::FeaturePolicyFeature::kFullscreen;
      return true;
    case blink::mojom::FeaturePolicyFeature::kGeolocation:
      *out = blink::FeaturePolicyFeature::kGeolocation;
      return true;
    case blink::mojom::FeaturePolicyFeature::kMicrophone:
      *out = blink::FeaturePolicyFeature::kMicrophone;
      return true;
    case blink::mojom::FeaturePolicyFeature::kMidiFeature:
      *out = blink::FeaturePolicyFeature::kMidiFeature;
      return true;
    case blink::mojom::FeaturePolicyFeature::kPayment:
      *out = blink::FeaturePolicyFeature::kPayment;
      return true;
    case blink::mojom::FeaturePolicyFeature::kSpeaker:
      *out = blink::FeaturePolicyFeature::kSpeaker;
      return true;
    case blink::mojom::FeaturePolicyFeature::kVibrate:
      *out = blink::FeaturePolicyFeature::kVibrate;
      return true;
    case blink::mojom::FeaturePolicyFeature::kDocumentCookie:
      *out = blink::FeaturePolicyFeature::kDocumentCookie;
      return true;
    case blink::mojom::FeaturePolicyFeature::kDocumentDomain:
      *out = blink::FeaturePolicyFeature::kDocumentDomain;
      return true;
    case blink::mojom::FeaturePolicyFeature::kDocumentWrite:
      *out = blink::FeaturePolicyFeature::kDocumentWrite;
      return true;
    case blink::mojom::FeaturePolicyFeature::kSyncScript:
      *out = blink::FeaturePolicyFeature::kSyncScript;
      return true;
    case blink::mojom::FeaturePolicyFeature::kSyncXHR:
      *out = blink::FeaturePolicyFeature::kSyncXHR;
      return true;
    case blink::mojom::FeaturePolicyFeature::kUsb:
      *out = blink::FeaturePolicyFeature::kUsb;
      return true;
    case blink::mojom::FeaturePolicyFeature::kAccessibilityEvents:
      *out = blink::FeaturePolicyFeature::kAccessibilityEvents;
      return true;
    case blink::mojom::FeaturePolicyFeature::kWebVr:
      *out = blink::FeaturePolicyFeature::kWebVr;
      return true;
  }

  return false;
}

using Traits =
    StructTraits<blink::mojom::ParsedFeaturePolicyDeclarationDataView,
                 blink::ParsedFeaturePolicyDeclaration>;

bool Traits::Read(blink::mojom::ParsedFeaturePolicyDeclarationDataView in,
                  blink::ParsedFeaturePolicyDeclaration* out) {
  out->matches_all_origins = in.matches_all_origins();

  if (!in.ReadOrigins(&out->origins) || !in.ReadFeature(&out->feature))
    return false;

  return true;
}

}  // namespace mojo
