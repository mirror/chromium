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

#ifndef REMOTE_SHELL_UNSTABLE_V1_SERVER_PROTOCOL_H
#define REMOTE_SHELL_UNSTABLE_V1_SERVER_PROTOCOL_H

#ifdef  __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stddef.h>
#include "wayland-util.h"

struct wl_client;
struct wl_resource;

struct zcr_remote_shell_v1;
struct zcr_remote_surface_v1;
struct zcr_notification_surface_v1;

extern const struct wl_interface zcr_remote_shell_v1_interface;
extern const struct wl_interface zcr_remote_surface_v1_interface;
extern const struct wl_interface zcr_notification_surface_v1_interface;

#ifndef ZCR_REMOTE_SHELL_V1_CONTAINER_ENUM
#define ZCR_REMOTE_SHELL_V1_CONTAINER_ENUM
/**
 * zcr_remote_shell_v1_container - containers for remote surfaces
 * @ZCR_REMOTE_SHELL_V1_CONTAINER_DEFAULT: default container
 * @ZCR_REMOTE_SHELL_V1_CONTAINER_OVERLAY: system modal container
 *
 * Determine how a remote surface should be stacked relative to other
 * shell surfaces.
 */
enum zcr_remote_shell_v1_container {
	ZCR_REMOTE_SHELL_V1_CONTAINER_DEFAULT = 1,
	ZCR_REMOTE_SHELL_V1_CONTAINER_OVERLAY = 2,
};
#endif /* ZCR_REMOTE_SHELL_V1_CONTAINER_ENUM */

#ifndef ZCR_REMOTE_SHELL_V1_STATE_TYPE_ENUM
#define ZCR_REMOTE_SHELL_V1_STATE_TYPE_ENUM
/**
 * zcr_remote_shell_v1_state_type - state types for remote surfaces
 * @ZCR_REMOTE_SHELL_V1_STATE_TYPE_NORMAL: normal window state
 * @ZCR_REMOTE_SHELL_V1_STATE_TYPE_MINIMIZED: minimized window state
 * @ZCR_REMOTE_SHELL_V1_STATE_TYPE_MAXIMIZED: maximized window state
 * @ZCR_REMOTE_SHELL_V1_STATE_TYPE_FULLSCREEN: fullscreen window state
 * @ZCR_REMOTE_SHELL_V1_STATE_TYPE_PINNED: pinned window state
 * @ZCR_REMOTE_SHELL_V1_STATE_TYPE_TRUSTED_PINNED: trusted pinned window
 *	state
 * @ZCR_REMOTE_SHELL_V1_STATE_TYPE_MOVING: moving window state
 *
 * Defines common show states for shell surfaces.
 */
enum zcr_remote_shell_v1_state_type {
	ZCR_REMOTE_SHELL_V1_STATE_TYPE_NORMAL = 1,
	ZCR_REMOTE_SHELL_V1_STATE_TYPE_MINIMIZED = 2,
	ZCR_REMOTE_SHELL_V1_STATE_TYPE_MAXIMIZED = 3,
	ZCR_REMOTE_SHELL_V1_STATE_TYPE_FULLSCREEN = 4,
	ZCR_REMOTE_SHELL_V1_STATE_TYPE_PINNED = 5,
	ZCR_REMOTE_SHELL_V1_STATE_TYPE_TRUSTED_PINNED = 6,
	ZCR_REMOTE_SHELL_V1_STATE_TYPE_MOVING = 7,
};
#endif /* ZCR_REMOTE_SHELL_V1_STATE_TYPE_ENUM */

#ifndef ZCR_REMOTE_SHELL_V1_ERROR_ENUM
#define ZCR_REMOTE_SHELL_V1_ERROR_ENUM
enum zcr_remote_shell_v1_error {
	ZCR_REMOTE_SHELL_V1_ERROR_ROLE = 0,
	ZCR_REMOTE_SHELL_V1_ERROR_INVALID_NOTIFICATION_ID = 1,
};
#endif /* ZCR_REMOTE_SHELL_V1_ERROR_ENUM */

#ifndef ZCR_REMOTE_SHELL_V1_LAYOUT_MODE_ENUM
#define ZCR_REMOTE_SHELL_V1_LAYOUT_MODE_ENUM
/**
 * zcr_remote_shell_v1_layout_mode - the layout mode
 * @ZCR_REMOTE_SHELL_V1_LAYOUT_MODE_WINDOWED: multiple windows
 * @ZCR_REMOTE_SHELL_V1_LAYOUT_MODE_TABLET: restricted mode for tablet
 *
 * Determine how a client should layout surfaces.
 */
enum zcr_remote_shell_v1_layout_mode {
	ZCR_REMOTE_SHELL_V1_LAYOUT_MODE_WINDOWED = 1,
	ZCR_REMOTE_SHELL_V1_LAYOUT_MODE_TABLET = 2,
};
#endif /* ZCR_REMOTE_SHELL_V1_LAYOUT_MODE_ENUM */

/**
 * zcr_remote_shell_v1 - remote_shell
 * @destroy: destroy remote_shell
 * @get_remote_surface: create a remote shell surface from a surface
 * @get_notification_surface: create a notification surface from a
 *	surface
 *
 * The global interface that allows clients to turn a wl_surface into a
 * "real window" which is remotely managed but can be stacked, activated
 * and made fullscreen by the user.
 */
struct zcr_remote_shell_v1_interface {
	/**
	 * destroy - destroy remote_shell
	 *
	 * Destroy this remote_shell object.
	 *
	 * Destroying a bound remote_shell object while there are surfaces
	 * still alive created by this remote_shell object instance is
	 * illegal and will result in a protocol error.
	 */
	void (*destroy)(struct wl_client *client,
			struct wl_resource *resource);
	/**
	 * get_remote_surface - create a remote shell surface from a
	 *	surface
	 * @id: (none)
	 * @surface: (none)
	 * @container: (none)
	 *
	 * This creates an remote_surface for the given surface and gives
	 * it the remote_surface role. A wl_surface can only be given a
	 * remote_surface role once. If get_remote_surface is called with a
	 * wl_surface that already has an active remote_surface associated
	 * with it, or if it had any other role, an error is raised.
	 *
	 * See the documentation of remote_surface for more details about
	 * what an remote_surface is and how it is used.
	 */
	void (*get_remote_surface)(struct wl_client *client,
				   struct wl_resource *resource,
				   uint32_t id,
				   struct wl_resource *surface,
				   uint32_t container);
	/**
	 * get_notification_surface - create a notification surface from
	 *	a surface
	 * @id: (none)
	 * @surface: (none)
	 * @notification_id: (none)
	 *
	 * Creates a notification_surface for the given surface, gives it
	 * the notification_surface role and associated it with a
	 * notification id.
	 */
	void (*get_notification_surface)(struct wl_client *client,
					 struct wl_resource *resource,
					 uint32_t id,
					 struct wl_resource *surface,
					 const char *notification_id);
};

#define ZCR_REMOTE_SHELL_V1_ACTIVATED	0
#define ZCR_REMOTE_SHELL_V1_CONFIGURATION_CHANGED	1
#define ZCR_REMOTE_SHELL_V1_WORKSPACE	2
#define ZCR_REMOTE_SHELL_V1_CONFIGURE	3

static inline void
zcr_remote_shell_v1_send_activated(struct wl_resource *resource_, struct wl_resource *gained_active, struct wl_resource *lost_active)
{
	wl_resource_post_event(resource_, ZCR_REMOTE_SHELL_V1_ACTIVATED, gained_active, lost_active);
}

static inline void
zcr_remote_shell_v1_send_configuration_changed(struct wl_resource *resource_, int32_t width, int32_t height, int32_t transform, wl_fixed_t scale_factor, int32_t work_area_inset_left, int32_t work_area_inset_top, int32_t work_area_inset_right, int32_t work_area_inset_bottom, uint32_t layout_mode)
{
	wl_resource_post_event(resource_, ZCR_REMOTE_SHELL_V1_CONFIGURATION_CHANGED, width, height, transform, scale_factor, work_area_inset_left, work_area_inset_top, work_area_inset_right, work_area_inset_bottom, layout_mode);
}

static inline void
zcr_remote_shell_v1_send_workspace(struct wl_resource *resource_, uint32_t id_hi, uint32_t id_lo, int32_t x, int32_t y, int32_t width, int32_t height, int32_t inset_left, int32_t inset_top, int32_t inset_right, int32_t inset_bottom, int32_t transform, wl_fixed_t scale_factor, uint32_t is_internal)
{
	wl_resource_post_event(resource_, ZCR_REMOTE_SHELL_V1_WORKSPACE, id_hi, id_lo, x, y, width, height, inset_left, inset_top, inset_right, inset_bottom, transform, scale_factor, is_internal);
}

static inline void
zcr_remote_shell_v1_send_configure(struct wl_resource *resource_, uint32_t layout_mode)
{
	wl_resource_post_event(resource_, ZCR_REMOTE_SHELL_V1_CONFIGURE, layout_mode);
}

#ifndef ZCR_REMOTE_SURFACE_V1_SYSTEMUI_VISIBILITY_STATE_ENUM
#define ZCR_REMOTE_SURFACE_V1_SYSTEMUI_VISIBILITY_STATE_ENUM
/**
 * zcr_remote_surface_v1_systemui_visibility_state - systemui visibility
 *	behavior
 * @ZCR_REMOTE_SURFACE_V1_SYSTEMUI_VISIBILITY_STATE_VISIBLE: system ui is
 *	visible
 * @ZCR_REMOTE_SURFACE_V1_SYSTEMUI_VISIBILITY_STATE_AUTOHIDE_NON_STICKY: 
 *	system ui autohides and is not sticky
 * @ZCR_REMOTE_SURFACE_V1_SYSTEMUI_VISIBILITY_STATE_AUTOHIDE_STICKY: 
 *	system ui autohides and is sticky
 *
 * Determine the visibility behavior of the system UI.
 */
enum zcr_remote_surface_v1_systemui_visibility_state {
	ZCR_REMOTE_SURFACE_V1_SYSTEMUI_VISIBILITY_STATE_VISIBLE = 1,
	ZCR_REMOTE_SURFACE_V1_SYSTEMUI_VISIBILITY_STATE_AUTOHIDE_NON_STICKY = 2,
	ZCR_REMOTE_SURFACE_V1_SYSTEMUI_VISIBILITY_STATE_AUTOHIDE_STICKY = 3,
};
#endif /* ZCR_REMOTE_SURFACE_V1_SYSTEMUI_VISIBILITY_STATE_ENUM */

#ifndef ZCR_REMOTE_SURFACE_V1_ORIENTATION_ENUM
#define ZCR_REMOTE_SURFACE_V1_ORIENTATION_ENUM
/**
 * zcr_remote_surface_v1_orientation - window orientation
 * @ZCR_REMOTE_SURFACE_V1_ORIENTATION_PORTRAIT: portrait
 * @ZCR_REMOTE_SURFACE_V1_ORIENTATION_LANDSCAPE: landscape
 *
 * The orientation of the window.
 */
enum zcr_remote_surface_v1_orientation {
	ZCR_REMOTE_SURFACE_V1_ORIENTATION_PORTRAIT = 1,
	ZCR_REMOTE_SURFACE_V1_ORIENTATION_LANDSCAPE = 2,
};
#endif /* ZCR_REMOTE_SURFACE_V1_ORIENTATION_ENUM */

/**
 * zcr_remote_surface_v1 - A desktop window
 * @destroy: Destroy the remote_surface
 * @set_app_id: set application ID
 * @set_window_geometry: set the new window geometry
 * @set_scale: set scale
 * @set_rectangular_shadow: set a rectangular shadow
 * @set_rectangular_shadow_background_opacity: suggests the window's
 *	background opacity
 * @set_title: set surface title
 * @set_top_inset: set top inset for surface
 * @activate: make the surface active
 * @maximize: maximize
 * @minimize: minimize
 * @restore: restore
 * @fullscreen: fullscreen
 * @unfullscreen: unfullscreen
 * @pin: pin
 * @unpin: unpin
 * @set_system_modal: suggests a re-layout of remote shell input area
 * @unset_system_modal: suggests a re-layout of remote shell input area
 * @set_rectangular_surface_shadow: set a rectangular shadow
 * @set_systemui_visibility: requests the system ui visibility behavior
 *	for the surface
 * @set_always_on_top: set always on top
 * @unset_always_on_top: unset always on top
 * @ack_configure: ack a configure event
 * @move: start an interactive move
 * @set_orientation: set orientation
 *
 * An interface that may be implemented by a wl_surface, for
 * implementations that provide a desktop-style user interface and allows
 * for remotely managed windows.
 *
 * It provides requests to treat surfaces like windows, allowing to set
 * properties like app id and geometry.
 *
 * The client must call wl_surface.commit on the corresponding wl_surface
 * for the remote_surface state to take effect.
 *
 * For a surface to be mapped by the compositor the client must have
 * committed both an remote_surface state and a buffer.
 */
struct zcr_remote_surface_v1_interface {
	/**
	 * destroy - Destroy the remote_surface
	 *
	 * Unmap and destroy the window. The window will be effectively
	 * hidden from the user's point of view, and all state will be
	 * lost.
	 */
	void (*destroy)(struct wl_client *client,
			struct wl_resource *resource);
	/**
	 * set_app_id - set application ID
	 * @app_id: (none)
	 *
	 * Set an application identifier for the surface.
	 */
	void (*set_app_id)(struct wl_client *client,
			   struct wl_resource *resource,
			   const char *app_id);
	/**
	 * set_window_geometry - set the new window geometry
	 * @x: (none)
	 * @y: (none)
	 * @width: (none)
	 * @height: (none)
	 *
	 * The window geometry of a window is its "visible bounds" from
	 * the user's perspective. Client-side decorations often have
	 * invisible portions like drop-shadows which should be ignored for
	 * the purposes of aligning, placing and constraining windows.
	 *
	 * The window geometry is double buffered, and will be applied at
	 * the time wl_surface.commit of the corresponding wl_surface is
	 * called.
	 *
	 * Once the window geometry of the surface is set once, it is not
	 * possible to unset it, and it will remain the same until
	 * set_window_geometry is called again, even if a new subsurface or
	 * buffer is attached.
	 *
	 * If never set, the value is the full bounds of the output. This
	 * updates dynamically on every commit.
	 *
	 * The arguments are given in the output coordinate space.
	 *
	 * The width and height must be greater than zero.
	 */
	void (*set_window_geometry)(struct wl_client *client,
				    struct wl_resource *resource,
				    int32_t x,
				    int32_t y,
				    int32_t width,
				    int32_t height);
	/**
	 * set_scale - set scale
	 * @scale: (none)
	 *
	 * Set a scale factor that will be applied to surface and all
	 * descendants.
	 */
	void (*set_scale)(struct wl_client *client,
			  struct wl_resource *resource,
			  wl_fixed_t scale);
	/**
	 * set_rectangular_shadow - set a rectangular shadow
	 * @x: (none)
	 * @y: (none)
	 * @width: (none)
	 * @height: (none)
	 *
	 * [Deprecated] Request that surface needs a rectangular shadow.
	 *
	 * This is only a request that the surface should have a
	 * rectangular shadow. The compositor may choose to ignore this
	 * request.
	 *
	 * The arguments are given in the output coordinate space and
	 * specifies the inner bounds of the shadow.
	 *
	 * The arguments are given in the output coordinate space.
	 * Specifying zero width and height will disable the shadow.
	 */
	void (*set_rectangular_shadow)(struct wl_client *client,
				       struct wl_resource *resource,
				       int32_t x,
				       int32_t y,
				       int32_t width,
				       int32_t height);
	/**
	 * set_rectangular_shadow_background_opacity - suggests the
	 *	window's background opacity
	 * @opacity: (none)
	 *
	 * Suggests the window's background opacity when the shadow is
	 * requested.
	 */
	void (*set_rectangular_shadow_background_opacity)(struct wl_client *client,
							  struct wl_resource *resource,
							  wl_fixed_t opacity);
	/**
	 * set_title - set surface title
	 * @title: (none)
	 *
	 * Set a short title for the surface.
	 *
	 * This string may be used to identify the surface in a task bar,
	 * window list, or other user interface elements provided by the
	 * compositor.
	 *
	 * The string must be encoded in UTF-8.
	 */
	void (*set_title)(struct wl_client *client,
			  struct wl_resource *resource,
			  const char *title);
	/**
	 * set_top_inset - set top inset for surface
	 * @height: (none)
	 *
	 * Set distance from the top of the surface to the contents.
	 *
	 * This distance typically represents the size of the window
	 * caption.
	 */
	void (*set_top_inset)(struct wl_client *client,
			      struct wl_resource *resource,
			      int32_t height);
	/**
	 * activate - make the surface active
	 * @serial: the serial of the user event
	 *
	 * Make the surface active and bring it to the front.
	 */
	void (*activate)(struct wl_client *client,
			 struct wl_resource *resource,
			 uint32_t serial);
	/**
	 * maximize - maximize
	 *
	 * Request that surface is maximized. The window geometry will be
	 * updated to whatever the compositor finds appropriate for a
	 * maximized window.
	 *
	 * This is only a request that the window should be maximized. The
	 * compositor may choose to ignore this request. The client should
	 * listen to set_maximized events to determine if the window was
	 * maximized or not.
	 */
	void (*maximize)(struct wl_client *client,
			 struct wl_resource *resource);
	/**
	 * minimize - minimize
	 *
	 * Request that surface is minimized.
	 *
	 * This is only a request that the window should be minimized. The
	 * compositor may choose to ignore this request. The client should
	 * listen to set_minimized events to determine if the window was
	 * minimized or not.
	 */
	void (*minimize)(struct wl_client *client,
			 struct wl_resource *resource);
	/**
	 * restore - restore
	 *
	 * Request that surface is restored. This restores the window
	 * geometry to what it was before the window was minimized,
	 * maximized or made fullscreen.
	 *
	 * This is only a request that the window should be restored. The
	 * compositor may choose to ignore this request. The client should
	 * listen to unset_maximized, unset_minimize and unset_fullscreen
	 * events to determine if the window was restored or not.
	 */
	void (*restore)(struct wl_client *client,
			struct wl_resource *resource);
	/**
	 * fullscreen - fullscreen
	 *
	 * Request that surface is made fullscreen.
	 *
	 * This is only a request that the window should be made
	 * fullscreen. The compositor may choose to ignore this request.
	 * The client should listen to set_fullscreen events to determine
	 * if the window was made fullscreen or not.
	 */
	void (*fullscreen)(struct wl_client *client,
			   struct wl_resource *resource);
	/**
	 * unfullscreen - unfullscreen
	 *
	 * Request that surface is made unfullscreen.
	 *
	 * This is only a request that the window should be made
	 * unfullscreen. The compositor may choose to ignore this request.
	 * The client should listen to unset_fullscreen events to determine
	 * if the window was made unfullscreen or not.
	 */
	void (*unfullscreen)(struct wl_client *client,
			     struct wl_resource *resource);
	/**
	 * pin - pin
	 * @trusted: (none)
	 *
	 * Request that surface is pinned.
	 *
	 * This is only a request that the window should be pinned. The
	 * compositor may choose to ignore this request. The client should
	 * listen to state_changed events to determine if the window was
	 * pinned or not. If trusted flag is non-zero, the app can prevent
	 * users from exiting the pinned mode.
	 */
	void (*pin)(struct wl_client *client,
		    struct wl_resource *resource,
		    int32_t trusted);
	/**
	 * unpin - unpin
	 *
	 * Request that surface is unpinned.
	 *
	 * This is only a request that the window should be unpinned. The
	 * compositor may choose to ignore this request. The client should
	 * listen to unset_pinned events to determine if the window was
	 * unpinned or not.
	 */
	void (*unpin)(struct wl_client *client,
		      struct wl_resource *resource);
	/**
	 * set_system_modal - suggests a re-layout of remote shell input
	 *	area
	 *
	 * Suggests a surface should become system modal.
	 */
	void (*set_system_modal)(struct wl_client *client,
				 struct wl_resource *resource);
	/**
	 * unset_system_modal - suggests a re-layout of remote shell
	 *	input area
	 *
	 * Suggests a surface should become non system modal.
	 */
	void (*unset_system_modal)(struct wl_client *client,
				   struct wl_resource *resource);
	/**
	 * set_rectangular_surface_shadow - set a rectangular shadow
	 * @x: (none)
	 * @y: (none)
	 * @width: (none)
	 * @height: (none)
	 *
	 * Request that surface needs a rectangular shadow.
	 *
	 * This is only a request that the surface should have a
	 * rectangular shadow. The compositor may choose to ignore this
	 * request.
	 *
	 * The arguments are given in the remote surface coordinate space
	 * and specifies inner bounds of the shadow. Specifying zero width
	 * and height will disable the shadow.
	 * @since: 2
	 */
	void (*set_rectangular_surface_shadow)(struct wl_client *client,
					       struct wl_resource *resource,
					       int32_t x,
					       int32_t y,
					       int32_t width,
					       int32_t height);
	/**
	 * set_systemui_visibility - requests the system ui visibility
	 *	behavior for the surface
	 * @visibility: (none)
	 *
	 * Requests how the surface will change the visibility of the
	 * system UI when it is made active.
	 * @since: 3
	 */
	void (*set_systemui_visibility)(struct wl_client *client,
					struct wl_resource *resource,
					uint32_t visibility);
	/**
	 * set_always_on_top - set always on top
	 *
	 * Request that surface is made to be always on top.
	 *
	 * This is only a request that the window should be always on top.
	 * The compositor may choose to ignore this request.
	 * @since: 4
	 */
	void (*set_always_on_top)(struct wl_client *client,
				  struct wl_resource *resource);
	/**
	 * unset_always_on_top - unset always on top
	 *
	 * Request that surface is made to be not always on top.
	 *
	 * This is only a request that the window should be not always on
	 * top. The compositor may choose to ignore this request.
	 * @since: 4
	 */
	void (*unset_always_on_top)(struct wl_client *client,
				    struct wl_resource *resource);
	/**
	 * ack_configure - ack a configure event
	 * @serial: the serial from the configure event
	 *
	 * When a configure event is received, if a client commits the
	 * surface in response to the configure event, then the client must
	 * make an ack_configure request sometime before the commit
	 * request, passing along the serial of the configure event.
	 *
	 * For instance, the compositor might use this information during
	 * display configuration to change its coordinate space for
	 * set_window_geometry requests only when the client has switched
	 * to the new coordinate space.
	 *
	 * If the client receives multiple configure events before it can
	 * respond to one, it only has to ack the last configure event.
	 *
	 * A client is not required to commit immediately after sending an
	 * ack_configure request - it may even ack_configure several times
	 * before its next surface commit.
	 *
	 * A client may send multiple ack_configure requests before
	 * committing, but only the last request sent before a commit
	 * indicates which configure event the client really is responding
	 * to.
	 * @since: 5
	 */
	void (*ack_configure)(struct wl_client *client,
			      struct wl_resource *resource,
			      uint32_t serial);
	/**
	 * move - start an interactive move
	 *
	 * Start an interactive, user-driven move of the surface.
	 *
	 * The compositor responds to this request with a configure event
	 * that transitions to the "moving" state. The client must only
	 * initiate motion after acknowledging the state change. The
	 * compositor can assume that subsequent set_window_geometry
	 * requests are position updates until the next state transition is
	 * acknowledged.
	 *
	 * The compositor may ignore move requests depending on the state
	 * of the surface, e.g. fullscreen or maximized.
	 * @since: 5
	 */
	void (*move)(struct wl_client *client,
		     struct wl_resource *resource);
	/**
	 * set_orientation - set orientation
	 * @orientation: (none)
	 *
	 * Set an orientation for the surface.
	 * @since: 5
	 */
	void (*set_orientation)(struct wl_client *client,
				struct wl_resource *resource,
				int32_t orientation);
};

#define ZCR_REMOTE_SURFACE_V1_CLOSE	0
#define ZCR_REMOTE_SURFACE_V1_STATE_TYPE_CHANGED	1
#define ZCR_REMOTE_SURFACE_V1_CONFIGURE	2

static inline void
zcr_remote_surface_v1_send_close(struct wl_resource *resource_)
{
	wl_resource_post_event(resource_, ZCR_REMOTE_SURFACE_V1_CLOSE);
}

static inline void
zcr_remote_surface_v1_send_state_type_changed(struct wl_resource *resource_, uint32_t state_type)
{
	wl_resource_post_event(resource_, ZCR_REMOTE_SURFACE_V1_STATE_TYPE_CHANGED, state_type);
}

static inline void
zcr_remote_surface_v1_send_configure(struct wl_resource *resource_, int32_t origin_offset_x, int32_t origin_offset_y, struct wl_array *states, uint32_t serial)
{
	wl_resource_post_event(resource_, ZCR_REMOTE_SURFACE_V1_CONFIGURE, origin_offset_x, origin_offset_y, states, serial);
}

/**
 * zcr_notification_surface_v1 - A notification window
 * @destroy: Destroy the notification_surface
 *
 * An interface that may be implemented by a wl_surface to host
 * notification contents.
 */
struct zcr_notification_surface_v1_interface {
	/**
	 * destroy - Destroy the notification_surface
	 *
	 * Unmap and destroy the notification surface.
	 */
	void (*destroy)(struct wl_client *client,
			struct wl_resource *resource);
};

#ifdef  __cplusplus
}
#endif

#endif
