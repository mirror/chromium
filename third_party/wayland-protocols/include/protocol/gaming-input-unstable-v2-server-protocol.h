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

#ifndef GAMING_INPUT_UNSTABLE_V2_SERVER_PROTOCOL_H
#define GAMING_INPUT_UNSTABLE_V2_SERVER_PROTOCOL_H

#ifdef  __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stddef.h>
#include "wayland-server.h"

struct wl_client;
struct wl_resource;

struct wl_seat;
struct zcr_gamepad_v2;
struct zcr_gaming_input_v2;
struct zcr_gaming_seat_v2;

extern const struct wl_interface zcr_gaming_input_v2_interface;
extern const struct wl_interface zcr_gaming_seat_v2_interface;
extern const struct wl_interface zcr_gamepad_v2_interface;

/**
 * zcr_gaming_input_v2 - extends wl_seat with gaming input devices
 * @get_gaming_seat: get a gaming seat
 * @destroy: release the memory for the gaming input object
 *
 * A global interface to provide gaming input devices for a given seat.
 *
 * Currently only gamepad devices are supported.
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
struct zcr_gaming_input_v2_interface {
	/**
	 * get_gaming_seat - get a gaming seat
	 * @gaming_seat: (none)
	 * @seat: (none)
	 *
	 * Get a gaming seat object for a given seat. Gaming seat
	 * provides access to gaming devices
	 */
	void (*get_gaming_seat)(struct wl_client *client,
				struct wl_resource *resource,
				uint32_t gaming_seat,
				struct wl_resource *seat);
	/**
	 * destroy - release the memory for the gaming input object
	 *
	 * Destroy gaming_input object. Objects created from this object
	 * are unaffected and should be destroyed separately.
	 */
	void (*destroy)(struct wl_client *client,
			struct wl_resource *resource);
};


/**
 * zcr_gaming_seat_v2 - controller object for all gaming devices of a
 *	seat
 * @destroy: release the memory for the gaming seat object
 *
 * An object that provides access to all the gaming devices of a seat.
 * When a gamepad is connected, the compositor will send gamepad_added
 * event.
 */
struct zcr_gaming_seat_v2_interface {
	/**
	 * destroy - release the memory for the gaming seat object
	 *
	 * Destroy gaming_seat object. Objects created from this object
	 * are unaffected and should be destroyed separately.
	 */
	void (*destroy)(struct wl_client *client,
			struct wl_resource *resource);
};

#define ZCR_GAMING_SEAT_V2_GAMEPAD_ADDED	0

#define ZCR_GAMING_SEAT_V2_GAMEPAD_ADDED_SINCE_VERSION	1

static inline void
zcr_gaming_seat_v2_send_gamepad_added(struct wl_resource *resource_, struct wl_resource *gamepad)
{
	wl_resource_post_event(resource_, ZCR_GAMING_SEAT_V2_GAMEPAD_ADDED, gamepad);
}

#ifndef ZCR_GAMEPAD_V2_BUTTON_STATE_ENUM
#define ZCR_GAMEPAD_V2_BUTTON_STATE_ENUM
/**
 * zcr_gamepad_v2_button_state - physical button state
 * @ZCR_GAMEPAD_V2_BUTTON_STATE_RELEASED: the button is not pressed
 * @ZCR_GAMEPAD_V2_BUTTON_STATE_PRESSED: the button is pressed
 *
 * Describes the physical state of a button that produced the button
 * event.
 */
enum zcr_gamepad_v2_button_state {
	ZCR_GAMEPAD_V2_BUTTON_STATE_RELEASED = 0,
	ZCR_GAMEPAD_V2_BUTTON_STATE_PRESSED = 1,
};
#endif /* ZCR_GAMEPAD_V2_BUTTON_STATE_ENUM */

/**
 * zcr_gamepad_v2 - gamepad input device
 * @destroy: destroy gamepad object
 *
 * The zcr_gamepad_v2 interface represents one or more gamepad input
 * devices, which are reported as a normalized 'Standard Gamepad' as it is
 * specified by the W3C Gamepad API at:
 * https://w3c.github.io/gamepad/#remapping
 */
struct zcr_gamepad_v2_interface {
	/**
	 * destroy - destroy gamepad object
	 *
	 * 
	 */
	void (*destroy)(struct wl_client *client,
			struct wl_resource *resource);
};

#define ZCR_GAMEPAD_V2_REMOVED	0
#define ZCR_GAMEPAD_V2_AXIS	1
#define ZCR_GAMEPAD_V2_BUTTON	2
#define ZCR_GAMEPAD_V2_FRAME	3

#define ZCR_GAMEPAD_V2_REMOVED_SINCE_VERSION	1
#define ZCR_GAMEPAD_V2_AXIS_SINCE_VERSION	1
#define ZCR_GAMEPAD_V2_BUTTON_SINCE_VERSION	1
#define ZCR_GAMEPAD_V2_FRAME_SINCE_VERSION	1

static inline void
zcr_gamepad_v2_send_removed(struct wl_resource *resource_)
{
	wl_resource_post_event(resource_, ZCR_GAMEPAD_V2_REMOVED);
}

static inline void
zcr_gamepad_v2_send_axis(struct wl_resource *resource_, uint32_t time, uint32_t axis, wl_fixed_t value)
{
	wl_resource_post_event(resource_, ZCR_GAMEPAD_V2_AXIS, time, axis, value);
}

static inline void
zcr_gamepad_v2_send_button(struct wl_resource *resource_, uint32_t time, uint32_t button, uint32_t state, wl_fixed_t analog)
{
	wl_resource_post_event(resource_, ZCR_GAMEPAD_V2_BUTTON, time, button, state, analog);
}

static inline void
zcr_gamepad_v2_send_frame(struct wl_resource *resource_, uint32_t time)
{
	wl_resource_post_event(resource_, ZCR_GAMEPAD_V2_FRAME, time);
}

#ifdef  __cplusplus
}
#endif

#endif
