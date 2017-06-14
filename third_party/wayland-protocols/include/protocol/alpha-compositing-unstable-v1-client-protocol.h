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

#ifndef ALPHA_COMPOSITING_UNSTABLE_V1_CLIENT_PROTOCOL_H
#define ALPHA_COMPOSITING_UNSTABLE_V1_CLIENT_PROTOCOL_H

#ifdef  __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stddef.h>
#include "wayland-client.h"

struct wl_client;
struct wl_resource;

struct wl_surface;
struct zcr_alpha_compositing_v1;
struct zcr_blending_v1;

extern const struct wl_interface zcr_alpha_compositing_v1_interface;
extern const struct wl_interface zcr_blending_v1_interface;

#ifndef ZCR_ALPHA_COMPOSITING_V1_ERROR_ENUM
#define ZCR_ALPHA_COMPOSITING_V1_ERROR_ENUM
enum zcr_alpha_compositing_v1_error {
	ZCR_ALPHA_COMPOSITING_V1_ERROR_BLENDING_EXISTS = 0,
};
#endif /* ZCR_ALPHA_COMPOSITING_V1_ERROR_ENUM */

#define ZCR_ALPHA_COMPOSITING_V1_DESTROY	0
#define ZCR_ALPHA_COMPOSITING_V1_GET_BLENDING	1

static inline void
zcr_alpha_compositing_v1_set_user_data(struct zcr_alpha_compositing_v1 *zcr_alpha_compositing_v1, void *user_data)
{
	wl_proxy_set_user_data((struct wl_proxy *) zcr_alpha_compositing_v1, user_data);
}

static inline void *
zcr_alpha_compositing_v1_get_user_data(struct zcr_alpha_compositing_v1 *zcr_alpha_compositing_v1)
{
	return wl_proxy_get_user_data((struct wl_proxy *) zcr_alpha_compositing_v1);
}

static inline void
zcr_alpha_compositing_v1_destroy(struct zcr_alpha_compositing_v1 *zcr_alpha_compositing_v1)
{
	wl_proxy_marshal((struct wl_proxy *) zcr_alpha_compositing_v1,
			 ZCR_ALPHA_COMPOSITING_V1_DESTROY);

	wl_proxy_destroy((struct wl_proxy *) zcr_alpha_compositing_v1);
}

static inline struct zcr_blending_v1 *
zcr_alpha_compositing_v1_get_blending(struct zcr_alpha_compositing_v1 *zcr_alpha_compositing_v1, struct wl_surface *surface)
{
	struct wl_proxy *id;

	id = wl_proxy_marshal_constructor((struct wl_proxy *) zcr_alpha_compositing_v1,
			 ZCR_ALPHA_COMPOSITING_V1_GET_BLENDING, &zcr_blending_v1_interface, NULL, surface);

	return (struct zcr_blending_v1 *) id;
}

#ifndef ZCR_BLENDING_V1_BLENDING_EQUATION_ENUM
#define ZCR_BLENDING_V1_BLENDING_EQUATION_ENUM
/**
 * zcr_blending_v1_blending_equation - different blending equations for
 *	compositing
 * @ZCR_BLENDING_V1_BLENDING_EQUATION_NONE: no blending
 * @ZCR_BLENDING_V1_BLENDING_EQUATION_PREMULT: one / one_minus_src_alpha
 * @ZCR_BLENDING_V1_BLENDING_EQUATION_COVERAGE: src_alpha /
 *	one_minus_src_alpha
 *
 * Blending equations that can be used when compositing a surface.
 */
enum zcr_blending_v1_blending_equation {
	ZCR_BLENDING_V1_BLENDING_EQUATION_NONE = 0,
	ZCR_BLENDING_V1_BLENDING_EQUATION_PREMULT = 1,
	ZCR_BLENDING_V1_BLENDING_EQUATION_COVERAGE = 2,
};
#endif /* ZCR_BLENDING_V1_BLENDING_EQUATION_ENUM */

#define ZCR_BLENDING_V1_DESTROY	0
#define ZCR_BLENDING_V1_SET_BLENDING	1
#define ZCR_BLENDING_V1_SET_ALPHA	2

static inline void
zcr_blending_v1_set_user_data(struct zcr_blending_v1 *zcr_blending_v1, void *user_data)
{
	wl_proxy_set_user_data((struct wl_proxy *) zcr_blending_v1, user_data);
}

static inline void *
zcr_blending_v1_get_user_data(struct zcr_blending_v1 *zcr_blending_v1)
{
	return wl_proxy_get_user_data((struct wl_proxy *) zcr_blending_v1);
}

static inline void
zcr_blending_v1_destroy(struct zcr_blending_v1 *zcr_blending_v1)
{
	wl_proxy_marshal((struct wl_proxy *) zcr_blending_v1,
			 ZCR_BLENDING_V1_DESTROY);

	wl_proxy_destroy((struct wl_proxy *) zcr_blending_v1);
}

static inline void
zcr_blending_v1_set_blending(struct zcr_blending_v1 *zcr_blending_v1, uint32_t equation)
{
	wl_proxy_marshal((struct wl_proxy *) zcr_blending_v1,
			 ZCR_BLENDING_V1_SET_BLENDING, equation);
}

static inline void
zcr_blending_v1_set_alpha(struct zcr_blending_v1 *zcr_blending_v1, wl_fixed_t value)
{
	wl_proxy_marshal((struct wl_proxy *) zcr_blending_v1,
			 ZCR_BLENDING_V1_SET_ALPHA, value);
}

#ifdef  __cplusplus
}
#endif

#endif
