// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.content.browser;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertNull;
import static org.junit.Assert.assertTrue;
import static org.mockito.ArgumentMatchers.anyInt;
import static org.mockito.Mockito.inOrder;
import static org.mockito.Mockito.never;

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
        public TestSmartSelectionMetricsLogger(SelectionEventProxy selectionEventProxy) {
            super(selectionEventProxy);
        }

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
        assertTrue(converter.updateSelectionState(address.substring(18, 25), 18, false));
        assertEquals("Parkway", converter.getGlobalSelectionText());
        assertEquals(18, converter.getGlobalStartOffset());

        // Expansion.
        assertTrue(converter.updateSelectionState(address.substring(5, 35), 5, false));
        assertEquals("Amphitheatre Parkway, Mountain", converter.getGlobalSelectionText());
        assertEquals(5, converter.getGlobalStartOffset());

        // Drag left handle. Select "Mountain".
        assertTrue(converter.updateSelectionState(address.substring(27, 35), 27, false));
        assertEquals("Amphitheatre Parkway, Mountain", converter.getGlobalSelectionText());
        assertEquals(5, converter.getGlobalStartOffset());

        // Drag left handle. Select " View".
        assertTrue(converter.updateSelectionState(address.substring(35, 40), 35, false));
        assertEquals("Amphitheatre Parkway, Mountain View", converter.getGlobalSelectionText());
        assertEquals(5, converter.getGlobalStartOffset());

        // Drag left handle. Select "1600 Amphitheatre Parkway, Mountain View".
        assertTrue(converter.updateSelectionState(address.substring(0, 40), 0, false));
        assertEquals(
                "1600 Amphitheatre Parkway, Mountain View", converter.getGlobalSelectionText());
        assertEquals(0, converter.getGlobalStartOffset());
    }

    @Test
    @Feature({"TextInput", "SmartSelection"})
    public void testUpdateSelectionStateOffsets() {
        SelectionIndicesConverter converter = new SelectionIndicesConverter();
        String address = "1600 Amphitheatre Parkway, Mountain View, CA 94043.";
        assertTrue(converter.updateSelectionState(address.substring(18, 25), 18, false));
        assertEquals("Parkway", converter.getGlobalSelectionText());
        assertEquals(18, converter.getGlobalStartOffset());

        // Drag to select "Amphitheatre Parkway". Overlap.
        assertFalse(converter.updateSelectionState(address.substring(5, 25), 18, false));
        assertNull(converter.getGlobalSelectionText());

        // Reset.
        assertTrue(converter.updateSelectionState(address.substring(18, 25), 18, true));
        assertEquals("Parkway", converter.getGlobalSelectionText());
        assertEquals(18, converter.getGlobalStartOffset());

        // Drag to select "Amphitheatre Parkway". Overlap.
        assertFalse(converter.updateSelectionState(address.substring(5, 25), 0, false));
        assertNull(converter.getGlobalSelectionText());

        // Reset.
        assertTrue(converter.updateSelectionState(address.substring(36, 40), 36, true));
        assertEquals("View", converter.getGlobalSelectionText());
        assertEquals(36, converter.getGlobalStartOffset());

        // Drag to select "Mountain View". Not overlap.
        assertFalse(converter.updateSelectionState(address.substring(27, 40), 0, false));
        assertNull(converter.getGlobalSelectionText());

        // Reset.
        assertTrue(converter.updateSelectionState(address.substring(36, 40), 36, true));
        assertEquals("View", converter.getGlobalSelectionText());
        assertEquals(36, converter.getGlobalStartOffset());

        // Drag to select "Mountain View". Adjacent. Wrong case.
        assertTrue(converter.updateSelectionState(address.substring(27, 40), 40, false));
        assertEquals("ViewMountain View", converter.getGlobalSelectionText());
        assertEquals(36, converter.getGlobalStartOffset());

        String text = "a a a a a";
        // Reset.
        assertTrue(converter.updateSelectionState(text.substring(2, 3), 2, true));
        assertEquals("a", converter.getGlobalSelectionText());
        assertEquals(2, converter.getGlobalStartOffset());

        // Drag to select "a a". Contains. Wrong case.
        assertTrue(converter.updateSelectionState(text.substring(4, 7), 2, false));
        assertEquals("a a", converter.getGlobalSelectionText());
        assertEquals(2, converter.getGlobalStartOffset());
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
        converter.updateSelectionState(sText, 0, /* force = */ true);
        converter.setIteratorText(sText);
        // Start from "Deny"
        assertEquals(0, converter.countWordsBackward(42, 42));
        assertEquals(1, converter.countWordsBackward(43, 42));
        assertEquals(8, converter.countWordsBackward(79, 42));
        assertEquals(31, converter.countWordsBackward(sText.length(), 42));

        // Start from "e" in "Deny"
        assertEquals(1, converter.countWordsBackward(44, 43));
        assertEquals(8, converter.countWordsBackward(79, 43));
        assertEquals(31, converter.countWordsBackward(sText.length(), 43));
    }

    @Test
    @Feature({"TextInput", "SmartSelection"})
    public void testCountWordsForward() {
        SelectionIndicesConverter converter = new SelectionIndicesConverter();
        converter.updateSelectionState(sText, 0, /* force = */ true);
        converter.setIteratorText(sText);
        // Start from "Deny"
        assertEquals(0, converter.countWordsForward(42, 42));
        assertEquals(1, converter.countWordsForward(42, 43));
        assertEquals(10, converter.countWordsForward(0, 42));
        assertEquals(31, converter.countWordsForward(42, sText.length()));

        // Start from "e" in "Deny"
        assertEquals(1, converter.countWordsForward(42, 43));
        assertEquals(10, converter.countWordsForward(0, 42));
        assertEquals(31, converter.countWordsForward(43, sText.length()));
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
        int[] indices = new int[2];
        // Whole range. "It\u00A0has\nnon breaking\tspaces."
        converter.getWordDelta(0, test.length(), indices);
        assertEquals(-1, indices[0]);
        assertEquals(5, indices[1]);

        // Itself. "has"
        converter.getWordDelta(3, 6, indices);
        assertEquals(0, indices[0]);
        assertEquals(1, indices[1]);

        // No overlap left. "It"
        converter.getWordDelta(0, 2, indices);
        assertEquals(-1, indices[0]);
        assertEquals(0, indices[1]);

        // No overlap right. "space"
        converter.getWordDelta(20, 25, indices);
        assertEquals(3, indices[0]);
        assertEquals(4, indices[1]);

        // "breaking\tspace"
        converter.getWordDelta(11, 25, indices);
        assertEquals(2, indices[0]);
        assertEquals(4, indices[1]);

        // Extra space case. " breaking\tspace"
        converter.getWordDelta(10, 25, indices);
        assertEquals(2, indices[0]);
        assertEquals(4, indices[1]);
    }

    @Test
    @Feature({"TextInput", "SmartSelection"})
    public void testNormalLoggingFlow() {
        SmartSelectionMetricsLogger.SelectionEventProxy selectionEventProxy =
                Mockito.mock(SmartSelectionMetricsLogger.SelectionEventProxy.class);
        TestSmartSelectionMetricsLogger logger =
                new TestSmartSelectionMetricsLogger(selectionEventProxy);
        InOrder order = inOrder(selectionEventProxy);

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

    @Test
    @Feature({"TextInput", "SmartSelection"})
    public void testMultipleDrag() {
        SmartSelectionMetricsLogger.SelectionEventProxy selectionEventProxy =
                Mockito.mock(SmartSelectionMetricsLogger.SelectionEventProxy.class);
        TestSmartSelectionMetricsLogger logger =
                new TestSmartSelectionMetricsLogger(selectionEventProxy);
        InOrder order = inOrder(selectionEventProxy);
        // Start new selection. First "Deny" in row#2.
        logger.logSelectionStarted("Deny", 42, /* editable = */ false);
        order.verify(selectionEventProxy).selectionStarted(0);

        // Drag right handle to "father".
        logger.logSelectionAction("Deny thy father", 42, ActionType.DRAG, null);
        order.verify(selectionEventProxy).selectionAction(0, 3, ActionType.DRAG);

        // Drag left handle to " and refuse"
        logger.logSelectionAction(" and refuse", 57, ActionType.DRAG, null);
        order.verify(selectionEventProxy, never()).selectionAction(anyInt(), anyInt(), anyInt());

        // Drag right handle to " Romeo?\nDeny thy father".
        logger.logSelectionAction(" Romeo?\nDeny thy father", 34, ActionType.DRAG, null);
        order.verify(selectionEventProxy, never()).selectionAction(anyInt(), anyInt(), anyInt());

        // Dismiss the selection.
        logger.logSelectionAction(" Romeo?\nDeny thy father", 34, ActionType.ABANDON, null);
        order.verify(selectionEventProxy, never()).selectionAction(anyInt(), anyInt(), anyInt());

        // Start a new selection.
        logger.logSelectionStarted("Deny", 42, /* editable = */ false);
        order.verify(selectionEventProxy).selectionStarted(0);
    }

    @Test
    @Feature({"TextInput", "SmartSelection"})
    public void testTextShift() {
        SmartSelectionMetricsLogger.SelectionEventProxy selectionEventProxy =
                Mockito.mock(SmartSelectionMetricsLogger.SelectionEventProxy.class);
        TestSmartSelectionMetricsLogger logger =
                new TestSmartSelectionMetricsLogger(selectionEventProxy);
        InOrder order = inOrder(selectionEventProxy);

        // Start to select, selected "thou" on in row#1.
        logger.logSelectionStarted("thou", 30, /* editable = */ false);
        order.verify(selectionEventProxy).selectionStarted(0);

        // Smart Selection, expand to "Wherefore art thou Romeo?".
        logger.logSelectionModified("Wherefore art thou Romeo?", 30, null);
        order.verify(selectionEventProxy, never()).selectionModified(anyInt(), anyInt());

        // Start to select, selected "thou" on in row#1.
        logger.logSelectionStarted("thou", 30, /* editable = */ false);
        order.verify(selectionEventProxy).selectionStarted(0);

        // Drag.
        logger.logSelectionAction("Wherefore art thou", 10, ActionType.DRAG, null);
        order.verify(selectionEventProxy, never()).selectionAction(anyInt(), anyInt(), anyInt());

        // Start to select, selected "thou" on in row#1.
        logger.logSelectionStarted("thou", 30, /* editable = */ false);
        order.verify(selectionEventProxy).selectionStarted(0);

        // Drag. Form "Wherefore art thouthou".
        logger.logSelectionAction("Wherefore art thou", 12, ActionType.DRAG, null);
        order.verify(selectionEventProxy).selectionAction(-3, 0, ActionType.DRAG);
    }

    @Test
    @Feature({"TextInput", "SmartSelection"})
    public void testSelectionChanged() {
        SmartSelectionMetricsLogger.SelectionEventProxy selectionEventProxy =
                Mockito.mock(SmartSelectionMetricsLogger.SelectionEventProxy.class);
        TestSmartSelectionMetricsLogger logger =
                new TestSmartSelectionMetricsLogger(selectionEventProxy);
        InOrder order = inOrder(selectionEventProxy);

        // Start to select, selected "thou" on in row#1.
        logger.logSelectionStarted("thou", 30, /* editable = */ false);
        order.verify(selectionEventProxy).selectionStarted(0);

        // "thou" changed to "math"
        logger.logSelectionAction("Wherefore art math", 16, ActionType.DRAG, null);
        order.verify(selectionEventProxy, never()).selectionAction(anyInt(), anyInt(), anyInt());

        // Start to select, selected "thou" on in row#1.
        logger.logSelectionStarted("thou", 30, /* editable = */ false);
        order.verify(selectionEventProxy).selectionStarted(0);

        // Drag while deleting "art ".
        logger.logSelectionAction("Wherefore thou", 16, ActionType.DRAG, null);
        order.verify(selectionEventProxy).selectionAction(-2, 0, ActionType.DRAG);

        // Start to select, selected "thou" on in row#1.
        logger.logSelectionStarted("thou", 30, /* editable = */ false);
        order.verify(selectionEventProxy).selectionStarted(0);

        // Drag while deleting "Wherefore art ".
        logger.logSelectionAction("thou", 16, ActionType.DRAG, null);
        order.verify(selectionEventProxy, never()).selectionAction(anyInt(), anyInt(), anyInt());
    }
}
