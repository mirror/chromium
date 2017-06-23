// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.
package org.chromium.chrome.browser.omnibox;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertNull;
import static org.junit.Assert.assertTrue;

import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.BlockJUnit4ClassRunner;

/**
 * Unit tests for {@link AutocompleteState}.
 */
@RunWith(BlockJUnit4ClassRunner.class)
public class AutocompleteStateUnitTest {
    @Test
    public void testIsProperSubstring() {
        assertTrue(AutocompleteState.isProperSubstringOf("ab", "abc"));

        assertFalse(AutocompleteState.isProperSubstringOf("ab", "ab"));
        assertFalse(AutocompleteState.isProperSubstringOf("abc", "ab"));
    }

    @Test
    public void testGetBackwardDeletedTextFrom() {
        assertEquals("c",
                new AutocompleteState("ab", "", 2, 2)
                        .getBackwardDeletedTextFrom(new AutocompleteState("abc", "d", 3, 3)));
        // A string is not seen as backward deleted from itself.
        assertEquals(null,
                new AutocompleteState("ab", "", 2, 2)
                        .getBackwardDeletedTextFrom(new AutocompleteState("ab", "d", 2, 2)));
        // Reversed.
        assertEquals(null,
                new AutocompleteState("abc", "", 3, 3)
                        .getBackwardDeletedTextFrom(new AutocompleteState("ab", "d", 2, 2)));
        // Selection not valid.
        assertNull(new AutocompleteState("ab", "", 2, 2)
                           .getBackwardDeletedTextFrom(new AutocompleteState("abc", "d", 2, 3)));
        assertNull(new AutocompleteState("ab", "", 2, 3)
                           .getBackwardDeletedTextFrom(new AutocompleteState("abc", "d", 2, 2)));
    }

    private void verifyShift(AutocompleteState s1, AutocompleteState s2, boolean expectedRetVal,
            String expectedAutocompleteText) {
        assertEquals(expectedRetVal, s2.shiftAutocompleteTextFrom(s1));
        assertEquals(expectedAutocompleteText, s2.getAutocompleteText());
    }

    @Test
    public void testShiftAutocompleteFrom() {
        verifyShift(new AutocompleteState("ab", "cd", 2, 2), new AutocompleteState("abc", "", 3, 3),
                true, "d");
        verifyShift(new AutocompleteState("ab", "dc", 2, 2), new AutocompleteState("abc", "", 3, 3),
                false, "");
        verifyShift(new AutocompleteState("ab", "dc", 2, 2), new AutocompleteState("ab", "", 2, 2),
                true, "dc");
    }
}
