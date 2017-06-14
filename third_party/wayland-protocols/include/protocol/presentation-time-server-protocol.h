/* 
 * Copyright © 2013-2014 Collabora, Ltd.
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

#ifndef PRESENTATION_TIME_SERVER_PROTOCOL_H
#define PRESENTATION_TIME_SERVER_PROTOCOL_H

#ifdef  __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stddef.h>
#include "wayland-server.h"

struct wl_client;
struct wl_resource;

struct wl_output;
struct wl_surface;
struct wp_presentation;
struct wp_presentation_feedback;

extern const struct wl_interface wp_presentation_interface;
extern const struct wl_interface wp_presentation_feedback_interface;

#ifndef WP_PRESENTATION_ERROR_ENUM
#define WP_PRESENTATION_ERROR_ENUM
/**
 * wp_presentation_error - fatal presentation errors
 * @WP_PRESENTATION_ERROR_INVALID_TIMESTAMP: invalid value in tv_nsec
 * @WP_PRESENTATION_ERROR_INVALID_FLAG: invalid flag
 *
 * These fatal protocol errors may be emitted in response to illegal
 * presentation requests.
 */
enum wp_presentation_error {
	WP_PRESENTATION_ERROR_INVALID_TIMESTAMP = 0,
	WP_PRESENTATION_ERROR_INVALID_FLAG = 1,
};
#endif /* WP_PRESENTATION_ERROR_ENUM */

/**
 * wp_presentation - timed presentation related wl_surface requests
 * @destroy: unbind from the presentation interface
 * @feedback: request presentation feedback information
 *
 * 
 *
 * The main feature of this interface is accurate presentation timing
 * feedback to ensure smooth video playback while maintaining audio/video
 * synchronization. Some features use the concept of a presentation clock,
 * which is defined in the presentation.clock_id event.
 *
 * A content update for a wl_surface is submitted by a wl_surface.commit
 * request. Request 'feedback' associates with the wl_surface.commit and
 * provides feedback on the content update, particularly the final realized
 * presentation time.
 *
 * When the final realized presentation time is available, e.g. after a
 * framebuffer flip completes, the requested
 * presentation_feedback.presented events are sent. The final presentation
 * time can differ from the compositor's predicted display update time and
 * the update's target time, especially when the compositor misses its
 * target vertical blanking period.
 */
struct wp_presentation_interface {
	/**
	 * destroy - unbind from the presentation interface
	 *
	 * Informs the server that the client will no longer be using
	 * this protocol object. Existing objects created by this object
	 * are not affected.
	 */
	void (*destroy)(struct wl_client *client,
			struct wl_resource *resource);
	/**
	 * feedback - request presentation feedback information
	 * @surface: target surface
	 * @callback: new feedback object
	 *
	 * Request presentation feedback for the current content
	 * submission on the given surface. This creates a new
	 * presentation_feedback object, which will deliver the feedback
	 * information once. If multiple presentation_feedback objects are
	 * created for the same submission, they will all deliver the same
	 * information.
	 *
	 * For details on what information is returned, see the
	 * presentation_feedback interface.
	 */
	void (*feedback)(struct wl_client *client,
			 struct wl_resource *resource,
			 struct wl_resource *surface,
			 uint32_t callback);
};

#define WP_PRESENTATION_CLOCK_ID	0

#define WP_PRESENTATION_CLOCK_ID_SINCE_VERSION	1

static inline void
wp_presentation_send_clock_id(struct wl_resource *resource_, uint32_t clk_id)
{
	wl_resource_post_event(resource_, WP_PRESENTATION_CLOCK_ID, clk_id);
}

#ifndef WP_PRESENTATION_FEEDBACK_KIND_ENUM
#define WP_PRESENTATION_FEEDBACK_KIND_ENUM
/**
 * wp_presentation_feedback_kind - bitmask of flags in presented event
 * @WP_PRESENTATION_FEEDBACK_KIND_VSYNC: presentation was vsync'd
 * @WP_PRESENTATION_FEEDBACK_KIND_HW_CLOCK: hardware provided the
 *	presentation timestamp
 * @WP_PRESENTATION_FEEDBACK_KIND_HW_COMPLETION: hardware signalled the
 *	start of the presentation
 * @WP_PRESENTATION_FEEDBACK_KIND_ZERO_COPY: presentation was done
 *	zero-copy
 *
 * These flags provide information about how the presentation of the
 * related content update was done. The intent is to help clients assess
 * the reliability of the feedback and the visual quality with respect to
 * possible tearing and timings. The flags are:
 *
 * VSYNC: The presentation was synchronized to the "vertical retrace" by
 * the display hardware such that tearing does not happen. Relying on user
 * space scheduling is not acceptable for this flag. If presentation is
 * done by a copy to the active frontbuffer, then it must guarantee that
 * tearing cannot happen.
 *
 * HW_CLOCK: The display hardware provided measurements that the hardware
 * driver converted into a presentation timestamp. Sampling a clock in user
 * space is not acceptable for this flag.
 *
 * HW_COMPLETION: The display hardware signalled that it started using the
 * new image content. The opposite of this is e.g. a timer being used to
 * guess when the display hardware has switched to the new image content.
 *
 * ZERO_COPY: The presentation of this update was done zero-copy. This
 * means the buffer from the client was given to display hardware as is,
 * without copying it. Compositing with OpenGL counts as copying, even if
 * textured directly from the client buffer. Possible zero-copy cases
 * include direct scanout of a fullscreen surface and a surface on a
 * hardware overlay.
 */
enum wp_presentation_feedback_kind {
	WP_PRESENTATION_FEEDBACK_KIND_VSYNC = 0x1,
	WP_PRESENTATION_FEEDBACK_KIND_HW_CLOCK = 0x2,
	WP_PRESENTATION_FEEDBACK_KIND_HW_COMPLETION = 0x4,
	WP_PRESENTATION_FEEDBACK_KIND_ZERO_COPY = 0x8,
};
#endif /* WP_PRESENTATION_FEEDBACK_KIND_ENUM */

#define WP_PRESENTATION_FEEDBACK_SYNC_OUTPUT	0
#define WP_PRESENTATION_FEEDBACK_PRESENTED	1
#define WP_PRESENTATION_FEEDBACK_DISCARDED	2

#define WP_PRESENTATION_FEEDBACK_SYNC_OUTPUT_SINCE_VERSION	1
#define WP_PRESENTATION_FEEDBACK_PRESENTED_SINCE_VERSION	1
#define WP_PRESENTATION_FEEDBACK_DISCARDED_SINCE_VERSION	1

static inline void
wp_presentation_feedback_send_sync_output(struct wl_resource *resource_, struct wl_resource *output)
{
	wl_resource_post_event(resource_, WP_PRESENTATION_FEEDBACK_SYNC_OUTPUT, output);
}

static inline void
wp_presentation_feedback_send_presented(struct wl_resource *resource_, uint32_t tv_sec_hi, uint32_t tv_sec_lo, uint32_t tv_nsec, uint32_t refresh, uint32_t seq_hi, uint32_t seq_lo, uint32_t flags)
{
	wl_resource_post_event(resource_, WP_PRESENTATION_FEEDBACK_PRESENTED, tv_sec_hi, tv_sec_lo, tv_nsec, refresh, seq_hi, seq_lo, flags);
}

static inline void
wp_presentation_feedback_send_discarded(struct wl_resource *resource_)
{
	wl_resource_post_event(resource_, WP_PRESENTATION_FEEDBACK_DISCARDED);
}

#ifdef  __cplusplus
}
#endif

#endif
