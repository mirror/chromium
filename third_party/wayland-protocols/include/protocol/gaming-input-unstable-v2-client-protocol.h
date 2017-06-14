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

#ifndef GAMING_INPUT_UNSTABLE_V2_CLIENT_PROTOCOL_H
#define GAMING_INPUT_UNSTABLE_V2_CLIENT_PROTOCOL_H

#ifdef  __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stddef.h>
#include "wayland-client.h"

struct wl_client;
struct wl_resource;

struct wl_seat;
struct zcr_gamepad_v2;
struct zcr_gaming_input_v2;
struct zcr_gaming_seat_v2;

extern const struct wl_interface zcr_gaming_input_v2_interface;
extern const struct wl_interface zcr_gaming_seat_v2_interface;
extern const struct wl_interface zcr_gamepad_v2_interface;

#define ZCR_GAMING_INPUT_V2_GET_GAMING_SEAT	0
#define ZCR_GAMING_INPUT_V2_DESTROY	1

static inline void
zcr_gaming_input_v2_set_user_data(struct zcr_gaming_input_v2 *zcr_gaming_input_v2, void *user_data)
{
	wl_proxy_set_user_data((struct wl_proxy *) zcr_gaming_input_v2, user_data);
}

static inline void *
zcr_gaming_input_v2_get_user_data(struct zcr_gaming_input_v2 *zcr_gaming_input_v2)
{
	return wl_proxy_get_user_data((struct wl_proxy *) zcr_gaming_input_v2);
}

static inline struct zcr_gaming_seat_v2 *
zcr_gaming_input_v2_get_gaming_seat(struct zcr_gaming_input_v2 *zcr_gaming_input_v2, struct wl_seat *seat)
{
	struct wl_proxy *gaming_seat;

	gaming_seat = wl_proxy_marshal_constructor((struct wl_proxy *) zcr_gaming_input_v2,
			 ZCR_GAMING_INPUT_V2_GET_GAMING_SEAT, &zcr_gaming_seat_v2_interface, NULL, seat);

	return (struct zcr_gaming_seat_v2 *) gaming_seat;
}

static inline void
zcr_gaming_input_v2_destroy(struct zcr_gaming_input_v2 *zcr_gaming_input_v2)
{
	wl_proxy_marshal((struct wl_proxy *) zcr_gaming_input_v2,
			 ZCR_GAMING_INPUT_V2_DESTROY);

	wl_proxy_destroy((struct wl_proxy *) zcr_gaming_input_v2);
}

/**
 * zcr_gaming_seat_v2 - controller object for all gaming devices of a
 *	seat
 * @gamepad_added: gamepad added event
 *
 * An object that provides access to all the gaming devices of a seat.
 * When a gamepad is connected, the compositor will send gamepad_added
 * event.
 */
struct zcr_gaming_seat_v2_listener {
	/**
	 * gamepad_added - gamepad added event
	 * @gamepad: new connected gamepad
	 *
	 * Notification that there is gamepad connected at this seat.
	 */
	void (*gamepad_added)(void *data,
			      struct zcr_gaming_seat_v2 *zcr_gaming_seat_v2,
			      struct zcr_gamepad_v2 *gamepad);
};

static inline int
zcr_gaming_seat_v2_add_listener(struct zcr_gaming_seat_v2 *zcr_gaming_seat_v2,
				const struct zcr_gaming_seat_v2_listener *listener, void *data)
{
	return wl_proxy_add_listener((struct wl_proxy *) zcr_gaming_seat_v2,
				     (void (**)(void)) listener, data);
}

#define ZCR_GAMING_SEAT_V2_DESTROY	0

static inline void
zcr_gaming_seat_v2_set_user_data(struct zcr_gaming_seat_v2 *zcr_gaming_seat_v2, void *user_data)
{
	wl_proxy_set_user_data((struct wl_proxy *) zcr_gaming_seat_v2, user_data);
}

static inline void *
zcr_gaming_seat_v2_get_user_data(struct zcr_gaming_seat_v2 *zcr_gaming_seat_v2)
{
	return wl_proxy_get_user_data((struct wl_proxy *) zcr_gaming_seat_v2);
}

static inline void
zcr_gaming_seat_v2_destroy(struct zcr_gaming_seat_v2 *zcr_gaming_seat_v2)
{
	wl_proxy_marshal((struct wl_proxy *) zcr_gaming_seat_v2,
			 ZCR_GAMING_SEAT_V2_DESTROY);

	wl_proxy_destroy((struct wl_proxy *) zcr_gaming_seat_v2);
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
 * @removed: gamepad removed
 * @axis: axis change event
 * @button: Gamepad button changed
 * @frame: Notifies end of a series of gamepad changes.
 *
 * The zcr_gamepad_v2 interface represents one or more gamepad input
 * devices, which are reported as a normalized 'Standard Gamepad' as it is
 * specified by the W3C Gamepad API at:
 * https://w3c.github.io/gamepad/#remapping
 */
struct zcr_gamepad_v2_listener {
	/**
	 * removed - gamepad removed
	 *
	 * Removed event is send when the gamepad is disconnected. The
	 * client should expect no more event and call destroy.
	 *
	 * This event cannot be used as destructor as requests (e.g.
	 * vibration) might be added to this interface.
	 */
	void (*removed)(void *data,
			struct zcr_gamepad_v2 *zcr_gamepad_v2);
	/**
	 * axis - axis change event
	 * @time: timestamp with millisecond granularity
	 * @axis: axis that produced this event
	 * @value: new value of axis
	 *
	 * Notification of axis change.
	 *
	 * The axis id specifies which axis has changed as defined by the
	 * W3C 'Standard Gamepad'.
	 *
	 * The value is calibrated and normalized to the -1 to 1 range.
	 */
	void (*axis)(void *data,
		     struct zcr_gamepad_v2 *zcr_gamepad_v2,
		     uint32_t time,
		     uint32_t axis,
		     wl_fixed_t value);
	/**
	 * button - Gamepad button changed
	 * @time: timestamp with millisecond granularity
	 * @button: id of button
	 * @state: digital state of the button
	 * @analog: analog value of the button
	 *
	 * Notification of button change.
	 *
	 * The button id specifies which button has changed as defined by
	 * the W3C 'Standard Gamepad'.
	 *
	 * A button can have a digital and an analog value. The analog
	 * value is normalized to a 0 to 1 range. If a button does not
	 * provide an analog value, it will be derived from the digital
	 * state.
	 */
	void (*button)(void *data,
		       struct zcr_gamepad_v2 *zcr_gamepad_v2,
		       uint32_t time,
		       uint32_t button,
		       uint32_t state,
		       wl_fixed_t analog);
	/**
	 * frame - Notifies end of a series of gamepad changes.
	 * @time: timestamp with millisecond granularity
	 *
	 * Indicates the end of a set of events that logically belong
	 * together. A client is expected to accumulate the data in all
	 * events within the frame before proceeding.
	 */
	void (*frame)(void *data,
		      struct zcr_gamepad_v2 *zcr_gamepad_v2,
		      uint32_t time);
};

static inline int
zcr_gamepad_v2_add_listener(struct zcr_gamepad_v2 *zcr_gamepad_v2,
			    const struct zcr_gamepad_v2_listener *listener, void *data)
{
	return wl_proxy_add_listener((struct wl_proxy *) zcr_gamepad_v2,
				     (void (**)(void)) listener, data);
}

#define ZCR_GAMEPAD_V2_DESTROY	0

static inline void
zcr_gamepad_v2_set_user_data(struct zcr_gamepad_v2 *zcr_gamepad_v2, void *user_data)
{
	wl_proxy_set_user_data((struct wl_proxy *) zcr_gamepad_v2, user_data);
}

static inline void *
zcr_gamepad_v2_get_user_data(struct zcr_gamepad_v2 *zcr_gamepad_v2)
{
	return wl_proxy_get_user_data((struct wl_proxy *) zcr_gamepad_v2);
}

static inline void
zcr_gamepad_v2_destroy(struct zcr_gamepad_v2 *zcr_gamepad_v2)
{
	wl_proxy_marshal((struct wl_proxy *) zcr_gamepad_v2,
			 ZCR_GAMEPAD_V2_DESTROY);

	wl_proxy_destroy((struct wl_proxy *) zcr_gamepad_v2);
}

#ifdef  __cplusplus
}
#endif

#endif
