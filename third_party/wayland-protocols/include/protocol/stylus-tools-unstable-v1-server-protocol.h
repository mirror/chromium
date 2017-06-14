/* 
 * Copyright 2017 The Chromium Authors.
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

#ifndef STYLUS_TOOLS_UNSTABLE_V1_SERVER_PROTOCOL_H
#define STYLUS_TOOLS_UNSTABLE_V1_SERVER_PROTOCOL_H

#ifdef  __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stddef.h>
#include "wayland-server.h"

struct wl_client;
struct wl_resource;

struct wl_surface;
struct zcr_stylus_tool_v1;
struct zcr_stylus_tools_v1;

extern const struct wl_interface zcr_stylus_tools_v1_interface;
extern const struct wl_interface zcr_stylus_tool_v1_interface;

#ifndef ZCR_STYLUS_TOOLS_V1_ERROR_ENUM
#define ZCR_STYLUS_TOOLS_V1_ERROR_ENUM
enum zcr_stylus_tools_v1_error {
	ZCR_STYLUS_TOOLS_V1_ERROR_STYLUS_TOOL_EXISTS = 0,
};
#endif /* ZCR_STYLUS_TOOLS_V1_ERROR_ENUM */

/**
 * zcr_stylus_tools_v1 - stylus_tools
 * @destroy: unbind from the stylus_tools interface
 * @get_stylus_tool: extend surface interface for stylus_tool
 *
 * The global interface is used to instantiate an interface extension for
 * a wl_surface object. This extended interface will then allow the client
 * to control the stylus-related behavior for input device event processing
 * related to wl_surface.
 */
struct zcr_stylus_tools_v1_interface {
	/**
	 * destroy - unbind from the stylus_tools interface
	 *
	 * Informs the server that the client will not be using this
	 * protocol object anymore. This does not affect any other objects,
	 * stylus_tool objects included.
	 */
	void (*destroy)(struct wl_client *client,
			struct wl_resource *resource);
	/**
	 * get_stylus_tool - extend surface interface for stylus_tool
	 * @id: the new stylus_tool interface id
	 * @surface: the surface
	 *
	 * Instantiate an interface extension for the given wl_surface to
	 * provide access to the stylus tools features. If the given
	 * wl_surface already has an stylus_tool object associated, the
	 * stylus_tool_exists protocol protocol error is raised.
	 */
	void (*get_stylus_tool)(struct wl_client *client,
				struct wl_resource *resource,
				uint32_t id,
				struct wl_resource *surface);
};


/**
 * zcr_stylus_tool_v1 - stylus_tool interface to a wl_surface
 * @destroy: remove stylus_tool from the surface
 * @set_stylus_only: Set the stylus-only mode
 *
 * An additional interface to a wl_surface object, which allows the
 * client to control the behavior of stylus tools.
 *
 * If the wl_surface associated with the stylus_tool object is destroyed,
 * the stylus_tool object becomes inert.
 *
 * If the stylus_tool object is destroyed, the stylus_tool state is removed
 * from the wl_surface. The change will be applied on the next
 * wl_surface.commit.
 */
struct zcr_stylus_tool_v1_interface {
	/**
	 * destroy - remove stylus_tool from the surface
	 *
	 * The associated wl_surface's stylus_tool state is removed. The
	 * change is applied on the next wl_surface.commit.
	 */
	void (*destroy)(struct wl_client *client,
			struct wl_resource *resource);
	/**
	 * set_stylus_only - Set the stylus-only mode
	 *
	 * Enables the "stylus-only" mode for input device event
	 * processing related to wl_surface.
	 */
	void (*set_stylus_only)(struct wl_client *client,
				struct wl_resource *resource);
};


#ifdef  __cplusplus
}
#endif

#endif
