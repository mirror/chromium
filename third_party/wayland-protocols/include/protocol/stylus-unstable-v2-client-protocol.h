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
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
 * THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER
 * DEALINGS IN THE SOFTWARE.
 */

#ifndef STYLUS_UNSTABLE_V2_CLIENT_PROTOCOL_H
#define STYLUS_UNSTABLE_V2_CLIENT_PROTOCOL_H

#ifdef  __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stddef.h>
#include "wayland-client.h"

struct wl_client;
struct wl_resource;

struct wl_touch;
struct zcr_stylus_v2;
struct zcr_touch_stylus_v2;

extern const struct wl_interface zcr_stylus_v2_interface;
extern const struct wl_interface zcr_touch_stylus_v2_interface;

#ifndef ZCR_STYLUS_V2_ERROR_ENUM
#define ZCR_STYLUS_V2_ERROR_ENUM
enum zcr_stylus_v2_error {
	ZCR_STYLUS_V2_ERROR_TOUCH_STYLUS_EXISTS = 0,
};
#endif /* ZCR_STYLUS_V2_ERROR_ENUM */

#define ZCR_STYLUS_V2_GET_TOUCH_STYLUS	0

static inline void
zcr_stylus_v2_set_user_data(struct zcr_stylus_v2 *zcr_stylus_v2, void *user_data)
{
	wl_proxy_set_user_data((struct wl_proxy *) zcr_stylus_v2, user_data);
}

static inline void *
zcr_stylus_v2_get_user_data(struct zcr_stylus_v2 *zcr_stylus_v2)
{
	return wl_proxy_get_user_data((struct wl_proxy *) zcr_stylus_v2);
}

static inline void
zcr_stylus_v2_destroy(struct zcr_stylus_v2 *zcr_stylus_v2)
{
	wl_proxy_destroy((struct wl_proxy *) zcr_stylus_v2);
}

static inline struct zcr_touch_stylus_v2 *
zcr_stylus_v2_get_touch_stylus(struct zcr_stylus_v2 *zcr_stylus_v2, struct wl_touch *touch)
{
	struct wl_proxy *id;

	id = wl_proxy_marshal_constructor((struct wl_proxy *) zcr_stylus_v2,
			 ZCR_STYLUS_V2_GET_TOUCH_STYLUS, &zcr_touch_stylus_v2_interface, NULL, touch);

	return (struct zcr_touch_stylus_v2 *) id;
}

#ifndef ZCR_TOUCH_STYLUS_V2_TOOL_TYPE_ENUM
#define ZCR_TOUCH_STYLUS_V2_TOOL_TYPE_ENUM
/**
 * zcr_touch_stylus_v2_tool_type - tool type of device.
 * @ZCR_TOUCH_STYLUS_V2_TOOL_TYPE_TOUCH: Touch
 * @ZCR_TOUCH_STYLUS_V2_TOOL_TYPE_PEN: Pen
 * @ZCR_TOUCH_STYLUS_V2_TOOL_TYPE_ERASER: Eraser
 *
 * 
 */
enum zcr_touch_stylus_v2_tool_type {
	ZCR_TOUCH_STYLUS_V2_TOOL_TYPE_TOUCH = 1,
	ZCR_TOUCH_STYLUS_V2_TOOL_TYPE_PEN = 2,
	ZCR_TOUCH_STYLUS_V2_TOOL_TYPE_ERASER = 3,
};
#endif /* ZCR_TOUCH_STYLUS_V2_TOOL_TYPE_ENUM */

/**
 * zcr_touch_stylus_v2 - stylus extension for touch
 * @tool: sets tool type of touch
 * @force: force change event
 * @tilt: tilt change event
 *
 * The zcr_touch_stylus_v1 interface extends the wl_touch interface with
 * events to describe details about a stylus.
 */
struct zcr_touch_stylus_v2_listener {
	/**
	 * tool - sets tool type of touch
	 * @id: touch id
	 * @type: type of tool in use
	 *
	 * Notification that the user is using a tool type other than
	 * touch. There can only be one tool in use at a time. This event
	 * is sent in the same frame as the wl_touch.down event. The tool
	 * type cannot change while a touch is being reported.
	 */
	void (*tool)(void *data,
		     struct zcr_touch_stylus_v2 *zcr_touch_stylus_v2,
		     uint32_t id,
		     uint32_t type);
	/**
	 * force - force change event
	 * @time: timestamp with millisecond granularity
	 * @id: touch id
	 * @force: new value of force
	 *
	 * Notification of a change in physical force on the surface of
	 * the screen. The force is calibrated and normalized to the 0 to 1
	 * range.
	 */
	void (*force)(void *data,
		      struct zcr_touch_stylus_v2 *zcr_touch_stylus_v2,
		      uint32_t time,
		      uint32_t id,
		      wl_fixed_t force);
	/**
	 * tilt - tilt change event
	 * @time: timestamp with millisecond granularity
	 * @id: touch id
	 * @tilt_x: tilt in x direction
	 * @tilt_y: tilt in y direction
	 *
	 * Notification of a change in tilt of a stylus.
	 *
	 * Measured from surface normal as plane angle in degrees, values
	 * lie in [-90,90]. A positive x is to the right and a positive y
	 * is towards the user.
	 */
	void (*tilt)(void *data,
		     struct zcr_touch_stylus_v2 *zcr_touch_stylus_v2,
		     uint32_t time,
		     uint32_t id,
		     wl_fixed_t tilt_x,
		     wl_fixed_t tilt_y);
};

static inline int
zcr_touch_stylus_v2_add_listener(struct zcr_touch_stylus_v2 *zcr_touch_stylus_v2,
				 const struct zcr_touch_stylus_v2_listener *listener, void *data)
{
	return wl_proxy_add_listener((struct wl_proxy *) zcr_touch_stylus_v2,
				     (void (**)(void)) listener, data);
}

#define ZCR_TOUCH_STYLUS_V2_DESTROY	0

static inline void
zcr_touch_stylus_v2_set_user_data(struct zcr_touch_stylus_v2 *zcr_touch_stylus_v2, void *user_data)
{
	wl_proxy_set_user_data((struct wl_proxy *) zcr_touch_stylus_v2, user_data);
}

static inline void *
zcr_touch_stylus_v2_get_user_data(struct zcr_touch_stylus_v2 *zcr_touch_stylus_v2)
{
	return wl_proxy_get_user_data((struct wl_proxy *) zcr_touch_stylus_v2);
}

static inline void
zcr_touch_stylus_v2_destroy(struct zcr_touch_stylus_v2 *zcr_touch_stylus_v2)
{
	wl_proxy_marshal((struct wl_proxy *) zcr_touch_stylus_v2,
			 ZCR_TOUCH_STYLUS_V2_DESTROY);

	wl_proxy_destroy((struct wl_proxy *) zcr_touch_stylus_v2);
}

#ifdef  __cplusplus
}
#endif

#endif
