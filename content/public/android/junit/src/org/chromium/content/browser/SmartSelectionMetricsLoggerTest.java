// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.content.browser;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;

import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.MockitoAnnotations;
import org.robolectric.annotation.Config;

import org.chromium.base.test.util.Feature;
import org.chromium.testing.local.LocalRobolectricTestRunner;

/**
 * Unit tests for the {@link SmartSelectionMetricsLogger}.
 */
@RunWith(LocalRobolectricTestRunner.class)
@Config(manifest = Config.NONE)
public class SmartSelectionMetricsLoggerTest {
    @Before
    public void setUp() throws Exception {
        MockitoAnnotations.initMocks(this);
    }

    @Test
    @Feature({"TextInput", "SmartSelection"})
    public void testOverlap() {
        assertTrue(SmartSelectionMetricsLogger.overlap(1, 3, 2, 4));
        assertTrue(SmartSelectionMetricsLogger.overlap(2, 4, 1, 3));

        assertTrue(SmartSelectionMetricsLogger.overlap(1, 4, 2, 3));
        assertTrue(SmartSelectionMetricsLogger.overlap(2, 3, 1, 4));

        assertTrue(SmartSelectionMetricsLogger.overlap(1, 4, 1, 3));
        assertTrue(SmartSelectionMetricsLogger.overlap(1, 3, 1, 4));

        assertTrue(SmartSelectionMetricsLogger.overlap(1, 4, 2, 4));
        assertTrue(SmartSelectionMetricsLogger.overlap(2, 4, 1, 4));

        assertTrue(SmartSelectionMetricsLogger.overlap(1, 4, 1, 4));

        assertFalse(SmartSelectionMetricsLogger.overlap(1, 2, 3, 4));
        assertFalse(SmartSelectionMetricsLogger.overlap(3, 4, 1, 2));

        assertFalse(SmartSelectionMetricsLogger.overlap(1, 2, 2, 4));
        assertFalse(SmartSelectionMetricsLogger.overlap(2, 4, 1, 2));
    }

    @Test
    @Feature({"TextInput", "SmartSelection"})
    public void testContains() {
        assertFalse(SmartSelectionMetricsLogger.contains(1, 3, 2, 4));
        assertFalse(SmartSelectionMetricsLogger.contains(2, 4, 1, 3));

        assertTrue(SmartSelectionMetricsLogger.contains(1, 4, 2, 3));
        assertFalse(SmartSelectionMetricsLogger.contains(2, 3, 1, 4));

        assertTrue(SmartSelectionMetricsLogger.contains(1, 4, 1, 3));
        assertFalse(SmartSelectionMetricsLogger.contains(1, 3, 1, 4));

        assertTrue(SmartSelectionMetricsLogger.contains(1, 4, 2, 4));
        assertFalse(SmartSelectionMetricsLogger.contains(2, 4, 1, 4));

        assertTrue(SmartSelectionMetricsLogger.contains(1, 4, 1, 4));

        assertFalse(SmartSelectionMetricsLogger.contains(1, 2, 3, 4));
        assertFalse(SmartSelectionMetricsLogger.contains(3, 4, 1, 2));

        assertFalse(SmartSelectionMetricsLogger.contains(1, 2, 2, 4));
        assertFalse(SmartSelectionMetricsLogger.contains(2, 4, 1, 2));
    }

    @Test
    @Feature({"TextInput", "SmartSelection"})
    public void testUpdateSelectionState() {
        SmartSelectionMetricsLogger logger = new SmartSelectionMetricsLogger();
        String address = "1600 Amphitheatre Parkway, Mountain View, CA 94043.";
        logger.updateSelectionState(address.substring(18, 25), 18, /* force = */ false);
        assertEquals("Parkway", logger.getSelectionText());
        assertEquals(18, logger.getSelectionStartOffset());

        // Drag.
        logger.updateSelectionState(address.substring(5, 35), 5, /* force = */ false);
        assertEquals("Amphitheatre Parkway, Mountain", logger.getSelectionText());
        assertEquals(5, logger.getSelectionStartOffset());

        // Drag again, overlap.
        logger.updateSelectionState(address.substring(18, 40), 18, /* force = */ false);
        assertEquals("Amphitheatre Parkway, Mountain View", logger.getSelectionText());
        assertEquals(5, logger.getSelectionStartOffset());

        // Extend to the whole selection range.
        logger.updateSelectionState(address, 0, /* force = */ false);
        assertEquals(address, logger.getSelectionText());
        assertEquals(0, logger.getSelectionStartOffset());

        // Drag again/Reset.
        logger.updateSelectionState(address.substring(18, 25), 8, /* force = */ false);
        assertEquals(address, logger.getSelectionText());
        assertEquals(0, logger.getSelectionStartOffset());
    }

    @Test
    @Feature({"TextInput", "SmartSelection"})
    public void testGetWordDelta() {
        SmartSelectionMetricsLogger logger = new SmartSelectionMetricsLogger();
        //            -1       0    1   2         3     45
        String test = "It\u00A0has\nnon breaking\tspaces.";
        logger.updateSelectionState(test, 0, /* force = */ false);
        logger.setOriginalOffset(3);

        // Whole range.
        logger.getWordDelta(0, test.length());
        int[] indices = logger.getLastIndices();
        assertEquals(-1, indices[0]);
        assertEquals(5, indices[1]);

        // Itself.
        logger.getWordDelta(3, 6);
        indices = logger.getLastIndices();
        assertEquals(0, indices[0]);
        assertEquals(1, indices[1]);

        // No overlap left.
        logger.getWordDelta(0, 2);
        indices = logger.getLastIndices();
        assertEquals(-1, indices[0]);
        assertEquals(0, indices[1]);

        // No overlap right.
        logger.getWordDelta(20, 25);
        indices = logger.getLastIndices();
        assertEquals(3, indices[0]);
        assertEquals(4, indices[1]);

        // Partial word case.
        logger.getWordDelta(22, 25);
        indices = logger.getLastIndices();
        assertEquals(3, indices[0]);
        assertEquals(4, indices[1]);
    }

    @Test
    @Feature({"TextInput", "SmartSelection"})
    public void testIsWhitespace() {
        SmartSelectionMetricsLogger logger = new SmartSelectionMetricsLogger();
        String test = "\u202F\u00A0 \t\n\u000b\f\r";
        logger.updateSelectionState(test, 0, /* force = */ false);
        assertTrue(logger.isWhitespace(0, test.length()));
    }
}
