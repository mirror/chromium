// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chromecast.base;

import static org.hamcrest.Matchers.contains;
import static org.hamcrest.Matchers.emptyIterable;
import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertNull;
import static org.junit.Assert.assertThat;
import static org.junit.Assert.assertTrue;

import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.BlockJUnit4ClassRunner;

import org.chromium.chromecast.base.CompositeTypes.Both;
import org.chromium.chromecast.base.CompositeTypes.Either;
import org.chromium.chromecast.base.CompositeTypes.Maybe;
import org.chromium.chromecast.base.CompositeTypes.Unit;

import java.util.ArrayList;
import java.util.List;

/**
 * Have you heard the gospel of The Structure and Interpretation of Computer Programs?
 */
@RunWith(BlockJUnit4ClassRunner.class)
public class CompositeTypesTest {
    // Concatenates the toString() result of all visited objects.
    // Demonstrates why the "? super T" type parameters are useful: casting visited objects to
    // parent classes to allow more generic programming.
    private String printAllVisitedStrings(CompositeTypes.Visitable<?, ?> visitable) {
        StringBuilder result = new StringBuilder();
        visitable.visit(new CompositeTypes.Visitor<Object, Object>() {
            @Override
            public void first(Object a) {
                result.append(a.toString());
            }
            @Override
            public void second(Object b) {
                result.append(b.toString());
            }
        });
        return result.toString();
    }

    @Test
    public void testVisitBoth() {
        Both<String, String> x = CompositeTypes.both("A", "B");
        assertEquals(printAllVisitedStrings(x), "AB");
    }

    @Test
    public void testVisitFirst() {
        Either<String, String> x = CompositeTypes.first("A");
        assertEquals(printAllVisitedStrings(x), "A");
    }

    @Test
    public void testVisitSecond() {
        Either<String, String> x = CompositeTypes.second("B");
        assertEquals(printAllVisitedStrings(x), "B");
    }

    @Test
    public void testVisitMaybeWithValue() {
        Maybe<String> x = CompositeTypes.some("yes");
        assertEquals(printAllVisitedStrings(x), "yes");
    }

    @Test
    public void testVisitMaybeWithoutValue() {
        Maybe<String> x = CompositeTypes.none();
        assertEquals(printAllVisitedStrings(x), "()");
    }

    @Test
    public void testMaybeIsPresentWhenPresent() {
        Maybe<String> x = CompositeTypes.some("here");
        assertTrue(x.isPresent());
    }

    @Test
    public void testMaybeIsPresentWhenNotPresent() {
        Maybe<String> x = CompositeTypes.none();
        assertFalse(x.isPresent());
    }

    @Test
    public void testMaybeIfPresentWnenPresent() {
        Maybe<String> x = CompositeTypes.some("body");
        List<String> result = new ArrayList<>();
        x.ifPresent((String s) -> result.add(s));
        assertThat(result, contains("body"));
    }

    @Test
    public void testMaybeIfPresentWhenNotPresent() {
        Maybe<String> x = CompositeTypes.none();
        List<String> result = new ArrayList<>();
        x.ifPresent((String s) -> result.add(s));
        assertThat(result, emptyIterable());
    }

    @Test
    public void testMaybeComposeIfPresent() {
        Maybe<String> x = CompositeTypes.some("a")
                                  .ifPresent((String s) -> s + "b")
                                  .ifPresent((String s) -> s + "c");
        assertEquals(printAllVisitedStrings(x), "abc");
    }

    @Test
    public void testMaybeComposeIfPresentMaybe() {
        Maybe<String> x = CompositeTypes.some("1")
                                  .ifPresentMaybe((String s) -> CompositeTypes.some(s + "2"))
                                  .ifPresentMaybe((String s) -> CompositeTypes.some(s + "3"));
        assertEquals(printAllVisitedStrings(x), "123");
    }

    @Test
    public void testAccessMembersOfBoth() {
        Both<Integer, Boolean> x = CompositeTypes.both(10, true);
        assertEquals((int) x.first, 10);
        assertEquals((boolean) x.second, true);
    }

    @Test
    public void testMatchFirst() {
        Either<String, String> x = CompositeTypes.first("match this");
        assertEquals(x.match(new CompositeTypes.Matcher<String, String, String>() {
            @Override
            public String first(String a) {
                return "successfully " + a;
            }
            @Override
            public String second(String b) {
                return "failure";
            }
        }),
                "successfully match this");
    }

    @Test
    public void testMatchSecond() {
        Either<String, String> x = CompositeTypes.second("match that");
        assertEquals(x.match(new CompositeTypes.Matcher<String, String, String>() {
            @Override
            public String first(String a) {
                return "failure";
            }
            @Override
            public String second(String b) {
                return b + " success";
            }
        }),
                "match that success");
    }

    @Test
    public void testMatchMaybeWithValue() {
        Maybe<String> x = CompositeTypes.some("x");
        assertEquals(x.match(new CompositeTypes.Matcher<String, Unit, String>() {
            @Override
            public String first(String s) {
                return "has " + s;
            }
            @Override
            public String second(Unit none) {
                return "uh-oh";
            }
        }),
                "has x");
    }

    @Test
    public void testMatchMaybeWithoutValue() {
        Maybe<String> x = CompositeTypes.none();
        assertEquals(x.match(new CompositeTypes.Matcher<String, Unit, String>() {
            @Override
            public String first(String s) {
                return "whoops";
            }
            @Override
            public String second(Unit none) {
                return "correct";
            }
        }),
                "correct");
    }

    @Test
    public void testDeeplyNestedCompositeType() {
        Both < String, Both < String,
                Both<String, String>>> x = CompositeTypes.both(
                        "A", CompositeTypes.both("B", CompositeTypes.both("C", "D")));
        assertEquals(x.first, "A");
        assertEquals(x.second.first, "B");
        assertEquals(x.second.second.first, "C");
        assertEquals(x.second.second.second, "D");
    }

    @Test
    public void testBothToString() {
        Both<String, String> x = CompositeTypes.both("first", "second");
        assertEquals(x.toString(), "first, second");
    }

    @Test
    public void testFirstToString() {
        Either<String, String> x = CompositeTypes.first("first");
        assertEquals(x.toString(), "first");
    }

    @Test
    public void testSecondToString() {
        Either<String, String> x = CompositeTypes.second("second");
        assertEquals(x.toString(), "second");
    }

    @Test
    public void testNestedBothToString() {
        Both < String, Both < String,
                Both<String, String>>> x = CompositeTypes.both(
                        "1", CompositeTypes.both("2", CompositeTypes.both("3", "4")));
        assertEquals(x.toString(), "1, 2, 3, 4");
    }

    @Test
    public void testUnitToString() {
        Unit u = CompositeTypes.unit();
        assertEquals(u.toString(), "()");
    }

    @Test
    public void testMaybeWithValueToString() {
        Maybe<String> x = CompositeTypes.some("blah");
        assertEquals(x.toString(), "blah");
    }

    @Test
    public void testMaybeWithoutValueToString() {
        Maybe<String> x = CompositeTypes.none();
        assertEquals(x.toString(), "()");
    }

    @Test
    public void testWrapMaybeWithValue() {
        Maybe<String> x = CompositeTypes.wrapMaybe("yo");
        assertEquals(x.toString(), "yo");
    }

    @Test
    public void testWrapMaybeWithNull() {
        Maybe<String> x = CompositeTypes.wrapMaybe(null);
        assertEquals(x.toString(), "()");
    }

    @Test
    public void testUnwrapMaybeWithValue() {
        Maybe<String> x = CompositeTypes.wrapMaybe("wrap this");
        assertEquals(x.unwrap(), "wrap this");
    }

    @Test
    public void testUnwrapMaybeWithNoValue() {
        Maybe<String> x = CompositeTypes.wrapMaybe(null);
        assertNull(x.unwrap(), null);
    }
}
