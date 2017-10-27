// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_VIZ_TEST_MOCK_HOST_FRAME_SINK_CLIENT_H_
#define COMPONENTS_VIZ_TEST_MOCK_HOST_FRAME_SINK_CLIENT_H_

#include "base/macros.h"
#include "components/viz/common/surfaces/surface_info.h"
#include "components/viz/host/host_frame_sink_client.h"
#include "testing/gmock/include/gmock/gmock.h"

namespace viz {

// A mock implementation of HostFrameSinkClient.
class MockHostFrameSinkClient : public HostFrameSinkClient {
 public:
  MockHostFrameSinkClient();
  ~MockHostFrameSinkClient() override;

  // HostFrameSinkClient implementation.
  MOCK_METHOD1(OnFirstSurfaceActivation, void(const SurfaceInfo&));

 private:
  DISALLOW_COPY_AND_ASSIGN(MockHostFrameSinkClient);
};

}  // namespace viz

#endif  // COMPONENTS_VIZ_TEST_MOCK_HOST_FRAME_SINK_CLIENT_H_
