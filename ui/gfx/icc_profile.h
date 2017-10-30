// Copyright 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_GFX_ICC_PROFILE_H_
#define UI_GFX_ICC_PROFILE_H_

#include <stdint.h>
#include <map>
#include <set>
#include <vector>

#include "base/gtest_prod_util.h"
#include "base/lazy_instance.h"
#include "base/memory/ref_counted.h"
#include "ui/gfx/color_space.h"

#if defined(OS_MACOSX)
#include <CoreGraphics/CGColorSpace.h>
#endif

extern "C" int LLVMFuzzerTestOneInput(const uint8_t* data, size_t size);

namespace mojo {
template <typename, typename> struct StructTraits;
}

namespace gfx {

class ColorSpace;

// The refcounted internals of an ICCProfile object.
class COLOR_SPACE_EXPORT ICCProfileInternals
    : public base::RefCountedThreadSafe<ICCProfileInternals> {
 public:
  static scoped_refptr<ICCProfileInternals> FromData(const void* data,
                                                     size_t size);

  bool IsValid() const;
  gfx::ColorSpace GetColorSpace();
  sk_sp<SkColorSpace> GetSkColorSpace() const;
  const std::vector<uint8_t>& GetData() const { return data_; }
  void HistogramDisplay(int64_t display_id);

 protected:
  // This must match ICCProfileAnalyzeResult enum in histograms.xml.
  enum AnalyzeResult {
    kICCExtractedMatrixAndAnalyticTrFn = 0,
    kICCExtractedMatrixAndApproximatedTrFn = 1,
    kICCFailedToConvergeToApproximateTrFn = 2,
    kICCFailedToExtractRawTrFn = 3,
    kICCFailedToExtractMatrix = 4,
    kICCFailedToParse = 5,
    kICCFailedToExtractSkColorSpace = 6,
    kICCFailedToCreateXform = 7,
    kICCFailedToApproximateTrFnAccurately = 8,
    kICCExtractedSRGBColorSpace = 9,
    kICCProfileAnalyzeLast = kICCExtractedSRGBColorSpace,
  };
  friend class base::RefCountedThreadSafe<ICCProfileInternals>;

  ICCProfileInternals(std::vector<uint8_t> data);
  virtual ~ICCProfileInternals();

  AnalyzeResult Analyze();

  // The actual data.
  std::vector<uint8_t> data_;

  // Results of Skia parsing the ICC profile data.
  sk_sp<SkColorSpace> sk_color_space_;

  // The result of converting the SkICC to a parametric color space.
  AnalyzeResult analyze_result_ = kICCFailedToParse;

  // The best-fit parametric primaries.
  gfx::ColorSpace::PrimaryID primaries_;
  SkMatrix44 to_XYZD50_;

  // The best-fit parametric transfer function.
  gfx::ColorSpace::TransferID transfer_;
  SkColorSpaceTransferFn transfer_fn_;
  float transfer_fn_error_ = 0;

  // The set of display ids which have have caused this ICC profile to be
  // recorded in UMA histograms. Only record an ICC profile once per display
  // id (since the same profile will be re-read repeatedly, e.g, when displays
  // are resized).
  std::set<int64_t> histogrammed_display_ids_;
};

// Used to represent a full ICC profile, usually retrieved from a monitor. It
// can be lossily compressed into a ColorSpace object. This structure should
// only be sent from higher-privilege processes to lower-privilege processes,
// as parsing this structure is not secure.
class COLOR_SPACE_EXPORT ICCProfile {
 public:
  ICCProfile();
  ICCProfile(ICCProfile&& other);
  ICCProfile(const ICCProfile& other);
  ICCProfile& operator=(ICCProfile&& other);
  ICCProfile& operator=(const ICCProfile& other);
  ~ICCProfile();
  bool operator==(const ICCProfile& other) const;
  bool operator!=(const ICCProfile& other) const;

  // Returns true if this profile was successfully parsed by SkICC.
  bool IsValid() const;

#if defined(OS_MACOSX)
  static ICCProfile FromCGColorSpace(CGColorSpaceRef cg_color_space);
#endif

  // Create directly from profile data.
  static ICCProfile FromData(const void* icc_profile, size_t size);

  // Return a ColorSpace that references this ICCProfile. ColorTransforms
  // created using this ColorSpace will match this ICCProfile precisely.
  ColorSpace GetColorSpace() const;

  // Return a ColorSpace that is the best parametric approximation of this
  // ICCProfile. The resulting ColorSpace will reference this ICCProfile only
  // if the parametric approximation is almost exact.
  ColorSpace GetParametricColorSpace() const;

  // Histogram how we this was approximated by a gfx::ColorSpace. Only
  // histogram a given profile once per display.
  void HistogramDisplay(int64_t display_id) const;

 private:
  ICCProfile(scoped_refptr<ICCProfileInternals> internals);
  scoped_refptr<ICCProfileInternals> internals_;

  FRIEND_TEST_ALL_PREFIXES(SimpleColorSpace, BT709toSRGBICC);
  FRIEND_TEST_ALL_PREFIXES(SimpleColorSpace, GetColorSpace);
  friend int ::LLVMFuzzerTestOneInput(const uint8_t*, size_t);
  friend class ColorSpace;
  friend class ColorTransformInternal;
  friend struct IPC::ParamTraits<gfx::ICCProfile>;
};

}  // namespace gfx

#endif  // UI_GFX_ICC_PROFILE_H_
