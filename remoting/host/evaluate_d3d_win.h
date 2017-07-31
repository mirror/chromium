// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef REMOTING_HOST_EVALUATE_D3D_H_
#define REMOTING_HOST_EVALUATE_D3D_H_

#include <string>
#include <vector>

namespace remoting {

// Evaluates the D3D capability of the system and outputs the results into
// stdout. This function is expected to be called in EvaluateCapabilityLocally()
// only.
int EvaluateD3D();

// Evaluates the D3D capability of the system in a separated process. Returns
// true if the process succeeded. The capabilities will be stored in |result| if
// it's not nullptr.
bool GetD3DCapability(std::vector<std::string>* result = nullptr);

}  // namespace remoting

#endif  // REMOTING_HOST_EVALUATE_D3D_H_
