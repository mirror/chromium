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

#ifndef SECURE_OUTPUT_UNSTABLE_V1_CLIENT_PROTOCOL_H
#define SECURE_OUTPUT_UNSTABLE_V1_CLIENT_PROTOCOL_H

#ifdef  __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stddef.h>
#include "wayland-client.h"

struct wl_client;
struct wl_resource;

struct wl_surface;
struct zcr_secure_output_v1;
struct zcr_security_v1;

extern const struct wl_interface zcr_secure_output_v1_interface;
extern const struct wl_interface zcr_security_v1_interface;

#ifndef ZCR_SECURE_OUTPUT_V1_ERROR_ENUM
#define ZCR_SECURE_OUTPUT_V1_ERROR_ENUM
enum zcr_secure_output_v1_error {
	ZCR_SECURE_OUTPUT_V1_ERROR_SECURITY_EXISTS = 0,
};
#endif /* ZCR_SECURE_OUTPUT_V1_ERROR_ENUM */

#define ZCR_SECURE_OUTPUT_V1_DESTROY	0
#define ZCR_SECURE_OUTPUT_V1_GET_SECURITY	1

static inline void
zcr_secure_output_v1_set_user_data(struct zcr_secure_output_v1 *zcr_secure_output_v1, void *user_data)
{
	wl_proxy_set_user_data((struct wl_proxy *) zcr_secure_output_v1, user_data);
}

static inline void *
zcr_secure_output_v1_get_user_data(struct zcr_secure_output_v1 *zcr_secure_output_v1)
{
	return wl_proxy_get_user_data((struct wl_proxy *) zcr_secure_output_v1);
}

static inline void
zcr_secure_output_v1_destroy(struct zcr_secure_output_v1 *zcr_secure_output_v1)
{
	wl_proxy_marshal((struct wl_proxy *) zcr_secure_output_v1,
			 ZCR_SECURE_OUTPUT_V1_DESTROY);

	wl_proxy_destroy((struct wl_proxy *) zcr_secure_output_v1);
}

static inline struct zcr_security_v1 *
zcr_secure_output_v1_get_security(struct zcr_secure_output_v1 *zcr_secure_output_v1, struct wl_surface *surface)
{
	struct wl_proxy *id;

	id = wl_proxy_marshal_constructor((struct wl_proxy *) zcr_secure_output_v1,
			 ZCR_SECURE_OUTPUT_V1_GET_SECURITY, &zcr_security_v1_interface, NULL, surface);

	return (struct zcr_security_v1 *) id;
}

#define ZCR_SECURITY_V1_DESTROY	0
#define ZCR_SECURITY_V1_ONLY_VISIBLE_ON_SECURE_OUTPUT	1

static inline void
zcr_security_v1_set_user_data(struct zcr_security_v1 *zcr_security_v1, void *user_data)
{
	wl_proxy_set_user_data((struct wl_proxy *) zcr_security_v1, user_data);
}

static inline void *
zcr_security_v1_get_user_data(struct zcr_security_v1 *zcr_security_v1)
{
	return wl_proxy_get_user_data((struct wl_proxy *) zcr_security_v1);
}

static inline void
zcr_security_v1_destroy(struct zcr_security_v1 *zcr_security_v1)
{
	wl_proxy_marshal((struct wl_proxy *) zcr_security_v1,
			 ZCR_SECURITY_V1_DESTROY);

	wl_proxy_destroy((struct wl_proxy *) zcr_security_v1);
}

static inline void
zcr_security_v1_only_visible_on_secure_output(struct zcr_security_v1 *zcr_security_v1)
{
	wl_proxy_marshal((struct wl_proxy *) zcr_security_v1,
			 ZCR_SECURITY_V1_ONLY_VISIBLE_ON_SECURE_OUTPUT);
}

#ifdef  __cplusplus
}
#endif

#endif
