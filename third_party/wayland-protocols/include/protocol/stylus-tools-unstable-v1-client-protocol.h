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

#ifndef STYLUS_TOOLS_UNSTABLE_V1_CLIENT_PROTOCOL_H
#define STYLUS_TOOLS_UNSTABLE_V1_CLIENT_PROTOCOL_H

#ifdef  __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stddef.h>
#include "wayland-client.h"

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

#define ZCR_STYLUS_TOOLS_V1_DESTROY	0
#define ZCR_STYLUS_TOOLS_V1_GET_STYLUS_TOOL	1

static inline void
zcr_stylus_tools_v1_set_user_data(struct zcr_stylus_tools_v1 *zcr_stylus_tools_v1, void *user_data)
{
	wl_proxy_set_user_data((struct wl_proxy *) zcr_stylus_tools_v1, user_data);
}

static inline void *
zcr_stylus_tools_v1_get_user_data(struct zcr_stylus_tools_v1 *zcr_stylus_tools_v1)
{
	return wl_proxy_get_user_data((struct wl_proxy *) zcr_stylus_tools_v1);
}

static inline void
zcr_stylus_tools_v1_destroy(struct zcr_stylus_tools_v1 *zcr_stylus_tools_v1)
{
	wl_proxy_marshal((struct wl_proxy *) zcr_stylus_tools_v1,
			 ZCR_STYLUS_TOOLS_V1_DESTROY);

	wl_proxy_destroy((struct wl_proxy *) zcr_stylus_tools_v1);
}

static inline struct zcr_stylus_tool_v1 *
zcr_stylus_tools_v1_get_stylus_tool(struct zcr_stylus_tools_v1 *zcr_stylus_tools_v1, struct wl_surface *surface)
{
	struct wl_proxy *id;

	id = wl_proxy_marshal_constructor((struct wl_proxy *) zcr_stylus_tools_v1,
			 ZCR_STYLUS_TOOLS_V1_GET_STYLUS_TOOL, &zcr_stylus_tool_v1_interface, NULL, surface);

	return (struct zcr_stylus_tool_v1 *) id;
}

#define ZCR_STYLUS_TOOL_V1_DESTROY	0
#define ZCR_STYLUS_TOOL_V1_SET_STYLUS_ONLY	1

static inline void
zcr_stylus_tool_v1_set_user_data(struct zcr_stylus_tool_v1 *zcr_stylus_tool_v1, void *user_data)
{
	wl_proxy_set_user_data((struct wl_proxy *) zcr_stylus_tool_v1, user_data);
}

static inline void *
zcr_stylus_tool_v1_get_user_data(struct zcr_stylus_tool_v1 *zcr_stylus_tool_v1)
{
	return wl_proxy_get_user_data((struct wl_proxy *) zcr_stylus_tool_v1);
}

static inline void
zcr_stylus_tool_v1_destroy(struct zcr_stylus_tool_v1 *zcr_stylus_tool_v1)
{
	wl_proxy_marshal((struct wl_proxy *) zcr_stylus_tool_v1,
			 ZCR_STYLUS_TOOL_V1_DESTROY);

	wl_proxy_destroy((struct wl_proxy *) zcr_stylus_tool_v1);
}

static inline void
zcr_stylus_tool_v1_set_stylus_only(struct zcr_stylus_tool_v1 *zcr_stylus_tool_v1)
{
	wl_proxy_marshal((struct wl_proxy *) zcr_stylus_tool_v1,
			 ZCR_STYLUS_TOOL_V1_SET_STYLUS_ONLY);
}

#ifdef  __cplusplus
}
#endif

#endif
