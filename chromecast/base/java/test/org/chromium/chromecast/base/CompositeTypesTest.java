// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chromecast.base;

import static org.hamcrest.Matchers.contains;
import static org.hamcrest.Matchers.emptyIterable;
import static org.hamcrest.Matchers.not;
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
import org.chromium.chromecast.base.Matching.Match;
import org.chromium.chromecast.base.Matching.Visit;

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
    private String printAllVisitedStrings(Matching.Visitable<?, ?> visitable) {
        StringBuilder result = new StringBuilder();
        visitable.visit(new Visit()
                                .first((Object a) -> result.append(a.toString()))
                                .second((Object b) -> result.append(b.toString())));
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
    public void testMaybeComposeMap() {
        Maybe<String> x =
                CompositeTypes.some("a").map((String s) -> s + "b").map((String s) -> s + "c");
        assertEquals(printAllVisitedStrings(x), "abc");
    }

    @Test
    public void testMaybeComposeFlatMap() {
        Maybe<String> x = CompositeTypes.some("1")
                                  .flatMap((String s) -> CompositeTypes.some(s + "2"))
                                  .flatMap((String s) -> CompositeTypes.some(s + "3"));
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
        assertEquals(x.match(new Match()
                                     .first((String a) -> "successfully " + a)
                                     .second((String b) -> "failure")),
                "successfully match this");
    }

    @Test
    public void testMatchSecond() {
        Either<String, String> x = CompositeTypes.second("match that");
        assertEquals(x.match(new Match()
                                     .first((String a) -> "failure")
                                     .second((String b) -> b + " success")),
                "match that success");
    }

    @Test
    public void testMatchMaybeWithValue() {
        Maybe<String> x = CompositeTypes.some("x");
        assertEquals(
                x.match(new Match().first((String s) -> "has " + s).second((Unit none) -> "uh-oh")),
                "has x");
    }

    @Test
    public void testMatchMaybeWithoutValue() {
        Maybe<String> x = CompositeTypes.none();
        assertEquals(
                x.match(new Match().first((String s) -> "whoops").second((Unit none) -> "correct")),
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
    public void testUnitEquals() {
        assertEquals(CompositeTypes.unit(), CompositeTypes.unit());
        assertThat(CompositeTypes.unit(), not("()"));
        assertThat(CompositeTypes.unit(),
                not(CompositeTypes.both(CompositeTypes.unit(), CompositeTypes.unit())));
    }

    @Test
    public void testBothEquals() {
        assertEquals(CompositeTypes.both("a", "b"), CompositeTypes.both("a", "b"));
        assertThat(CompositeTypes.both("a", "b"), not(CompositeTypes.both("b", "a")));
        assertThat(CompositeTypes.both("b", "a"), not(CompositeTypes.both("a", "b")));
        assertThat(CompositeTypes.both("a", "a"), not(CompositeTypes.both("b", "b")));
    }

    @Test
    public void testEitherEquals() {
        assertEquals(CompositeTypes.first("a"), CompositeTypes.first("a"));
        assertThat(CompositeTypes.first("a"), not(CompositeTypes.second("a")));
        assertThat(CompositeTypes.first("a"), not(CompositeTypes.first("b")));
    }

    @Test
    public void testMaybeEquals() {
        assertEquals(CompositeTypes.none(), CompositeTypes.none());
        assertThat(CompositeTypes.none(), not(CompositeTypes.some("a")));
        assertThat(CompositeTypes.some("a"), not(CompositeTypes.none()));
        assertThat(CompositeTypes.some("a"), not(CompositeTypes.some("b")));
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

    @Test
    public void testUnwrapIsInverseOfWrapMaybe() {
        assertEquals(CompositeTypes.wrapMaybe("hey").unwrap(), "hey");
        assertEquals(
                CompositeTypes.wrapMaybe(CompositeTypes.wrapMaybe("how").unwrap()).unwrap(), "how");
    }

    @Test
    public void testBothHashCodeIsEqualIfObjectsAreEqual() {
        assertEquals(
                CompositeTypes.both("a", "b").hashCode(), CompositeTypes.both("a", "b").hashCode());
    }

    @Test
    public void testBothHashCodesAreNotEqualWhenObjectsAreNotEqual() {
        assertThat(CompositeTypes.both("a", "b").hashCode(),
                not(CompositeTypes.both("a", "c").hashCode()));
    }

    @Test
    public void testBothHashCodeIsNotInvertible() {
        assertThat(CompositeTypes.both("a", "b").hashCode(),
                not(CompositeTypes.both("b", "a").hashCode()));
    }

    @Test
    public void testEitherHashCodeIsEqualIfObjectsAreEqual() {
        assertEquals(CompositeTypes.first("a").hashCode(), CompositeTypes.first("a").hashCode());
        assertEquals(CompositeTypes.second("b").hashCode(), CompositeTypes.second("b").hashCode());
    }

    @Test
    public void testEitherHashCodeIsNotEqualWhenObjectsAreNotEqual() {
        assertThat(CompositeTypes.first("a").hashCode(), not(CompositeTypes.first("b").hashCode()));
    }

    @Test
    public void testEitherHashCodeIsNotEqualWhenBranchIsDifferent() {
        assertThat(
                CompositeTypes.first("a").hashCode(), not(CompositeTypes.second("a").hashCode()));
    }

    @Test
    public void testMaybeHashCodeIsDerivedFromUnderlyingEitherType() {
        assertEquals(CompositeTypes.none().hashCode(),
                CompositeTypes.second(CompositeTypes.unit()).hashCode());
        assertEquals(CompositeTypes.some("a").hashCode(), CompositeTypes.first("a").hashCode());
    }
}
