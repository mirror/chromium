// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.content.browser;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;
import static org.mockito.Mockito.inOrder;

import android.content.Context;

import org.junit.Before;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.InOrder;
import org.mockito.Mockito;
import org.mockito.MockitoAnnotations;
import org.robolectric.annotation.Config;
import org.robolectric.shadows.ShadowLog;

import org.chromium.base.test.util.Feature;
import org.chromium.content.browser.SmartSelectionMetricsLogger.ActionType;
import org.chromium.testing.local.LocalRobolectricTestRunner;

/**
 * Unit tests for the {@link SmartSelectionMetricsLogger}.
 */
@RunWith(LocalRobolectricTestRunner.class)
@Config(manifest = Config.NONE)
public class SmartSelectionMetricsLoggerTest {
    // Char index                  0         1         2         3         4
    // Word index (thou)          -7-6   -5-4   -3-2        -1   0    1    2
    private static String sText = "O Romeo, Romeo! Wherefore art thou Romeo?\n"
            //         5         6         7
            // 3    4   5      6   7      8   9   0
            + "Deny thy father and refuse thy name.\n"
            // 8         9         0         1         2
            // 1 2 3  4    5    6  7 8  9   0     1  2   3
            + "Or, if thou wilt not, be but sworn my love,\n"
            //       3         4         5
            // 4   567  8  9      0  1 2      3
            + "And I’ll no longer be a Capulet.\n";
    private static class TestSmartSelectionMetricsLogger extends SmartSelectionMetricsLogger {
        @Override
        public void logEvent(Object selectionEvent) {
            // no-op
        }

        @Override
        public Object createTracker(Context context, boolean editable) {
            return new Object();
        }
    }

    @Before
    public void setUp() throws Exception {
        MockitoAnnotations.initMocks(this);
        ShadowLog.stream = System.out;
    }

    @Test
    @Feature({"TextInput", "SmartSelection"})
    public void testOverlap() {
        assertTrue(SelectionIndicesConverter.overlap(1, 3, 2, 4));
        assertTrue(SelectionIndicesConverter.overlap(2, 4, 1, 3));

        assertTrue(SelectionIndicesConverter.overlap(1, 4, 2, 3));
        assertTrue(SelectionIndicesConverter.overlap(2, 3, 1, 4));

        assertTrue(SelectionIndicesConverter.overlap(1, 4, 1, 3));
        assertTrue(SelectionIndicesConverter.overlap(1, 3, 1, 4));

        assertTrue(SelectionIndicesConverter.overlap(1, 4, 2, 4));
        assertTrue(SelectionIndicesConverter.overlap(2, 4, 1, 4));

        assertTrue(SelectionIndicesConverter.overlap(1, 4, 1, 4));

        assertFalse(SelectionIndicesConverter.overlap(1, 2, 3, 4));
        assertFalse(SelectionIndicesConverter.overlap(3, 4, 1, 2));

        assertFalse(SelectionIndicesConverter.overlap(1, 2, 2, 4));
        assertFalse(SelectionIndicesConverter.overlap(2, 4, 1, 2));
    }

    @Test
    @Feature({"TextInput", "SmartSelection"})
    public void testContains() {
        assertFalse(SelectionIndicesConverter.contains(1, 3, 2, 4));
        assertFalse(SelectionIndicesConverter.contains(2, 4, 1, 3));

        assertTrue(SelectionIndicesConverter.contains(1, 4, 2, 3));
        assertFalse(SelectionIndicesConverter.contains(2, 3, 1, 4));

        assertTrue(SelectionIndicesConverter.contains(1, 4, 1, 3));
        assertFalse(SelectionIndicesConverter.contains(1, 3, 1, 4));

        assertTrue(SelectionIndicesConverter.contains(1, 4, 2, 4));
        assertFalse(SelectionIndicesConverter.contains(2, 4, 1, 4));

        assertTrue(SelectionIndicesConverter.contains(1, 4, 1, 4));

        assertFalse(SelectionIndicesConverter.contains(1, 2, 3, 4));
        assertFalse(SelectionIndicesConverter.contains(3, 4, 1, 2));

        assertFalse(SelectionIndicesConverter.contains(1, 2, 2, 4));
        assertFalse(SelectionIndicesConverter.contains(2, 4, 1, 2));
    }

    @Test
    @Feature({"TextInput", "SmartSelection"})
    public void testUpdateSelectionState() {
        SelectionIndicesConverter converter = new SelectionIndicesConverter();
        String address = "1600 Amphitheatre Parkway, Mountain View, CA 94043.";
        converter.updateSelectionState(address.substring(18, 25), 18, /* force = */ false);
        assertEquals("Parkway", converter.getSelectionText());
        assertEquals(18, converter.getSelectionStartOffset());

        // Drag.
        converter.updateSelectionState(address.substring(5, 35), 5, /* force = */ false);
        assertEquals("Amphitheatre Parkway, Mountain", converter.getSelectionText());
        assertEquals(5, converter.getSelectionStartOffset());

        // Drag again, overlap.
        converter.updateSelectionState(address.substring(18, 40), 18, /* force = */ false);
        assertEquals("Amphitheatre Parkway, Mountain View", converter.getSelectionText());
        assertEquals(5, converter.getSelectionStartOffset());

        // Extend to the whole selection range.
        converter.updateSelectionState(address, 0, /* force = */ false);
        assertEquals(address, converter.getSelectionText());
        assertEquals(0, converter.getSelectionStartOffset());

        // Drag again/Reset.
        converter.updateSelectionState(address.substring(18, 25), 8, /* force = */ false);
        assertEquals(address, converter.getSelectionText());
        assertEquals(0, converter.getSelectionStartOffset());
    }

    @Test
    @Feature({"TextInput", "SmartSelection"})
    public void testIsWhitespace() {
        SelectionIndicesConverter converter = new SelectionIndicesConverter();
        String test = "\u202F\u00A0 \t\n\u000b\f\r";
        converter.updateSelectionState(test, 0, /* force = */ false);
        assertTrue(converter.isWhitespace(0, test.length()));
    }

    @Test
    @Feature({"TextInput", "SmartSelection"})
    public void testCountWordsBackward() {
        SelectionIndicesConverter converter = new SelectionIndicesConverter();
        converter.updateSelectionState(sText, 0, /* force = */ false);
        converter.getWordDelta(0, 0);
        assertEquals(0, converter.countWordsBackward(42, 42));
        assertEquals(8, converter.countWordsBackward(79, 42));
        assertEquals(31, converter.countWordsBackward(sText.length(), 42));
    }

    @Test
    @Feature({"TextInput", "SmartSelection"})
    public void testCountWordsForward() {
        SelectionIndicesConverter converter = new SelectionIndicesConverter();
        converter.updateSelectionState(sText, 0, /* force = */ false);
        converter.getWordDelta(0, 0);
        assertEquals(0, converter.countWordsForward(42, 42));
        assertEquals(10, converter.countWordsForward(0, 42));
        assertEquals(31, converter.countWordsForward(42, sText.length()));
    }

    @Test
    @Feature({"TextInput", "SmartSelection"})
    public void testGetWordDelta() {
        SelectionIndicesConverter converter = new SelectionIndicesConverter();
        // char offset 0         5     0    5     0    5
        //            -1       0    1   2         3     45
        String test = "It\u00A0has\nnon breaking\tspaces.";
        converter.updateSelectionState(test, 0, /* force = */ false);
        converter.setInitialStartOffset(3);

        // Whole range. "It\u00A0has\nnon breaking\tspaces."
        int[] indices = converter.getWordDelta(0, test.length());
        assertEquals(-1, indices[0]);
        assertEquals(5, indices[1]);

        // Itself. "has"
        indices = converter.getWordDelta(3, 6);
        assertEquals(0, indices[0]);
        assertEquals(1, indices[1]);

        // No overlap left. "It"
        indices = converter.getWordDelta(0, 2);
        assertEquals(-1, indices[0]);
        assertEquals(0, indices[1]);

        // No overlap right. "space"
        indices = converter.getWordDelta(20, 25);
        assertEquals(3, indices[0]);
        assertEquals(4, indices[1]);

        // "breaking\tspace"
        indices = converter.getWordDelta(11, 25);
        assertEquals(2, indices[0]);
        assertEquals(4, indices[1]);

        // Extra space case. " breaking\tspace"
        indices = converter.getWordDelta(10, 25);
        assertEquals(2, indices[0]);
        assertEquals(4, indices[1]);
    }

    @Test
    @Feature({"TextInput", "SmartSelection"})
    public void testNormalLoggingFlow() {
        SmartSelectionMetricsLogger.SelectionEventProxy selectionEventProxy =
                Mockito.mock(SmartSelectionMetricsLogger.SelectionEventProxy.class);
        TestSmartSelectionMetricsLogger logger = new TestSmartSelectionMetricsLogger();
        InOrder order = inOrder(selectionEventProxy);
        logger.setSelectionEventProxy(selectionEventProxy);

        // Start to select, selected "thou" on in row#1.
        logger.logSelectionStarted("thou", 30, /* editable = */ false);
        order.verify(selectionEventProxy).selectionStarted(0);

        // Smart Selection, expand to "Wherefore art thou Romeo?".
        logger.logSelectionModified("Wherefore art thou Romeo?", 16, null);
        order.verify(selectionEventProxy).selectionModified(-2, 3);

        // Smart Selection reset, to the last Romeo in row#1.
        logger.logSelectionAction("Romeo", 35, ActionType.RESET, null);
        order.verify(selectionEventProxy).selectionAction(1, 2, ActionType.RESET);

        // User clear selection.
        logger.logSelectionAction("Romeo", 35, ActionType.ABANDON, null);
        order.verify(selectionEventProxy).selectionAction(1, 2, ActionType.ABANDON);

        // Start new selection. First "Deny" in row#2.
        logger.logSelectionStarted("Deny", 42, /* editable = */ false);
        order.verify(selectionEventProxy).selectionStarted(0);

        // Drag right handle to "father".
        logger.logSelectionAction("Deny thy father", 42, ActionType.DRAG, null);
        order.verify(selectionEventProxy).selectionAction(0, 3, ActionType.DRAG);

        // Drag left handle to " and refuse"
        logger.logSelectionAction(" and refuse", 57, ActionType.DRAG, null);
        order.verify(selectionEventProxy).selectionAction(3, 5, ActionType.DRAG);

        // Drag right handle to " Romeo?\nDeny thy father".
        logger.logSelectionAction(" Romeo?\nDeny thy father", 34, ActionType.DRAG, null);
        order.verify(selectionEventProxy).selectionAction(-2, 3, ActionType.DRAG);

        // User start a new selection without abandon.
        logger.logSelectionStarted("thou", 30, /* editable = */ false);
        order.verify(selectionEventProxy).selectionStarted(0);

        // Smart Selection, expand to "Wherefore art thou Romeo?".
        logger.logSelectionModified("Wherefore art thou Romeo?", 16, null);
        order.verify(selectionEventProxy).selectionModified(-2, 3);

        // COPY, PASTE, CUT, SHARE, SMART_SHARE are basically the same.
        logger.logSelectionAction("Wherefore art thou Romeo?", 16, ActionType.COPY, null);
        order.verify(selectionEventProxy).selectionAction(-2, 3, ActionType.COPY);

        // SELECT_ALL
        logger.logSelectionStarted("thou", 30, /* editable = */ true);
        order.verify(selectionEventProxy).selectionStarted(0);

        logger.logSelectionAction(sText, 0, ActionType.SELECT_ALL, null);
        order.verify(selectionEventProxy).selectionAction(-7, 34, ActionType.SELECT_ALL);
    }
}
