// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef REMOTING_HOST_EVALUATE_D3D_H_
#define REMOTING_HOST_EVALUATE_D3D_H_

namespace remoting {

// Evaluates the D3D capability of the system and outputs the results into
// stdout. This function is expected to be called in EvaluateCapabilityLocally()
// only.
int EvaluateD3D();

}  // namespace remoting

#endif  // REMOTING_HOST_EVALUATE_D3D_H_
