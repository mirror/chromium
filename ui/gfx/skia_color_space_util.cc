// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "ui/gfx/skia_color_space_util.h"

#include <algorithm>
#include <cmath>
#include <vector>

#include "base/logging.h"

namespace gfx {

namespace {

// Evaluate the gradient of the nonlinear component of fn
void SkTransferFnEvalGradientNonlinear(const SkColorSpaceTransferFn& fn,
                                       float x,
                                       float* d_fn_d_fA_at_x,
                                       float* d_fn_d_fB_at_x,
                                       float* d_fn_d_fE_at_x,
                                       float* d_fn_d_fG_at_x) {
  float base = fn.fA * x + fn.fB;
  if (base > 0.f) {
    *d_fn_d_fA_at_x = fn.fG * x * std::pow(base, fn.fG - 1.f);
    *d_fn_d_fB_at_x = fn.fG * std::pow(base, fn.fG - 1.f);
    *d_fn_d_fE_at_x = 1.f;
    *d_fn_d_fG_at_x = std::pow(base, fn.fG) * std::log(base);
  } else {
    *d_fn_d_fA_at_x = 0.f;
    *d_fn_d_fB_at_x = 0.f;
    *d_fn_d_fE_at_x = 0.f;
    *d_fn_d_fG_at_x = 0.f;
  }
}

// Take one Gauss-Newton step updating fA, fB, fE, and fG, given fD.
bool SkTransferFnGaussNewtonStepNonlinear(SkColorSpaceTransferFn* fn,
                                          float* error_Linfty_after,
                                          const float* x,
                                          const float* t,
                                          size_t n) {
  // Let ne_lhs be the left hand side of the normal equations, and let ne_rhs
  // be the right hand side. Zero the diagonal of |ne_lhs| and all of |ne_rhs|.
  SkMatrix44 ne_lhs;
  SkVector4 ne_rhs;
  for (int row = 0; row < 4; ++row) {
    for (int col = 0; col < 4; ++col) {
      ne_lhs.set(row, col, 0);
    }
    ne_rhs.fData[row] = 0;
  }

  // Add the contributions from each sample to the normal equations.
  for (size_t i = 0; i < n; ++i) {
    // Ignore points in the linear segment.
    if (x[i] < fn->fD)
      continue;

    // Let J be the gradient of fn with respect to parameters A, B, E, and G,
    // evaulated at this point.
    float J[4];
    SkTransferFnEvalGradientNonlinear(*fn, x[i], &J[0], &J[1], &J[2], &J[3]);
    // Let r be the residual at this point;
    float r = t[i] - SkTransferFnEval(*fn, x[i]);

    // Update the normal equations left hand side with the outer product of J
    // with itself.
    for (int row = 0; row < 4; ++row) {
      for (int col = 0; col < 4; ++col) {
        ne_lhs.set(row, col, ne_lhs.get(row, col) + J[row] * J[col]);
      }

      // Update the normal equations right hand side the product of J with the
      // residual
      ne_rhs.fData[row] += J[row] * r;
    }
  }

  // Note that if fG = 1, then the normal equations will be singular (because,
  // when fG = 1, because fB and fE are equivalent parameters).  To avoid
  // problems, fix fE (row/column 3) in these circumstances.
  float kEpsilonForG = 1.f / 1024.f;
  if (std::abs(fn->fG - 1) < kEpsilonForG) {
    for (int row = 0; row < 4; ++row) {
      float value = (row == 2) ? 1.f : 0.f;
      ne_lhs.set(row, 2, value);
      ne_lhs.set(2, row, value);
    }
    ne_rhs.fData[2] = 0.f;
  }

  // Solve the normal equations.
  SkMatrix44 ne_lhs_inv;
  if (!ne_lhs.invert(&ne_lhs_inv))
    return false;
  SkVector4 step = ne_lhs_inv * ne_rhs;

  // Update the transfer function.
  fn->fA += step.fData[0];
  fn->fB += step.fData[1];
  fn->fE += step.fData[2];
  fn->fG += step.fData[3];

  // fA should always be positive.
  fn->fA = std::max(fn->fA, 0.f);

  // Recalculate fD and fC so that the linear segment
  // is a tangent to the non-linear curve and the cut
  // point is where the lines meet.
  float minD = 0.f;
  float maxD = 0.5f;
  fn->fD = 0.f;
  for (int i = 0; i < 10; i++) {
    float mid = (minD + maxD) / 2.f;
    float value = SkTransferFnEval(*fn, mid);
    float slope = fn->fA * fn->fG * pow(fn->fA * mid + fn->fB, fn->fG - 1.f);
    // Check where the tangent hits x=0
    if (value - slope * mid > 0) {
      minD = mid;
    } else {
      maxD = mid;
    }
  }
  fn->fD = (minD + maxD) / 2;
  fn->fC = fn->fA * fn->fG * pow(fn->fA * fn->fD + fn->fB, fn->fG - 1.f);

  // Ensure that fn be defined at fD.
  if (fn->fA * fn->fD + fn->fB < 0.f)
    fn->fB = -fn->fA * fn->fD;

  // Compute the Linfinity error.
  *error_Linfty_after = 0;
  for (size_t i = 0; i < n; ++i) {
    float error = std::abs(t[i] - SkTransferFnEval(*fn, x[i]));
    float weight = sqrt(x[i]);
    error *= weight;
    *error_Linfty_after = std::max(error, *error_Linfty_after);
  }

  return true;
}

// Solve for fA, fB, fE, and fG, given fD. The initial value of |fn| is the
// point from which iteration starts.
bool SkTransferFnSolveNonlinear(SkColorSpaceTransferFn* fn,
                                const float* x,
                                const float* t,
                                size_t n) {
  // Take a maximum of 8 Gauss-Newton steps.
  const size_t kNumSteps = 8;

  // The L-infinity error after each step.
  float step_error[kNumSteps] = {0};
  size_t step = 0;
  for (;; ++step) {
    // If the normal equations are singular, we can't continue.
    if (!SkTransferFnGaussNewtonStepNonlinear(fn, &step_error[step], x, t, n))
      return false;

    // If the error is inf or nan, we are clearly not converging.
    if (std::isnan(step_error[step]) || std::isinf(step_error[step]))
      return false;

    // Stop if our error is tiny.
    float kEarlyOutTinyErrorThreshold = (1.f / 16.f) / 256.f;
    if (step_error[step] < kEarlyOutTinyErrorThreshold)
      break;

    // Stop if our error is not changing, or changing in the wrong direction.
    if (step > 1) {
      // If our error is is huge for two iterations, we're probably not in the
      // region of convergence.
      if (step_error[step] > 1.f && step_error[step - 1] > 1.f)
        return false;
    }
    if (step == kNumSteps - 1)
      break;
  }

  // Declare failure if our error is obviously too high.
  float kDidNotConvergeThreshold = 64.f / 256.f;
  if (step_error[step] > kDidNotConvergeThreshold)
    return false;

  // We've converged to a reasonable solution. If some of the parameters are
  // extremely close to 0 or 1, set them to 0 or 1.
  const float kRoundEpsilon = 1.f / 1024.f;
  if (std::abs(fn->fA - 1.f) < kRoundEpsilon)
    fn->fA = 1.f;
  if (std::abs(fn->fB) < kRoundEpsilon)
    fn->fB = 0;
  if (std::abs(fn->fE) < kRoundEpsilon)
    fn->fE = 0;
  if (std::abs(fn->fG - 1.f) < kRoundEpsilon)
    fn->fG = 1.f;
  return true;
}

bool SkApproximateTransferFnInternal(const float* x,
                                     const float* t,
                                     size_t n,
                                     SkColorSpaceTransferFn* fn) {
  // First, guess at a value of fD. Assume that the nonlinear segment applies
  // to all x >= 0.15. This is generally a safe assumption (fD is usually less
  // than 0.1).
  const float kLinearSegmentMaximum = 0.15f;
  fn->fD = kLinearSegmentMaximum;

  // Do a nonlinear regression on the nonlinear segment. Use a number of guesses
  // for the initial value of fG, because not all values will converge.
  bool nonlinear_fit_converged = false;
  {
    const size_t kNumInitialGammas = 4;
    float initial_gammas[kNumInitialGammas] = {2.2f, 1.f, 3.f, 0.5f};
    for (size_t i = 0; i < kNumInitialGammas; ++i) {
      fn->fG = initial_gammas[i];
      fn->fA = 1;
      fn->fB = 0;
      fn->fC = 1;
      fn->fE = 0;
      fn->fF = 0;
      if (SkTransferFnSolveNonlinear(fn, x, t, n)) {
        nonlinear_fit_converged = true;
        break;
      }
    }
  }
  if (!nonlinear_fit_converged)
    return false;

  return true;
}

}  // namespace

float SkTransferFnEvalUnclamped(const SkColorSpaceTransferFn& fn, float x) {
  if (x < fn.fD)
    return fn.fC * x + fn.fF;
  return std::pow(fn.fA * x + fn.fB, fn.fG) + fn.fE;
}

float SkTransferFnEval(const SkColorSpaceTransferFn& fn, float x) {
  float fn_at_x_unclamped = SkTransferFnEvalUnclamped(fn, x);
  return std::min(std::max(fn_at_x_unclamped, 0.f), 1.f);
}

SkColorSpaceTransferFn SkTransferFnInverse(const SkColorSpaceTransferFn& fn) {
  SkColorSpaceTransferFn fn_inv = {0};
  if (fn.fA > 0 && fn.fG > 0) {
    double a_to_the_g = std::pow(fn.fA, fn.fG);
    fn_inv.fA = 1.f / a_to_the_g;
    fn_inv.fB = -fn.fE / a_to_the_g;
    fn_inv.fG = 1.f / fn.fG;
  }
  fn_inv.fD = fn.fC * fn.fD + fn.fF;
  fn_inv.fE = -fn.fB / fn.fA;
  if (fn.fC != 0) {
    fn_inv.fC = 1.f / fn.fC;
    fn_inv.fF = -fn.fF / fn.fC;
  }
  return fn_inv;
}

bool SkTransferFnsApproximatelyCancel(const SkColorSpaceTransferFn& a,
                                      const SkColorSpaceTransferFn& b) {
  const float kStep = 1.f / 8.f;
  const float kEpsilon = 2.5f / 256.f;
  for (float x = 0; x <= 1.f; x += kStep) {
    float a_of_x = SkTransferFnEval(a, x);
    float b_of_a_of_x = SkTransferFnEval(b, a_of_x);
    if (std::abs(b_of_a_of_x - x) > kEpsilon)
      return false;
  }
  return true;
}

bool SkTransferFnIsApproximatelyIdentity(const SkColorSpaceTransferFn& a) {
  const float kStep = 1.f / 8.f;
  const float kEpsilon = 2.5f / 256.f;
  for (float x = 0; x <= 1.f; x += kStep) {
    float a_of_x = SkTransferFnEval(a, x);
    if (std::abs(a_of_x - x) > kEpsilon)
      return false;
  }
  return true;
}

bool SkApproximateTransferFn(const float* x,
                             const float* t,
                             size_t n,
                             SkColorSpaceTransferFn* fn) {
  if (SkApproximateTransferFnInternal(x, t, n, fn))
    return true;
  fn->fA = 1;
  fn->fB = 0;
  fn->fC = 1;
  fn->fD = 0;
  fn->fE = 0;
  fn->fF = 0;
  fn->fG = 1;
  return false;
}

bool SkApproximateTransferFn(sk_sp<SkICC> sk_icc,
                             float* max_error,
                             SkColorSpaceTransferFn* fn) {
  SkICC::Tables tables;
  bool got_tables = sk_icc->rawTransferFnData(&tables);
  if (!got_tables)
    return false;

  // Merge all channels' tables into a single array.
  SkICC::Channel* channels[3] = {&tables.fGreen, &tables.fRed, &tables.fBlue};
  std::vector<float> x;
  std::vector<float> t;
  for (size_t c = 0; c < 3; ++c) {
    SkICC::Channel* channel = channels[c];
    const float* data = reinterpret_cast<const float*>(
        tables.fStorage->bytes() + channel->fOffset);
    for (int i = 0; i < channel->fCount; ++i) {
      float xi = i / (channel->fCount - 1.f);
      float ti = data[i];
      x.push_back(xi);
      t.push_back(ti);
    }
  }

  // Approximate the transfer function.
  bool converged =
      SkApproximateTransferFnInternal(x.data(), t.data(), x.size(), fn);
  if (!converged)
    return false;

  // Compute the error among all channels.
  *max_error = 0;
  for (size_t i = 0; i < x.size(); ++i) {
    float fn_of_xi = gfx::SkTransferFnEval(*fn, x[i]);
    float error_at_xi = std::abs(t[i] - fn_of_xi);
    *max_error = std::max(*max_error, error_at_xi);
  }
  return true;
}

bool SkMatrixIsApproximatelyIdentity(const SkMatrix44& m) {
  const float kEpsilon = 1.f / 256.f;
  for (int i = 0; i < 4; ++i) {
    for (int j = 0; j < 4; ++j) {
      float identity_value = i == j ? 1 : 0;
      float value = m.get(i, j);
      if (std::abs(identity_value - value) > kEpsilon)
        return false;
    }
  }
  return true;
}

}  // namespace gfx
