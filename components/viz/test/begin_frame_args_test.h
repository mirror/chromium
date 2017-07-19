// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_VIZ_TEST_BEGIN_FRAME_ARGS_TEST_H_
#define COMPONENTS_VIZ_TEST_BEGIN_FRAME_ARGS_TEST_H_

#include <stdint.h>

#include <iosfwd>

#include "base/test/simple_test_tick_clock.h"
#include "base/time/time.h"
#include "components/viz/common/begin_frame_args.h"
#include "components/viz/common/viz_common_export.h"
#include "testing/gtest/include/gtest/gtest.h"

namespace viz {

// Functions for quickly creating BeginFrameArgs
VIZ_COMMON_EXPORT BeginFrameArgs
CreateBeginFrameArgsForTesting(BeginFrameArgs::CreationLocation location,
                               uint32_t source_id,
                               uint64_t sequence_number);

VIZ_COMMON_EXPORT BeginFrameArgs
CreateBeginFrameArgsForTesting(BeginFrameArgs::CreationLocation location,
                               uint32_t source_id,
                               uint64_t sequence_number,
                               base::TimeTicks frame_time);

VIZ_COMMON_EXPORT BeginFrameArgs
CreateBeginFrameArgsForTesting(BeginFrameArgs::CreationLocation location,
                               uint32_t source_id,
                               uint64_t sequence_number,
                               int64_t frame_time,
                               int64_t deadline,
                               int64_t interval);

VIZ_COMMON_EXPORT BeginFrameArgs
CreateBeginFrameArgsForTesting(BeginFrameArgs::CreationLocation location,
                               uint32_t source_id,
                               uint64_t sequence_number,
                               int64_t frame_time,
                               int64_t deadline,
                               int64_t interval,
                               BeginFrameArgs::BeginFrameArgsType type);

// Creates a BeginFrameArgs using the fake Now value stored on the
// OrderSimpleTaskRunner.
VIZ_COMMON_EXPORT BeginFrameArgs
CreateBeginFrameArgsForTesting(BeginFrameArgs::CreationLocation location,
                               uint32_t source_id,
                               uint64_t sequence_number,
                               base::SimpleTestTickClock* now_src);

// gtest helpers -- these *must* be in the same namespace as the types they
// operate on.

// Allow "EXPECT_EQ(args1, args2);"
// We don't define operator!= because EXPECT_NE(args1, args2) isn't all that
// sensible.
VIZ_COMMON_EXPORT bool operator==(const BeginFrameArgs& lhs,
                                  const BeginFrameArgs& rhs);

// Allow gtest to pretty print begin frame args.
VIZ_COMMON_EXPORT ::std::ostream& operator<<(::std::ostream& os,
                                             const BeginFrameArgs& args);

VIZ_COMMON_EXPORT void PrintTo(const BeginFrameArgs& args, ::std::ostream* os);

// Allow "EXPECT_EQ(ack1, ack2);"
VIZ_COMMON_EXPORT bool operator==(const BeginFrameAck& lhs,
                                  const BeginFrameAck& rhs);

// Allow gtest to pretty print BeginFrameAcks.
VIZ_COMMON_EXPORT ::std::ostream& operator<<(::std::ostream& os,
                                             const BeginFrameAck& ack);

VIZ_COMMON_EXPORT void PrintTo(const BeginFrameAck& ack, ::std::ostream* os);

}  // namespace viz

#endif  // COMPONENTS_VIZ_TEST_BEGIN_FRAME_ARGS_TEST_H_
