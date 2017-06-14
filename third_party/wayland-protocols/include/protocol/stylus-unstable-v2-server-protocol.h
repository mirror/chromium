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

#ifndef STYLUS_UNSTABLE_V2_SERVER_PROTOCOL_H
#define STYLUS_UNSTABLE_V2_SERVER_PROTOCOL_H

#ifdef  __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stddef.h>
#include "wayland-server.h"

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

/**
 * zcr_stylus_v2 - extends wl_touch with events for on-screen stylus
 * @get_touch_stylus: get stylus interface for touch
 *
 * Allows a wl_touch to report stylus specific information. The client
 * can interpret the on-screen stylus like any other touch event, and use
 * this protocol to obtain detail information about the type of stylus, as
 * well as the force and tilt of the tool.
 *
 * These events are to be fired by the server within the same frame as
 * other wl_touch events.
 *
 * Warning! The protocol described in this file is experimental and
 * backward incompatible changes may be made. Backward compatible changes
 * may be added together with the corresponding uinterface version bump.
 * Backward incompatible changes are done by bumping the version number in
 * the protocol and uinterface names and resetting the interface version.
 * Once the protocol is to be declared stable, the 'z' prefix and the
 * version number in the protocol and interface names are removed and the
 * interface version number is reset.
 */
struct zcr_stylus_v2_interface {
	/**
	 * get_touch_stylus - get stylus interface for touch
	 * @id: (none)
	 * @touch: (none)
	 *
	 * Create touch_stylus object. See zcr_touch_stylus_v1 interface
	 * for details. If the given wl_touch already has a touch_stylus
	 * object associated, the touch_stylus_exists protocol error is
	 * raised.
	 */
	void (*get_touch_stylus)(struct wl_client *client,
				 struct wl_resource *resource,
				 uint32_t id,
				 struct wl_resource *touch);
};


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
 * @destroy: destroy stylus object
 *
 * The zcr_touch_stylus_v1 interface extends the wl_touch interface with
 * events to describe details about a stylus.
 */
struct zcr_touch_stylus_v2_interface {
	/**
	 * destroy - destroy stylus object
	 *
	 * 
	 */
	void (*destroy)(struct wl_client *client,
			struct wl_resource *resource);
};

#define ZCR_TOUCH_STYLUS_V2_TOOL	0
#define ZCR_TOUCH_STYLUS_V2_FORCE	1
#define ZCR_TOUCH_STYLUS_V2_TILT	2

#define ZCR_TOUCH_STYLUS_V2_TOOL_SINCE_VERSION	1
#define ZCR_TOUCH_STYLUS_V2_FORCE_SINCE_VERSION	1
#define ZCR_TOUCH_STYLUS_V2_TILT_SINCE_VERSION	1

static inline void
zcr_touch_stylus_v2_send_tool(struct wl_resource *resource_, uint32_t id, uint32_t type)
{
	wl_resource_post_event(resource_, ZCR_TOUCH_STYLUS_V2_TOOL, id, type);
}

static inline void
zcr_touch_stylus_v2_send_force(struct wl_resource *resource_, uint32_t time, uint32_t id, wl_fixed_t force)
{
	wl_resource_post_event(resource_, ZCR_TOUCH_STYLUS_V2_FORCE, time, id, force);
}

static inline void
zcr_touch_stylus_v2_send_tilt(struct wl_resource *resource_, uint32_t time, uint32_t id, wl_fixed_t tilt_x, wl_fixed_t tilt_y)
{
	wl_resource_post_event(resource_, ZCR_TOUCH_STYLUS_V2_TILT, time, id, tilt_x, tilt_y);
}

#ifdef  __cplusplus
}
#endif

#endif
