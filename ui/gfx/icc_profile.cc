// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/gfx/icc_profile.h"

#include <list>

#include "base/command_line.h"
#include "base/containers/mru_cache.h"
#include "base/lazy_instance.h"
#include "base/metrics/histogram_macros.h"
#include "base/synchronization/lock.h"
#include "third_party/skia/include/core/SkColorSpaceXform.h"
#include "third_party/skia/include/core/SkICC.h"
#include "ui/gfx/color_space_switches.h"
#include "ui/gfx/skia_color_space_util.h"

namespace gfx {

const uint64_t ICCProfile::test_id_adobe_rgb_ = 1;
const uint64_t ICCProfile::test_id_color_spin_ = 2;
const uint64_t ICCProfile::test_id_generic_rgb_ = 3;
const uint64_t ICCProfile::test_id_srgb_ = 4;
const uint64_t ICCProfile::test_id_no_analytic_tr_fn_ = 5;
const uint64_t ICCProfile::test_id_a2b_only_ = 6;
const uint64_t ICCProfile::test_id_overshoot_ = 7;

namespace {

// Allow keeping around a maximum of 8 cached ICC profiles. Beware that
// we will do a linear search thorugh currently-cached ICC profiles,
// when creating a new ICC profile.
const size_t kMaxCachedICCProfiles = 8;

struct Cache {
  Cache() : id_to_icc_profile_mru(kMaxCachedICCProfiles) {}
  ~Cache() {}

  // Start from-ICC-data IDs at the end of the hard-coded test id list above.
  uint64_t next_unused_id = 10;
  base::MRUCache<uint64_t, ICCProfile> id_to_icc_profile_mru;
  base::Lock lock;
};
static base::LazyInstance<Cache>::DestructorAtExit g_cache =
    LAZY_INSTANCE_INITIALIZER;

// This must match ICCProfileAnalyzeResult enum in histograms.xml.
enum ICCAnalyzeResult {
  kICCExtractedMatrixAndAnalyticTrFn = 0,
  kICCExtractedMatrixAndApproximatedTrFn = 1,
  kICCFailedToApproximateTrFnObsolete = 2,
  kICCFailedToExtractRawTrFn = 3,
  kICCFailedToExtractMatrix = 4,
  kICCFailedToParse = 5,
  kICCFailedToExtractSkColorSpace = 6,
  kICCFailedToCreateXform = 7,
  kICCFailedToConvergeToApproximateTrFn = 8,
  kICCFailedToApproximateTrFnAccurately = 9,
  kICCExtractedSRGBColorSpace = 10,
  kICCProfileAnalyzeLast = kICCExtractedSRGBColorSpace,
};

struct ExtractColorSpacesResult {
  ExtractColorSpacesResult(ICCAnalyzeResult icc_analyze_result)
      : icc_analyze_result(icc_analyze_result) {}

  ICCAnalyzeResult icc_analyze_result;
  float nonlinear_fit_error = -1;

  gfx::ColorSpace parametric_color_space;
  sk_sp<SkColorSpace> useable_sk_color_space;
};

ExtractColorSpacesResult ExtractColorSpaces(const std::vector<char>& data) {
  // Parse the profile and attempt to create a SkColorSpaceXform out of it.
  sk_sp<SkColorSpace> sk_srgb_color_space = SkColorSpace::MakeSRGB();
  sk_sp<SkICC> sk_icc = SkICC::Make(data.data(), data.size());
  if (!sk_icc) {
    DLOG(ERROR) << "Failed to parse ICC profile to SkICC.";
    return ICCAnalyzeResult(kICCFailedToParse);
  }
  sk_sp<SkColorSpace> sk_icc_color_space =
      SkColorSpace::MakeICC(data.data(), data.size());
  if (!sk_icc_color_space) {
    DLOG(ERROR) << "Failed to parse ICC profile to SkColorSpace.";
    return ICCAnalyzeResult(kICCFailedToExtractSkColorSpace);
  }
  std::unique_ptr<SkColorSpaceXform> sk_color_space_xform =
      SkColorSpaceXform::New(sk_srgb_color_space.get(),
                             sk_icc_color_space.get());
  if (!sk_color_space_xform) {
    DLOG(ERROR) << "Parsed ICC profile but can't create SkColorSpaceXform.";
    return ICCAnalyzeResult(kICCFailedToCreateXform);
  }

  // Because this SkColorSpace can be used to construct a transform, mark it
  // as "useable". Mark the "best approximation" as sRGB to start.
  ExtractColorSpacesResult result(kICCExtractedSRGBColorSpace);
  result.parametric_color_space = ColorSpace::CreateSRGB();
  result.useable_sk_color_space = sk_icc_color_space;

  // If our SkColorSpace representation is sRGB then return that.
  if (SkColorSpace::Equals(sk_srgb_color_space.get(),
                           sk_icc_color_space.get())) {
    return result;
  }

  // A primary matrix is required for our parametric approximation.
  SkMatrix44 to_XYZD50_matrix;
  if (!sk_icc->toXYZD50(&to_XYZD50_matrix)) {
    DLOG(ERROR) << "Failed to extract ICC profile primary matrix.";
    result.icc_analyze_result = kICCFailedToExtractMatrix;
    return result;
  }

  // Update our best guess to be the specified primary matrix with an sRGB
  // transfer function.
  result.parametric_color_space = gfx::ColorSpace::CreateCustom(
      to_XYZD50_matrix, ColorSpace::TransferID::IEC61966_2_1);

  // Try to directly extract a numerical transfer function.
  SkColorSpaceTransferFn exact_tr_fn;
  if (sk_icc->isNumericalTransferFn(&exact_tr_fn)) {
    result.icc_analyze_result = kICCExtractedMatrixAndAnalyticTrFn;
    result.parametric_color_space =
        gfx::ColorSpace::CreateCustom(to_XYZD50_matrix, exact_tr_fn);
    return result;
  }

  // If that fails, try to numerically approximate the transfer function.
  SkColorSpaceTransferFn approx_tr_fn;
  float approx_tr_fn_max_error = 0;
  if (!SkApproximateTransferFn(sk_icc, &approx_tr_fn_max_error,
                               &approx_tr_fn)) {
    DLOG(ERROR) << "Failed approximate transfer function.";
    result.icc_analyze_result = kICCFailedToConvergeToApproximateTrFn;
    return result;
  }

  // Include the fit error in the result.
  result.nonlinear_fit_error = approx_tr_fn_max_error;

  // If the approximation converged but wasn't very accurate, don't use it.
  const float kMaxError = 2.f / 256.f;
  if (approx_tr_fn_max_error >= kMaxError) {
    DLOG(ERROR) << "Failed to accurately approximate transfer function, error: "
                << 256.f * approx_tr_fn_max_error << "/256";
    result.icc_analyze_result = kICCFailedToApproximateTrFnAccurately;
    return result;
  }

  // If the approximation converged and the error is small, use it.
  result.icc_analyze_result = kICCExtractedMatrixAndApproximatedTrFn;
  result.parametric_color_space =
      gfx::ColorSpace::CreateCustom(to_XYZD50_matrix, approx_tr_fn);
  return result;
}

}  // namespace

ICCProfile::ICCProfile() = default;
ICCProfile::ICCProfile(ICCProfile&& other) = default;
ICCProfile::ICCProfile(const ICCProfile& other) = default;
ICCProfile& ICCProfile::operator=(ICCProfile&& other) = default;
ICCProfile& ICCProfile::operator=(const ICCProfile& other) = default;
ICCProfile::~ICCProfile() = default;

bool ICCProfile::operator==(const ICCProfile& other) const {
  return data_ == other.data_;
}

bool ICCProfile::operator!=(const ICCProfile& other) const {
  return !(*this == other);
}

bool ICCProfile::IsValid() const {
  return successfully_parsed_by_sk_icc_;
}

// static
ICCProfile ICCProfile::FromData(const void* data, size_t size) {
  return FromDataWithId(data, size, 0);
}

// static
ICCProfile ICCProfile::FromDataWithId(const void* data,
                                      size_t size,
                                      uint64_t new_profile_id) {
  if (!size)
    return ICCProfile();

  const char* data_as_char = reinterpret_cast<const char*>(data);
  {
    // Linearly search the cached ICC profiles to find one with the same data.
    // If it exists, re-use its id and touch it in the cache.
    Cache& cache = g_cache.Get();
    base::AutoLock lock(cache.lock);
    for (auto iter = cache.id_to_icc_profile_mru.begin();
         iter != cache.id_to_icc_profile_mru.end(); ++iter) {
      const std::vector<char>& iter_data = iter->second.data_;
      if (iter_data.size() != size || memcmp(data, iter_data.data(), size))
        continue;
      auto found = cache.id_to_icc_profile_mru.Get(iter->second.id_);
      return found->second;
    }
    if (!new_profile_id)
      new_profile_id = cache.next_unused_id++;
  }

  // Create a new cached id and add it to the cache. Histogram the results,
  // since this is a new profile we are seeing.
  ICCProfile icc_profile;
  icc_profile.id_ = new_profile_id;
  icc_profile.data_.insert(icc_profile.data_.begin(), data_as_char,
                           data_as_char + size);
  icc_profile.ComputeColorSpaceAndCache(true);
  return icc_profile;
}

#if (!defined(OS_WIN) && !defined(USE_X11) && !defined(OS_MACOSX)) || \
    defined(OS_IOS)
// static
ICCProfile ICCProfile::FromBestMonitor() {
  return ICCProfile();
}
#endif

// static
const std::vector<char>& ICCProfile::GetData() const {
  return data_;
}

const ColorSpace& ICCProfile::GetColorSpace() const {
  // Move this ICC profile to the most recently used end of the cache,
  // inserting if needed.
  if (id_) {
    Cache& cache = g_cache.Get();
    base::AutoLock lock(cache.lock);
    auto found = cache.id_to_icc_profile_mru.Get(id_);
    if (found == cache.id_to_icc_profile_mru.end())
      found = cache.id_to_icc_profile_mru.Put(id_, *this);
  }
  return color_space_;
}

const ColorSpace& ICCProfile::GetParametricColorSpace() const {
  // Move this ICC profile to the most recently used end of the cache,
  // inserting if needed.
  if (id_) {
    Cache& cache = g_cache.Get();
    base::AutoLock lock(cache.lock);
    auto found = cache.id_to_icc_profile_mru.Get(id_);
    if (found == cache.id_to_icc_profile_mru.end())
      found = cache.id_to_icc_profile_mru.Put(id_, *this);
  }
  return parametric_color_space_;
}

// static
bool ICCProfile::FromId(uint64_t id,
                        ICCProfile* icc_profile) {
  if (!id)
    return false;

  Cache& cache = g_cache.Get();
  base::AutoLock lock(cache.lock);

  auto found = cache.id_to_icc_profile_mru.Get(id);
  if (found == cache.id_to_icc_profile_mru.end())
    return false;

  *icc_profile = found->second;
  return true;
}

void ICCProfile::ComputeColorSpaceAndCache(bool histogram_results) {
  if (!id_)
    return;

  // If this already exists in the cache, just update its |color_space_|.
  {
    Cache& cache = g_cache.Get();
    base::AutoLock lock(cache.lock);
    auto found = cache.id_to_icc_profile_mru.Get(id_);
    if (found != cache.id_to_icc_profile_mru.end()) {
      color_space_ = found->second.color_space_;
      parametric_color_space_ = found->second.parametric_color_space_;
      successfully_parsed_by_sk_icc_ =
          found->second.successfully_parsed_by_sk_icc_;
      return;
    }
  }

  // Parse the ICC profile
  ExtractColorSpacesResult result = ExtractColorSpaces(data_);
  switch (result.icc_analyze_result) {
    case kICCExtractedSRGBColorSpace:
    case kICCExtractedMatrixAndAnalyticTrFn:
    case kICCExtractedMatrixAndApproximatedTrFn:
      // Successfully and accurately extracted color space.
      successfully_parsed_by_sk_icc_ = true;
      parametric_color_space_ = result.parametric_color_space;
      parametric_color_space_.icc_profile_id_ = id_;
      color_space_ = parametric_color_space_;
      break;
    case kICCFailedToExtractRawTrFn:
    case kICCFailedToExtractMatrix:
    case kICCFailedToConvergeToApproximateTrFn:
    case kICCFailedToApproximateTrFnAccurately:
      // Successfully but inaccurately extracted color space.
      successfully_parsed_by_sk_icc_ = true;
      parametric_color_space_ = result.parametric_color_space;
      color_space_ = ColorSpace(ColorSpace::PrimaryID::ICC_BASED,
                                ColorSpace::TransferID::ICC_BASED);
      color_space_.icc_profile_id_ = id_;
      color_space_.icc_profile_sk_color_space_ = result.useable_sk_color_space;
      break;
    case kICCFailedToParse:
    case kICCFailedToExtractSkColorSpace:
    case kICCFailedToCreateXform:
      // Can't even use this color space, will just pretend it was sRGB.
      successfully_parsed_by_sk_icc_ = false;
      parametric_color_space_ = gfx::ColorSpace();
      color_space_ = parametric_color_space_;
      break;
    case kICCFailedToApproximateTrFnObsolete:
      NOTREACHED();
      break;
  }

  if (histogram_results) {
    UMA_HISTOGRAM_ENUMERATION("Blink.ColorSpace.Destination.ICCResult",
                              result.icc_analyze_result,
                              kICCProfileAnalyzeLast);

    // Add histograms for numerical approximation.
    bool nonlinear_fit_converged =
        result.icc_analyze_result == kICCExtractedMatrixAndApproximatedTrFn ||
        result.icc_analyze_result == kICCFailedToApproximateTrFnAccurately;
    bool nonlinear_fit_did_not_converge =
        result.icc_analyze_result == kICCFailedToConvergeToApproximateTrFn;
    if (nonlinear_fit_converged || nonlinear_fit_did_not_converge) {
      UMA_HISTOGRAM_BOOLEAN(
          "Blink.ColorSpace.Destination.NonlinearFitConverged",
          nonlinear_fit_converged);
    }
    if (nonlinear_fit_converged) {
      UMA_HISTOGRAM_CUSTOM_COUNTS(
          "Blink.ColorSpace.Destination.NonlinearFitError",
          static_cast<int>(result.nonlinear_fit_error * 255), 0, 127, 16);
    }
  }

  // Add to the cache.
  {
    Cache& cache = g_cache.Get();
    base::AutoLock lock(cache.lock);
    cache.id_to_icc_profile_mru.Put(id_, *this);
  }
}

}  // namespace gfx
