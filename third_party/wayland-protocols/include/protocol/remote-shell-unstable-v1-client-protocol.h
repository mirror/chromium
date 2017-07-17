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

#ifndef REMOTE_SHELL_UNSTABLE_V1_CLIENT_PROTOCOL_H
#define REMOTE_SHELL_UNSTABLE_V1_CLIENT_PROTOCOL_H

#ifdef  __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stddef.h>
#include "wayland-client.h"

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
 * @activated: activated surface changed
 * @configuration_changed: suggests a re-configuration of remote shell
 * @workspace: area of remote shell
 * @configure: suggests configuration of remote shell
 *
 * The global interface that allows clients to turn a wl_surface into a
 * "real window" which is remotely managed but can be stacked, activated
 * and made fullscreen by the user.
 */
struct zcr_remote_shell_v1_listener {
	/**
	 * activated - activated surface changed
	 * @gained_active: (none)
	 * @lost_active: (none)
	 *
	 * Notifies client that the activated surface changed.
	 */
	void (*activated)(void *data,
			  struct zcr_remote_shell_v1 *zcr_remote_shell_v1,
			  struct wl_surface *gained_active,
			  struct wl_surface *lost_active);
	/**
	 * configuration_changed - suggests a re-configuration of remote
	 *	shell
	 * @width: (none)
	 * @height: (none)
	 * @transform: (none)
	 * @scale_factor: (none)
	 * @work_area_inset_left: (none)
	 * @work_area_inset_top: (none)
	 * @work_area_inset_right: (none)
	 * @work_area_inset_bottom: (none)
	 * @layout_mode: (none)
	 *
	 * [Deprecated] Suggests a re-configuration of remote shell.
	 */
	void (*configuration_changed)(void *data,
				      struct zcr_remote_shell_v1 *zcr_remote_shell_v1,
				      int32_t width,
				      int32_t height,
				      int32_t transform,
				      wl_fixed_t scale_factor,
				      int32_t work_area_inset_left,
				      int32_t work_area_inset_top,
				      int32_t work_area_inset_right,
				      int32_t work_area_inset_bottom,
				      uint32_t layout_mode);
	/**
	 * workspace - area of remote shell
	 * @id_hi: (none)
	 * @id_lo: (none)
	 * @x: (none)
	 * @y: (none)
	 * @width: (none)
	 * @height: (none)
	 * @inset_left: (none)
	 * @inset_top: (none)
	 * @inset_right: (none)
	 * @inset_bottom: (none)
	 * @transform: (none)
	 * @scale_factor: (none)
	 * @is_internal: 1 if screen is built-in
	 *
	 * Defines an area of the remote shell used for layout. Each
	 * series of "workspace" events must be terminated by a "configure"
	 * event.
	 * @since: 5
	 */
	void (*workspace)(void *data,
			  struct zcr_remote_shell_v1 *zcr_remote_shell_v1,
			  uint32_t id_hi,
			  uint32_t id_lo,
			  int32_t x,
			  int32_t y,
			  int32_t width,
			  int32_t height,
			  int32_t inset_left,
			  int32_t inset_top,
			  int32_t inset_right,
			  int32_t inset_bottom,
			  int32_t transform,
			  wl_fixed_t scale_factor,
			  uint32_t is_internal);
	/**
	 * configure - suggests configuration of remote shell
	 * @layout_mode: (none)
	 *
	 * Suggests a new configuration of the remote shell. Preceded by
	 * a series of "workspace" events.
	 * @since: 5
	 */
	void (*configure)(void *data,
			  struct zcr_remote_shell_v1 *zcr_remote_shell_v1,
			  uint32_t layout_mode);
};

static inline int
zcr_remote_shell_v1_add_listener(struct zcr_remote_shell_v1 *zcr_remote_shell_v1,
				 const struct zcr_remote_shell_v1_listener *listener, void *data)
{
	return wl_proxy_add_listener((struct wl_proxy *) zcr_remote_shell_v1,
				     (void (**)(void)) listener, data);
}

#define ZCR_REMOTE_SHELL_V1_DESTROY	0
#define ZCR_REMOTE_SHELL_V1_GET_REMOTE_SURFACE	1
#define ZCR_REMOTE_SHELL_V1_GET_NOTIFICATION_SURFACE	2

static inline void
zcr_remote_shell_v1_set_user_data(struct zcr_remote_shell_v1 *zcr_remote_shell_v1, void *user_data)
{
	wl_proxy_set_user_data((struct wl_proxy *) zcr_remote_shell_v1, user_data);
}

static inline void *
zcr_remote_shell_v1_get_user_data(struct zcr_remote_shell_v1 *zcr_remote_shell_v1)
{
	return wl_proxy_get_user_data((struct wl_proxy *) zcr_remote_shell_v1);
}

static inline void
zcr_remote_shell_v1_destroy(struct zcr_remote_shell_v1 *zcr_remote_shell_v1)
{
	wl_proxy_marshal((struct wl_proxy *) zcr_remote_shell_v1,
			 ZCR_REMOTE_SHELL_V1_DESTROY);

	wl_proxy_destroy((struct wl_proxy *) zcr_remote_shell_v1);
}

static inline struct zcr_remote_surface_v1 *
zcr_remote_shell_v1_get_remote_surface(struct zcr_remote_shell_v1 *zcr_remote_shell_v1, struct wl_surface *surface, uint32_t container)
{
	struct wl_proxy *id;

	id = wl_proxy_marshal_constructor((struct wl_proxy *) zcr_remote_shell_v1,
			 ZCR_REMOTE_SHELL_V1_GET_REMOTE_SURFACE, &zcr_remote_surface_v1_interface, NULL, surface, container);

	return (struct zcr_remote_surface_v1 *) id;
}

static inline struct zcr_notification_surface_v1 *
zcr_remote_shell_v1_get_notification_surface(struct zcr_remote_shell_v1 *zcr_remote_shell_v1, struct wl_surface *surface, const char *notification_id)
{
	struct wl_proxy *id;

	id = wl_proxy_marshal_constructor((struct wl_proxy *) zcr_remote_shell_v1,
			 ZCR_REMOTE_SHELL_V1_GET_NOTIFICATION_SURFACE, &zcr_notification_surface_v1_interface, NULL, surface, notification_id);

	return (struct zcr_notification_surface_v1 *) id;
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
 * @close: surface wants to be closed
 * @state_type_changed: surface state type changed
 * @configure: suggest a surface change
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
struct zcr_remote_surface_v1_listener {
	/**
	 * close - surface wants to be closed
	 *
	 * The close event is sent by the compositor when the user wants
	 * the surface to be closed. This should be equivalent to the user
	 * clicking the close button in client-side decorations, if your
	 * application has any...
	 *
	 * This is only a request that the user intends to close your
	 * window. The client may choose to ignore this request, or show a
	 * dialog to ask the user to save their data...
	 */
	void (*close)(void *data,
		      struct zcr_remote_surface_v1 *zcr_remote_surface_v1);
	/**
	 * state_type_changed - surface state type changed
	 * @state_type: (none)
	 *
	 * [Deprecated] The state_type_changed event is sent by the
	 * compositor when the surface state changed.
	 *
	 * This is an event to notify that the window state changed in
	 * compositor. The state change may be triggered by a client's
	 * request, or some user action directly handled by the compositor.
	 * The client may choose to ignore this event.
	 */
	void (*state_type_changed)(void *data,
				   struct zcr_remote_surface_v1 *zcr_remote_surface_v1,
				   uint32_t state_type);
	/**
	 * configure - suggest a surface change
	 * @origin_offset_x: (none)
	 * @origin_offset_y: (none)
	 * @states: (none)
	 * @serial: (none)
	 *
	 * The configure event asks the client to change surface state.
	 *
	 * The client must apply the origin offset to window positions in
	 * set_window_geometry requests.
	 *
	 * The states listed in the event are state_type values, and might
	 * change due to a client request or an event directly handled by
	 * the compositor.
	 *
	 * Clients should arrange their surface for the new state, and then
	 * send an ack_configure request with the serial sent in this
	 * configure event at some point before committing the new surface.
	 *
	 * If the client receives multiple configure events before it can
	 * respond to one, it is free to discard all but the last event it
	 * received.
	 * @since: 5
	 */
	void (*configure)(void *data,
			  struct zcr_remote_surface_v1 *zcr_remote_surface_v1,
			  int32_t origin_offset_x,
			  int32_t origin_offset_y,
			  struct wl_array *states,
			  uint32_t serial);
};

static inline int
zcr_remote_surface_v1_add_listener(struct zcr_remote_surface_v1 *zcr_remote_surface_v1,
				   const struct zcr_remote_surface_v1_listener *listener, void *data)
{
	return wl_proxy_add_listener((struct wl_proxy *) zcr_remote_surface_v1,
				     (void (**)(void)) listener, data);
}

#define ZCR_REMOTE_SURFACE_V1_DESTROY	0
#define ZCR_REMOTE_SURFACE_V1_SET_APP_ID	1
#define ZCR_REMOTE_SURFACE_V1_SET_WINDOW_GEOMETRY	2
#define ZCR_REMOTE_SURFACE_V1_SET_SCALE	3
#define ZCR_REMOTE_SURFACE_V1_SET_RECTANGULAR_SHADOW	4
#define ZCR_REMOTE_SURFACE_V1_SET_RECTANGULAR_SHADOW_BACKGROUND_OPACITY	5
#define ZCR_REMOTE_SURFACE_V1_SET_TITLE	6
#define ZCR_REMOTE_SURFACE_V1_SET_TOP_INSET	7
#define ZCR_REMOTE_SURFACE_V1_ACTIVATE	8
#define ZCR_REMOTE_SURFACE_V1_MAXIMIZE	9
#define ZCR_REMOTE_SURFACE_V1_MINIMIZE	10
#define ZCR_REMOTE_SURFACE_V1_RESTORE	11
#define ZCR_REMOTE_SURFACE_V1_FULLSCREEN	12
#define ZCR_REMOTE_SURFACE_V1_UNFULLSCREEN	13
#define ZCR_REMOTE_SURFACE_V1_PIN	14
#define ZCR_REMOTE_SURFACE_V1_UNPIN	15
#define ZCR_REMOTE_SURFACE_V1_SET_SYSTEM_MODAL	16
#define ZCR_REMOTE_SURFACE_V1_UNSET_SYSTEM_MODAL	17
#define ZCR_REMOTE_SURFACE_V1_SET_RECTANGULAR_SURFACE_SHADOW	18
#define ZCR_REMOTE_SURFACE_V1_SET_SYSTEMUI_VISIBILITY	19
#define ZCR_REMOTE_SURFACE_V1_SET_ALWAYS_ON_TOP	20
#define ZCR_REMOTE_SURFACE_V1_UNSET_ALWAYS_ON_TOP	21
#define ZCR_REMOTE_SURFACE_V1_ACK_CONFIGURE	22
#define ZCR_REMOTE_SURFACE_V1_MOVE	23
#define ZCR_REMOTE_SURFACE_V1_SET_ORIENTATION	24

static inline void
zcr_remote_surface_v1_set_user_data(struct zcr_remote_surface_v1 *zcr_remote_surface_v1, void *user_data)
{
	wl_proxy_set_user_data((struct wl_proxy *) zcr_remote_surface_v1, user_data);
}

static inline void *
zcr_remote_surface_v1_get_user_data(struct zcr_remote_surface_v1 *zcr_remote_surface_v1)
{
	return wl_proxy_get_user_data((struct wl_proxy *) zcr_remote_surface_v1);
}

static inline void
zcr_remote_surface_v1_destroy(struct zcr_remote_surface_v1 *zcr_remote_surface_v1)
{
	wl_proxy_marshal((struct wl_proxy *) zcr_remote_surface_v1,
			 ZCR_REMOTE_SURFACE_V1_DESTROY);

	wl_proxy_destroy((struct wl_proxy *) zcr_remote_surface_v1);
}

static inline void
zcr_remote_surface_v1_set_app_id(struct zcr_remote_surface_v1 *zcr_remote_surface_v1, const char *app_id)
{
	wl_proxy_marshal((struct wl_proxy *) zcr_remote_surface_v1,
			 ZCR_REMOTE_SURFACE_V1_SET_APP_ID, app_id);
}

static inline void
zcr_remote_surface_v1_set_window_geometry(struct zcr_remote_surface_v1 *zcr_remote_surface_v1, int32_t x, int32_t y, int32_t width, int32_t height)
{
	wl_proxy_marshal((struct wl_proxy *) zcr_remote_surface_v1,
			 ZCR_REMOTE_SURFACE_V1_SET_WINDOW_GEOMETRY, x, y, width, height);
}

static inline void
zcr_remote_surface_v1_set_scale(struct zcr_remote_surface_v1 *zcr_remote_surface_v1, wl_fixed_t scale)
{
	wl_proxy_marshal((struct wl_proxy *) zcr_remote_surface_v1,
			 ZCR_REMOTE_SURFACE_V1_SET_SCALE, scale);
}

static inline void
zcr_remote_surface_v1_set_rectangular_shadow(struct zcr_remote_surface_v1 *zcr_remote_surface_v1, int32_t x, int32_t y, int32_t width, int32_t height)
{
	wl_proxy_marshal((struct wl_proxy *) zcr_remote_surface_v1,
			 ZCR_REMOTE_SURFACE_V1_SET_RECTANGULAR_SHADOW, x, y, width, height);
}

static inline void
zcr_remote_surface_v1_set_rectangular_shadow_background_opacity(struct zcr_remote_surface_v1 *zcr_remote_surface_v1, wl_fixed_t opacity)
{
	wl_proxy_marshal((struct wl_proxy *) zcr_remote_surface_v1,
			 ZCR_REMOTE_SURFACE_V1_SET_RECTANGULAR_SHADOW_BACKGROUND_OPACITY, opacity);
}

static inline void
zcr_remote_surface_v1_set_title(struct zcr_remote_surface_v1 *zcr_remote_surface_v1, const char *title)
{
	wl_proxy_marshal((struct wl_proxy *) zcr_remote_surface_v1,
			 ZCR_REMOTE_SURFACE_V1_SET_TITLE, title);
}

static inline void
zcr_remote_surface_v1_set_top_inset(struct zcr_remote_surface_v1 *zcr_remote_surface_v1, int32_t height)
{
	wl_proxy_marshal((struct wl_proxy *) zcr_remote_surface_v1,
			 ZCR_REMOTE_SURFACE_V1_SET_TOP_INSET, height);
}

static inline void
zcr_remote_surface_v1_activate(struct zcr_remote_surface_v1 *zcr_remote_surface_v1, uint32_t serial)
{
	wl_proxy_marshal((struct wl_proxy *) zcr_remote_surface_v1,
			 ZCR_REMOTE_SURFACE_V1_ACTIVATE, serial);
}

static inline void
zcr_remote_surface_v1_maximize(struct zcr_remote_surface_v1 *zcr_remote_surface_v1)
{
	wl_proxy_marshal((struct wl_proxy *) zcr_remote_surface_v1,
			 ZCR_REMOTE_SURFACE_V1_MAXIMIZE);
}

static inline void
zcr_remote_surface_v1_minimize(struct zcr_remote_surface_v1 *zcr_remote_surface_v1)
{
	wl_proxy_marshal((struct wl_proxy *) zcr_remote_surface_v1,
			 ZCR_REMOTE_SURFACE_V1_MINIMIZE);
}

static inline void
zcr_remote_surface_v1_restore(struct zcr_remote_surface_v1 *zcr_remote_surface_v1)
{
	wl_proxy_marshal((struct wl_proxy *) zcr_remote_surface_v1,
			 ZCR_REMOTE_SURFACE_V1_RESTORE);
}

static inline void
zcr_remote_surface_v1_fullscreen(struct zcr_remote_surface_v1 *zcr_remote_surface_v1)
{
	wl_proxy_marshal((struct wl_proxy *) zcr_remote_surface_v1,
			 ZCR_REMOTE_SURFACE_V1_FULLSCREEN);
}

static inline void
zcr_remote_surface_v1_unfullscreen(struct zcr_remote_surface_v1 *zcr_remote_surface_v1)
{
	wl_proxy_marshal((struct wl_proxy *) zcr_remote_surface_v1,
			 ZCR_REMOTE_SURFACE_V1_UNFULLSCREEN);
}

static inline void
zcr_remote_surface_v1_pin(struct zcr_remote_surface_v1 *zcr_remote_surface_v1, int32_t trusted)
{
	wl_proxy_marshal((struct wl_proxy *) zcr_remote_surface_v1,
			 ZCR_REMOTE_SURFACE_V1_PIN, trusted);
}

static inline void
zcr_remote_surface_v1_unpin(struct zcr_remote_surface_v1 *zcr_remote_surface_v1)
{
	wl_proxy_marshal((struct wl_proxy *) zcr_remote_surface_v1,
			 ZCR_REMOTE_SURFACE_V1_UNPIN);
}

static inline void
zcr_remote_surface_v1_set_system_modal(struct zcr_remote_surface_v1 *zcr_remote_surface_v1)
{
	wl_proxy_marshal((struct wl_proxy *) zcr_remote_surface_v1,
			 ZCR_REMOTE_SURFACE_V1_SET_SYSTEM_MODAL);
}

static inline void
zcr_remote_surface_v1_unset_system_modal(struct zcr_remote_surface_v1 *zcr_remote_surface_v1)
{
	wl_proxy_marshal((struct wl_proxy *) zcr_remote_surface_v1,
			 ZCR_REMOTE_SURFACE_V1_UNSET_SYSTEM_MODAL);
}

static inline void
zcr_remote_surface_v1_set_rectangular_surface_shadow(struct zcr_remote_surface_v1 *zcr_remote_surface_v1, int32_t x, int32_t y, int32_t width, int32_t height)
{
	wl_proxy_marshal((struct wl_proxy *) zcr_remote_surface_v1,
			 ZCR_REMOTE_SURFACE_V1_SET_RECTANGULAR_SURFACE_SHADOW, x, y, width, height);
}

static inline void
zcr_remote_surface_v1_set_systemui_visibility(struct zcr_remote_surface_v1 *zcr_remote_surface_v1, uint32_t visibility)
{
	wl_proxy_marshal((struct wl_proxy *) zcr_remote_surface_v1,
			 ZCR_REMOTE_SURFACE_V1_SET_SYSTEMUI_VISIBILITY, visibility);
}

static inline void
zcr_remote_surface_v1_set_always_on_top(struct zcr_remote_surface_v1 *zcr_remote_surface_v1)
{
	wl_proxy_marshal((struct wl_proxy *) zcr_remote_surface_v1,
			 ZCR_REMOTE_SURFACE_V1_SET_ALWAYS_ON_TOP);
}

static inline void
zcr_remote_surface_v1_unset_always_on_top(struct zcr_remote_surface_v1 *zcr_remote_surface_v1)
{
	wl_proxy_marshal((struct wl_proxy *) zcr_remote_surface_v1,
			 ZCR_REMOTE_SURFACE_V1_UNSET_ALWAYS_ON_TOP);
}

static inline void
zcr_remote_surface_v1_ack_configure(struct zcr_remote_surface_v1 *zcr_remote_surface_v1, uint32_t serial)
{
	wl_proxy_marshal((struct wl_proxy *) zcr_remote_surface_v1,
			 ZCR_REMOTE_SURFACE_V1_ACK_CONFIGURE, serial);
}

static inline void
zcr_remote_surface_v1_move(struct zcr_remote_surface_v1 *zcr_remote_surface_v1)
{
	wl_proxy_marshal((struct wl_proxy *) zcr_remote_surface_v1,
			 ZCR_REMOTE_SURFACE_V1_MOVE);
}

static inline void
zcr_remote_surface_v1_set_orientation(struct zcr_remote_surface_v1 *zcr_remote_surface_v1, int32_t orientation)
{
	wl_proxy_marshal((struct wl_proxy *) zcr_remote_surface_v1,
			 ZCR_REMOTE_SURFACE_V1_SET_ORIENTATION, orientation);
}

#define ZCR_NOTIFICATION_SURFACE_V1_DESTROY	0

static inline void
zcr_notification_surface_v1_set_user_data(struct zcr_notification_surface_v1 *zcr_notification_surface_v1, void *user_data)
{
	wl_proxy_set_user_data((struct wl_proxy *) zcr_notification_surface_v1, user_data);
}

static inline void *
zcr_notification_surface_v1_get_user_data(struct zcr_notification_surface_v1 *zcr_notification_surface_v1)
{
	return wl_proxy_get_user_data((struct wl_proxy *) zcr_notification_surface_v1);
}

static inline void
zcr_notification_surface_v1_destroy(struct zcr_notification_surface_v1 *zcr_notification_surface_v1)
{
	wl_proxy_marshal((struct wl_proxy *) zcr_notification_surface_v1,
			 ZCR_NOTIFICATION_SURFACE_V1_DESTROY);

	wl_proxy_destroy((struct wl_proxy *) zcr_notification_surface_v1);
}

#ifdef  __cplusplus
}
#endif

#endif
