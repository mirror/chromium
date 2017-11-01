// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/gfx/icc_profile.h"

#include <list>
#include <set>

#include "base/command_line.h"
#include "base/containers/mru_cache.h"
#include "base/lazy_instance.h"
#include "base/metrics/histogram_macros.h"
#include "base/synchronization/lock.h"
#include "third_party/skia/include/core/SkColorSpaceXform.h"
#include "third_party/skia/include/core/SkICC.h"
#include "ui/gfx/skia_color_space_util.h"

namespace gfx {

namespace {

// Profile cache that exist for power reasons on macOS. The motivation is
// as follows:
//  * We specify the color space that an IOSurface is to use via a raw ICC
//    profile (as a CFDataRef).
//  * The WindowServer will consume that profile when it does compositing.
//  * If an IOSurface exactly matches (byte for byte) the ICC profile of the
//    screen it is being composited on, then color conversion is skipped, and
//    power drops substantially.
//  * Conversely, if we don't have an exact match (even if the ICC profile we
//    specify has the same parametric representation), power use increases.
// The usage of the cache is as follows:
//  * This cache is updated whenever we
//     * Retrieve an ICCProfile from the operating system.
//     * Send or receive an ICCProfile over IPC. In particular, when the
//     renderer
//       receives a content::ScreenInfo.
//  * This cache is queried in IOSurfaceSetColorSpace.
using ProfileCacheBase =
    base::MRUCache<gfx::ColorSpace, scoped_refptr<ICCProfile>>;
class ProfileCache : public ProfileCacheBase {
 public:
  static const size_t kMaxCachedICCProfiles = 16;
  ProfileCache() : ProfileCacheBase(kMaxCachedICCProfiles) {}
};
base::LazyInstance<ProfileCache>::DestructorAtExit g_display_cache =
    LAZY_INSTANCE_INITIALIZER;

using DataToProfileMap = std::map<std::vector<uint8_t>, ICCProfile*>;

// A global map from data to ICCProfile objects.
base::LazyInstance<DataToProfileMap>::DestructorAtExit g_data_to_profile_map =
    LAZY_INSTANCE_INITIALIZER;

// A lock that must be held while accessing the above map and histogram data.
base::LazyInstance<base::Lock>::DestructorAtExit g_lock =
    LAZY_INSTANCE_INITIALIZER;

}  // namespace

ICCProfile::AnalyzeResult ICCProfile::Initialize() {
  // Start out with no parametric data.
  primaries_ = gfx::ColorSpace::PrimaryID::INVALID;
  transfer_ = gfx::ColorSpace::TransferID::INVALID;

  // Parse the profile and attempt to create a SkColorSpaceXform out of it.
  sk_sp<SkColorSpace> sk_srgb_color_space = SkColorSpace::MakeSRGB();
  sk_sp<SkICC> sk_icc = SkICC::Make(data_.data(), data_.size());
  if (!sk_icc) {
    DLOG(ERROR) << "Failed to parse ICC profile to SkICC.";
    return kICCFailedToParse;
  }
  sk_color_space_ = SkColorSpace::MakeICC(data_.data(), data_.size());
  if (!sk_color_space_) {
    DLOG(ERROR) << "Failed to parse ICC profile to SkColorSpace.";
    return kICCFailedToExtractSkColorSpace;
  }

  // If our SkColorSpace representation is sRGB then return that.
  if (SkColorSpace::Equals(sk_srgb_color_space.get(), sk_color_space_.get())) {
    primaries_ = gfx::ColorSpace::PrimaryID::BT709;
    transfer_ = gfx::ColorSpace::TransferID::IEC61966_2_1;
    return kICCExtractedSRGBColorSpace;
  }

  std::unique_ptr<SkColorSpaceXform> sk_color_space_xform =
      SkColorSpaceXform::New(sk_srgb_color_space.get(), sk_color_space_.get());
  if (!sk_color_space_xform) {
    DLOG(ERROR) << "Parsed ICC profile but can't create SkColorSpaceXform.";
    return kICCFailedToCreateXform;
  }

  // Because this SkColorSpace can be used to construct a transform, we can use
  // it to create a LUT based color transform, at the very least. If we fail to
  // get any better approximation, we'll use sRGB as our approximation.
  primaries_ = ColorSpace::PrimaryID::ICC_BASED;
  transfer_ = ColorSpace::TransferID::ICC_BASED;
  ColorSpace::CreateSRGB().GetPrimaryMatrix(&to_XYZD50_);
  ColorSpace::CreateSRGB().GetTransferFunction(&transfer_fn_);

  // A primary matrix is required for our parametric representations. Use it if
  // it exists.
  SkMatrix44 to_XYZD50_matrix;
  if (!sk_icc->toXYZD50(&to_XYZD50_matrix)) {
    DLOG(ERROR) << "Failed to extract ICC profile primary matrix.";
    return kICCFailedToExtractMatrix;
  }
  primaries_ = ColorSpace::PrimaryID::CUSTOM;
  to_XYZD50_ = to_XYZD50_matrix;

  // Try to directly extract a numerical transfer function. Use it if it
  // exists.
  SkColorSpaceTransferFn exact_tr_fn;
  if (sk_icc->isNumericalTransferFn(&exact_tr_fn)) {
    transfer_ = ColorSpace::TransferID::CUSTOM;
    transfer_fn_ = exact_tr_fn;
    return kICCExtractedMatrixAndAnalyticTrFn;
  }

  // Attempt to fit a parametric transfer function to the table data in the
  // profile.
  SkColorSpaceTransferFn approx_tr_fn;
  if (!SkApproximateTransferFn(sk_icc, &transfer_fn_error_, &approx_tr_fn)) {
    DLOG(ERROR) << "Failed approximate transfer function.";
    return kICCFailedToConvergeToApproximateTrFn;
  }

  // If this converged, but has too high error, use the sRGB transfer function
  // from above.
  const float kMaxError = 2.f / 256.f;
  if (transfer_fn_error_ >= kMaxError) {
    DLOG(ERROR) << "Failed to accurately approximate transfer function, error: "
                << 256.f * transfer_fn_error_ << "/256";
    return kICCFailedToApproximateTrFnAccurately;
  };

  // If the error is sufficiently low, declare that the approximation is
  // accurate.
  transfer_ = ColorSpace::TransferID::CUSTOM;
  transfer_fn_ = approx_tr_fn;
  return kICCExtractedMatrixAndApproximatedTrFn;
}

bool ICCProfile::IsValid() const {
  switch (analyze_result_) {
    case kICCFailedToParse:
    case kICCFailedToExtractSkColorSpace:
    case kICCFailedToCreateXform:
      return false;
    default:
      break;
  }
  return true;
}

// static
const std::vector<uint8_t>& ICCProfile::GetData() const {
  return data_;
}

ColorSpace ICCProfile::GetColorSpace() {
  ColorSpace color_space;
  if (!IsValid())
    return color_space;
  color_space.icc_profile_ = scoped_refptr<ICCProfile>(this);
  color_space.matrix_ = ColorSpace::MatrixID::RGB;
  color_space.range_ = ColorSpace::RangeID::FULL;
  if (primaries_ == ColorSpace::PrimaryID::CUSTOM ||
      primaries_ == ColorSpace::PrimaryID::ICC_BASED) {
    color_space.SetCustomPrimaries(to_XYZD50_);
  }
  color_space.primaries_ = primaries_;
  if (transfer_ == ColorSpace::TransferID::CUSTOM ||
      transfer_ == ColorSpace::TransferID::ICC_BASED) {
    color_space.SetCustomTransferFunction(transfer_fn_);
  }
  color_space.transfer_ = transfer_;
  return color_space;
}

sk_sp<SkColorSpace> ICCProfile::GetSkColorSpace() const {
  return sk_color_space_;
}

void ICCProfile::HistogramDisplay(int64_t display_id) {
  base::AutoLock lock(g_lock.Get());

  // If we have already histogrammed this display, don't histogram it.
  if (histogrammed_display_ids_.count(display_id))
    return;
  histogrammed_display_ids_.insert(display_id);

  UMA_HISTOGRAM_ENUMERATION("Blink.ColorSpace.Destination.ICCResult",
                            analyze_result_, kICCProfileAnalyzeLast);

  // Add histograms for numerical approximation.
  bool nonlinear_fit_converged =
      analyze_result_ == kICCExtractedMatrixAndApproximatedTrFn ||
      analyze_result_ == kICCFailedToApproximateTrFnAccurately;
  bool nonlinear_fit_did_not_converge =
      analyze_result_ == kICCFailedToConvergeToApproximateTrFn;
  if (nonlinear_fit_converged || nonlinear_fit_did_not_converge) {
    UMA_HISTOGRAM_BOOLEAN("Blink.ColorSpace.Destination.NonlinearFitConverged",
                          nonlinear_fit_converged);
  }
  if (nonlinear_fit_converged) {
    UMA_HISTOGRAM_CUSTOM_COUNTS(
        "Blink.ColorSpace.Destination.NonlinearFitError",
        static_cast<int>(transfer_fn_error_ * 255), 0, 127, 16);
  }
}

// static
scoped_refptr<ICCProfile> ICCProfile::FromData(const void* data_as_void,
                                               size_t size) {
  base::AutoLock lock(g_lock.Get());

  std::vector<uint8_t> data;
  const uint8_t* data_as_byte = reinterpret_cast<const uint8_t*>(data_as_void);
  data.insert(data.begin(), data_as_byte, data_as_byte + size);

  ICCProfile*& icc_profile_raw = g_data_to_profile_map.Get()[data];
  if (icc_profile_raw)
    icc_profile_raw->AddRef();
  else
    icc_profile_raw = new ICCProfile(std::move(data));
  scoped_refptr<ICCProfile> icc_profile(icc_profile_raw);
  return icc_profile;
}

// static
scoped_refptr<ICCProfile> ICCProfile::FromDisplayCache(
    const gfx::ColorSpace& color_space) {
  // See comments at the definition of |g_display_cache|.
  base::AutoLock lock(g_lock.Get());
  auto found = g_display_cache.Get().Get(color_space);
  if (found != g_display_cache.Get().end())
    return found->second;
  return nullptr;
}

void ICCProfile::AddToDisplayCache() {
  base::AutoLock lock(g_lock.Get());
  AddRef();
  g_display_cache.Get().Put(GetColorSpace(), scoped_refptr<ICCProfile>(this));
}

ICCProfile::ICCProfile(std::vector<uint8_t> data) : data_(std::move(data)) {
  analyze_result_ = Initialize();
}

ICCProfile::~ICCProfile() {
  base::AutoLock lock(g_lock.Get());

  auto found = g_data_to_profile_map.Get().find(data_);
  DCHECK(found != g_data_to_profile_map.Get().end());
  DCHECK_EQ(found->second, this);
  g_data_to_profile_map.Get().erase(found);
}

}  // namespace gfx
