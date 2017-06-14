/* 
 * Copyright © 2014, 2015 Collabora, Ltd.
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

#ifndef LINUX_DMABUF_UNSTABLE_V1_CLIENT_PROTOCOL_H
#define LINUX_DMABUF_UNSTABLE_V1_CLIENT_PROTOCOL_H

#ifdef  __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stddef.h>
#include "wayland-client.h"

struct wl_client;
struct wl_resource;

struct wl_buffer;
struct zwp_linux_buffer_params_v1;
struct zwp_linux_dmabuf_v1;

extern const struct wl_interface zwp_linux_dmabuf_v1_interface;
extern const struct wl_interface zwp_linux_buffer_params_v1_interface;

/**
 * zwp_linux_dmabuf_v1 - factory for creating dmabuf-based wl_buffers
 * @format: supported buffer format
 * @modifier: supported buffer format modifier
 *
 * Following the interfaces from:
 * https://www.khronos.org/registry/egl/extensions/EXT/EGL_EXT_image_dma_buf_import.txt
 * and the Linux DRM sub-system's AddFb2 ioctl.
 *
 * This interface offers ways to create generic dmabuf-based wl_buffers.
 * Immediately after a client binds to this interface, the set of supported
 * formats and format modifiers is sent with 'format' and 'modifier'
 * events.
 *
 * The following are required from clients:
 *
 * - Clients must ensure that either all data in the dma-buf is coherent
 * for all subsequent read access or that coherency is correctly handled by
 * the underlying kernel-side dma-buf implementation.
 *
 * - Don't make any more attachments after sending the buffer to the
 * compositor. Making more attachments later increases the risk of the
 * compositor not being able to use (re-import) an existing dmabuf-based
 * wl_buffer.
 *
 * The underlying graphics stack must ensure the following:
 *
 * - The dmabuf file descriptors relayed to the server will stay valid for
 * the whole lifetime of the wl_buffer. This means the server may at any
 * time use those fds to import the dmabuf into any kernel sub-system that
 * might accept it.
 *
 * To create a wl_buffer from one or more dmabufs, a client creates a
 * zwp_linux_dmabuf_params_v1 object with a
 * zwp_linux_dmabuf_v1.create_params request. All planes required by the
 * intended format are added with the 'add' request. Finally, a 'create' or
 * 'create_immed' request is issued, which has the following outcome
 * depending on the import success.
 *
 * The 'create' request, - on success, triggers a 'created' event which
 * provides the final wl_buffer to the client. - on failure, triggers a
 * 'failed' event to convey that the server cannot use the dmabufs received
 * from the client.
 *
 * For the 'create_immed' request, - on success, the server immediately
 * imports the added dmabufs to create a wl_buffer. No event is sent from
 * the server in this case. - on failure, the server can choose to either:
 * - terminate the client by raising a fatal error. - mark the wl_buffer as
 * failed, and send a 'failed' event to the client. If the client uses a
 * failed wl_buffer as an argument to any request, the behaviour is
 * compositor implementation-defined.
 *
 * Warning! The protocol described in this file is experimental and
 * backward incompatible changes may be made. Backward compatible changes
 * may be added together with the corresponding interface version bump.
 * Backward incompatible changes are done by bumping the version number in
 * the protocol and interface names and resetting the interface version.
 * Once the protocol is to be declared stable, the 'z' prefix and the
 * version number in the protocol and interface names are removed and the
 * interface version number is reset.
 */
struct zwp_linux_dmabuf_v1_listener {
	/**
	 * format - supported buffer format
	 * @format: DRM_FORMAT code
	 *
	 * This event advertises one buffer format that the server
	 * supports. All the supported formats are advertised once when the
	 * client binds to this interface. A roundtrip after binding
	 * guarantees that the client has received all supported formats.
	 *
	 * For the definition of the format codes, see the
	 * zwp_linux_buffer_params_v1::create request.
	 *
	 * Warning: the 'format' event is likely to be deprecated and
	 * replaced with the 'modifier' event introduced in
	 * zwp_linux_dmabuf_v1 version 3, described below. Please refrain
	 * from using the information received from this event.
	 */
	void (*format)(void *data,
		       struct zwp_linux_dmabuf_v1 *zwp_linux_dmabuf_v1,
		       uint32_t format);
	/**
	 * modifier - supported buffer format modifier
	 * @format: DRM_FORMAT code
	 * @modifier_hi: high 32 bits of layout modifier
	 * @modifier_lo: low 32 bits of layout modifier
	 *
	 * This event advertises the formats that the server supports,
	 * along with the modifiers supported for each format. All the
	 * supported modifiers for all the supported formats are advertised
	 * once when the client binds to this interface. A roundtrip after
	 * binding guarantees that the client has received all supported
	 * format-modifier pairs.
	 *
	 * For the definition of the format and modifier codes, see the
	 * zwp_linux_buffer_params_v1::create request.
	 * @since: 3
	 */
	void (*modifier)(void *data,
			 struct zwp_linux_dmabuf_v1 *zwp_linux_dmabuf_v1,
			 uint32_t format,
			 uint32_t modifier_hi,
			 uint32_t modifier_lo);
};

static inline int
zwp_linux_dmabuf_v1_add_listener(struct zwp_linux_dmabuf_v1 *zwp_linux_dmabuf_v1,
				 const struct zwp_linux_dmabuf_v1_listener *listener, void *data)
{
	return wl_proxy_add_listener((struct wl_proxy *) zwp_linux_dmabuf_v1,
				     (void (**)(void)) listener, data);
}

#define ZWP_LINUX_DMABUF_V1_DESTROY	0
#define ZWP_LINUX_DMABUF_V1_CREATE_PARAMS	1

static inline void
zwp_linux_dmabuf_v1_set_user_data(struct zwp_linux_dmabuf_v1 *zwp_linux_dmabuf_v1, void *user_data)
{
	wl_proxy_set_user_data((struct wl_proxy *) zwp_linux_dmabuf_v1, user_data);
}

static inline void *
zwp_linux_dmabuf_v1_get_user_data(struct zwp_linux_dmabuf_v1 *zwp_linux_dmabuf_v1)
{
	return wl_proxy_get_user_data((struct wl_proxy *) zwp_linux_dmabuf_v1);
}

static inline void
zwp_linux_dmabuf_v1_destroy(struct zwp_linux_dmabuf_v1 *zwp_linux_dmabuf_v1)
{
	wl_proxy_marshal((struct wl_proxy *) zwp_linux_dmabuf_v1,
			 ZWP_LINUX_DMABUF_V1_DESTROY);

	wl_proxy_destroy((struct wl_proxy *) zwp_linux_dmabuf_v1);
}

static inline struct zwp_linux_buffer_params_v1 *
zwp_linux_dmabuf_v1_create_params(struct zwp_linux_dmabuf_v1 *zwp_linux_dmabuf_v1)
{
	struct wl_proxy *params_id;

	params_id = wl_proxy_marshal_constructor((struct wl_proxy *) zwp_linux_dmabuf_v1,
			 ZWP_LINUX_DMABUF_V1_CREATE_PARAMS, &zwp_linux_buffer_params_v1_interface, NULL);

	return (struct zwp_linux_buffer_params_v1 *) params_id;
}

#ifndef ZWP_LINUX_BUFFER_PARAMS_V1_ERROR_ENUM
#define ZWP_LINUX_BUFFER_PARAMS_V1_ERROR_ENUM
enum zwp_linux_buffer_params_v1_error {
	ZWP_LINUX_BUFFER_PARAMS_V1_ERROR_ALREADY_USED = 0,
	ZWP_LINUX_BUFFER_PARAMS_V1_ERROR_PLANE_IDX = 1,
	ZWP_LINUX_BUFFER_PARAMS_V1_ERROR_PLANE_SET = 2,
	ZWP_LINUX_BUFFER_PARAMS_V1_ERROR_INCOMPLETE = 3,
	ZWP_LINUX_BUFFER_PARAMS_V1_ERROR_INVALID_FORMAT = 4,
	ZWP_LINUX_BUFFER_PARAMS_V1_ERROR_INVALID_DIMENSIONS = 5,
	ZWP_LINUX_BUFFER_PARAMS_V1_ERROR_OUT_OF_BOUNDS = 6,
	ZWP_LINUX_BUFFER_PARAMS_V1_ERROR_INVALID_WL_BUFFER = 7,
};
#endif /* ZWP_LINUX_BUFFER_PARAMS_V1_ERROR_ENUM */

#ifndef ZWP_LINUX_BUFFER_PARAMS_V1_FLAGS_ENUM
#define ZWP_LINUX_BUFFER_PARAMS_V1_FLAGS_ENUM
enum zwp_linux_buffer_params_v1_flags {
	ZWP_LINUX_BUFFER_PARAMS_V1_FLAGS_Y_INVERT = 1,
	ZWP_LINUX_BUFFER_PARAMS_V1_FLAGS_INTERLACED = 2,
	ZWP_LINUX_BUFFER_PARAMS_V1_FLAGS_BOTTOM_FIRST = 4,
};
#endif /* ZWP_LINUX_BUFFER_PARAMS_V1_FLAGS_ENUM */

/**
 * zwp_linux_buffer_params_v1 - parameters for creating a dmabuf-based
 *	wl_buffer
 * @created: buffer creation succeeded
 * @failed: buffer creation failed
 *
 * This temporary object is a collection of dmabufs and other parameters
 * that together form a single logical buffer. The temporary object may
 * eventually create one wl_buffer unless cancelled by destroying it before
 * requesting 'create'.
 *
 * Single-planar formats only require one dmabuf, however multi-planar
 * formats may require more than one dmabuf. For all formats, an 'add'
 * request must be called once per plane (even if the underlying dmabuf fd
 * is identical).
 *
 * You must use consecutive plane indices ('plane_idx' argument for 'add')
 * from zero to the number of planes used by the drm_fourcc format code.
 * All planes required by the format must be given exactly once, but can be
 * given in any order. Each plane index can be set only once.
 */
struct zwp_linux_buffer_params_v1_listener {
	/**
	 * created - buffer creation succeeded
	 * @buffer: the newly created wl_buffer
	 *
	 * This event indicates that the attempted buffer creation was
	 * successful. It provides the new wl_buffer referencing the
	 * dmabuf(s).
	 *
	 * Upon receiving this event, the client should destroy the
	 * zlinux_dmabuf_params object.
	 */
	void (*created)(void *data,
			struct zwp_linux_buffer_params_v1 *zwp_linux_buffer_params_v1,
			struct wl_buffer *buffer);
	/**
	 * failed - buffer creation failed
	 *
	 * This event indicates that the attempted buffer creation has
	 * failed. It usually means that one of the dmabuf constraints has
	 * not been fulfilled.
	 *
	 * Upon receiving this event, the client should destroy the
	 * zlinux_buffer_params object.
	 */
	void (*failed)(void *data,
		       struct zwp_linux_buffer_params_v1 *zwp_linux_buffer_params_v1);
};

static inline int
zwp_linux_buffer_params_v1_add_listener(struct zwp_linux_buffer_params_v1 *zwp_linux_buffer_params_v1,
					const struct zwp_linux_buffer_params_v1_listener *listener, void *data)
{
	return wl_proxy_add_listener((struct wl_proxy *) zwp_linux_buffer_params_v1,
				     (void (**)(void)) listener, data);
}

#define ZWP_LINUX_BUFFER_PARAMS_V1_DESTROY	0
#define ZWP_LINUX_BUFFER_PARAMS_V1_ADD	1
#define ZWP_LINUX_BUFFER_PARAMS_V1_CREATE	2
#define ZWP_LINUX_BUFFER_PARAMS_V1_CREATE_IMMED	3

static inline void
zwp_linux_buffer_params_v1_set_user_data(struct zwp_linux_buffer_params_v1 *zwp_linux_buffer_params_v1, void *user_data)
{
	wl_proxy_set_user_data((struct wl_proxy *) zwp_linux_buffer_params_v1, user_data);
}

static inline void *
zwp_linux_buffer_params_v1_get_user_data(struct zwp_linux_buffer_params_v1 *zwp_linux_buffer_params_v1)
{
	return wl_proxy_get_user_data((struct wl_proxy *) zwp_linux_buffer_params_v1);
}

static inline void
zwp_linux_buffer_params_v1_destroy(struct zwp_linux_buffer_params_v1 *zwp_linux_buffer_params_v1)
{
	wl_proxy_marshal((struct wl_proxy *) zwp_linux_buffer_params_v1,
			 ZWP_LINUX_BUFFER_PARAMS_V1_DESTROY);

	wl_proxy_destroy((struct wl_proxy *) zwp_linux_buffer_params_v1);
}

static inline void
zwp_linux_buffer_params_v1_add(struct zwp_linux_buffer_params_v1 *zwp_linux_buffer_params_v1, int32_t fd, uint32_t plane_idx, uint32_t offset, uint32_t stride, uint32_t modifier_hi, uint32_t modifier_lo)
{
	wl_proxy_marshal((struct wl_proxy *) zwp_linux_buffer_params_v1,
			 ZWP_LINUX_BUFFER_PARAMS_V1_ADD, fd, plane_idx, offset, stride, modifier_hi, modifier_lo);
}

static inline void
zwp_linux_buffer_params_v1_create(struct zwp_linux_buffer_params_v1 *zwp_linux_buffer_params_v1, int32_t width, int32_t height, uint32_t format, uint32_t flags)
{
	wl_proxy_marshal((struct wl_proxy *) zwp_linux_buffer_params_v1,
			 ZWP_LINUX_BUFFER_PARAMS_V1_CREATE, width, height, format, flags);
}

static inline struct wl_buffer *
zwp_linux_buffer_params_v1_create_immed(struct zwp_linux_buffer_params_v1 *zwp_linux_buffer_params_v1, int32_t width, int32_t height, uint32_t format, uint32_t flags)
{
	struct wl_proxy *buffer_id;

	buffer_id = wl_proxy_marshal_constructor((struct wl_proxy *) zwp_linux_buffer_params_v1,
			 ZWP_LINUX_BUFFER_PARAMS_V1_CREATE_IMMED, &wl_buffer_interface, NULL, width, height, format, flags);

	return (struct wl_buffer *) buffer_id;
}

#ifdef  __cplusplus
}
#endif

#endif
