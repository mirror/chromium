// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.device.gamepad;

import android.view.KeyEvent;
import android.view.MotionEvent;

/**
 * Abstracts away the GamepadDevice class in order to allow the creation of mock Gamepads
 * during testing.
 */
interface GamepadDevice {
    /**
     * Updates the axes and buttons maping of a gamepad device to a standard gamepad format.
     */
    public void updateButtonsAndAxesMapping();

    /**
     * @return Device Id of the gamepad device.
     */
    public int getId();

    /**
     * @return Device name of the gamepad device.
     */
    public String getName();

    /**
     * @return Device index of the gamepad device.
     */
    public int getIndex();

    /**
     * @return The timestamp when the gamepad device was last interacted.
     */
    public long getTimestamp();

    /**
     * @return The axes state of the gamepad device.
     */
    public float[] getAxes();

    /**
     * @return The buttons state of the gamepad device.
     */
    public float[] getButtons();

    /**
     * Reset the axes and buttons data of the gamepad device everytime gamepad data access is
     * paused.
     */
    public void clearData();

    /**
     * @return Mapping status of the gamepad device.
     */
    public boolean isStandardGamepad();

    /**
     * Handles key event from the gamepad device.
     * @return True if the key event from the gamepad device has been consumed.
     */
    public boolean handleKeyEvent(KeyEvent event);

    /**
     * Handles motion event from the gamepad device.
     * @return True if the motion event from the gamepad device has been consumed.
     */
    public boolean handleMotionEvent(MotionEvent event);
}