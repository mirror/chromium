/* 
 * Copyright 2016 The Chromium Authors.
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a
 * copy of this software and associated documentation files (the "Software"),
 * to deal in the Software without restriction, including without limitation
 * the rights to use, copy, modify, merge, publish, distribute, sublicense,
 * and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice (including the next
 * paragraph) shall be included in all copies or substantial portions of the
 * Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.  IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

#ifndef VSYNC_FEEDBACK_UNSTABLE_V1_CLIENT_PROTOCOL_H
#define VSYNC_FEEDBACK_UNSTABLE_V1_CLIENT_PROTOCOL_H

#ifdef  __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stddef.h>
#include "wayland-client.h"

struct wl_client;
struct wl_resource;

struct wl_output;
struct zcr_vsync_feedback_v1;
struct zcr_vsync_timing_v1;

extern const struct wl_interface zcr_vsync_feedback_v1_interface;
extern const struct wl_interface zcr_vsync_timing_v1_interface;

#define ZCR_VSYNC_FEEDBACK_V1_DESTROY	0
#define ZCR_VSYNC_FEEDBACK_V1_GET_VSYNC_TIMING	1

static inline void
zcr_vsync_feedback_v1_set_user_data(struct zcr_vsync_feedback_v1 *zcr_vsync_feedback_v1, void *user_data)
{
	wl_proxy_set_user_data((struct wl_proxy *) zcr_vsync_feedback_v1, user_data);
}

static inline void *
zcr_vsync_feedback_v1_get_user_data(struct zcr_vsync_feedback_v1 *zcr_vsync_feedback_v1)
{
	return wl_proxy_get_user_data((struct wl_proxy *) zcr_vsync_feedback_v1);
}

static inline void
zcr_vsync_feedback_v1_destroy(struct zcr_vsync_feedback_v1 *zcr_vsync_feedback_v1)
{
	wl_proxy_marshal((struct wl_proxy *) zcr_vsync_feedback_v1,
			 ZCR_VSYNC_FEEDBACK_V1_DESTROY);

	wl_proxy_destroy((struct wl_proxy *) zcr_vsync_feedback_v1);
}

static inline struct zcr_vsync_timing_v1 *
zcr_vsync_feedback_v1_get_vsync_timing(struct zcr_vsync_feedback_v1 *zcr_vsync_feedback_v1, struct wl_output *output)
{
	struct wl_proxy *id;

	id = wl_proxy_marshal_constructor((struct wl_proxy *) zcr_vsync_feedback_v1,
			 ZCR_VSYNC_FEEDBACK_V1_GET_VSYNC_TIMING, &zcr_vsync_timing_v1_interface, NULL, output);

	return (struct zcr_vsync_timing_v1 *) id;
}

struct zcr_vsync_timing_v1_listener {
	/**
	 * update - vsync timing updated
	 * @timebase_l: new vsync timebase (lower 32 bits)
	 * @timebase_h: new vsync timebase (upper 32 bits)
	 * @interval_l: new vsync interval (lower 32 bits)
	 * @interval_h: new vsync interval (upper 32 bits)
	 *
	 * Notifies client that vertical synchronization timing of given
	 * wl_output has changed.
	 *
	 * Timing information consists of two data, timebase and interval.
	 * Timebase is an absolute timestamp of the vsync event that caused
	 * the timing to change. Interval is a period of time between
	 * subsequent vsync events.
	 *
	 * The unit of all above mentioned time values shall be
	 * microseconds and absolute timestamps should match the realm of
	 * the primary system monotonic counter, i.e. the POSIX
	 * clock_gettime(CLOCK_MONOTONIC). Data type of both values is
	 * defined to be a 64-bit unsigned integer, but since the biggest
	 * unsigned integer datatype defined by the Wayland protocol is the
	 * 32-bit uint, both timebase and interval are split into most
	 * significant and least significant part, suffixed by "_h" and
	 * "_l" respectively.
	 */
	void (*update)(void *data,
		       struct zcr_vsync_timing_v1 *zcr_vsync_timing_v1,
		       uint32_t timebase_l,
		       uint32_t timebase_h,
		       uint32_t interval_l,
		       uint32_t interval_h);
};

static inline int
zcr_vsync_timing_v1_add_listener(struct zcr_vsync_timing_v1 *zcr_vsync_timing_v1,
				 const struct zcr_vsync_timing_v1_listener *listener, void *data)
{
	return wl_proxy_add_listener((struct wl_proxy *) zcr_vsync_timing_v1,
				     (void (**)(void)) listener, data);
}

#define ZCR_VSYNC_TIMING_V1_DESTROY	0

static inline void
zcr_vsync_timing_v1_set_user_data(struct zcr_vsync_timing_v1 *zcr_vsync_timing_v1, void *user_data)
{
	wl_proxy_set_user_data((struct wl_proxy *) zcr_vsync_timing_v1, user_data);
}

static inline void *
zcr_vsync_timing_v1_get_user_data(struct zcr_vsync_timing_v1 *zcr_vsync_timing_v1)
{
	return wl_proxy_get_user_data((struct wl_proxy *) zcr_vsync_timing_v1);
}

static inline void
zcr_vsync_timing_v1_destroy(struct zcr_vsync_timing_v1 *zcr_vsync_timing_v1)
{
	wl_proxy_marshal((struct wl_proxy *) zcr_vsync_timing_v1,
			 ZCR_VSYNC_TIMING_V1_DESTROY);

	wl_proxy_destroy((struct wl_proxy *) zcr_vsync_timing_v1);
}

#ifdef  __cplusplus
}
#endif

#endif
