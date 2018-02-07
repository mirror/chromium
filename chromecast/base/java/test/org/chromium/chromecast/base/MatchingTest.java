// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chromecast.base;

import static org.hamcrest.Matchers.contains;
import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertThat;

import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.BlockJUnit4ClassRunner;

import org.chromium.chromecast.base.Matching.Match;
import org.chromium.chromecast.base.Matching.Matchable;
import org.chromium.chromecast.base.Matching.Matcher;
import org.chromium.chromecast.base.Matching.Visit;
import org.chromium.chromecast.base.Matching.Visitable;
import org.chromium.chromecast.base.Matching.Visitor;

import java.util.ArrayList;
import java.util.List;

/**
 * Tests for Matching, an abstraction for the Visitor pattern with two options.
 */
@RunWith(BlockJUnit4ClassRunner.class)
public class MatchingTest {
    private Matchable<String, String> matchFirst(String value) {
        return new Matchable<String, String>() {
            @Override
            public <R> R match(Matcher<? super String, ? super String, R> matcher) {
                return matcher.first(value);
            }
        };
    }

    private Matchable<String, String> matchSecond(String value) {
        return new Matchable<String, String>() {
            @Override
            public <R> R match(Matcher<? super String, ? super String, R> matcher) {
                return matcher.second(value);
            }
        };
    }

    private Visitable<String, String> visitFirst(String value) {
        return new Visitable<String, String>() {
            @Override
            public void visit(Visitor<? super String, ? super String> visitor) {
                visitor.first(value);
            }
        };
    }

    private Visitable<String, String> visitSecond(String value) {
        return new Visitable<String, String>() {
            @Override
            public void visit(Visitor<? super String, ? super String> visitor) {
                visitor.second(value);
            }
        };
    }
    @Test
    public void testMatchFirstWithCustomClass() {
        assertEquals(matchFirst("abba").match(new Matcher<String, String, String>() {
            @Override
            public String first(String s) {
                return "match first: " + s;
            }
            @Override
            public String second(String s) {
                return "match second: " + s;
            }
        }),
                "match first: abba");
    }

    @Test
    public void testMatchSecondWithCustomClass() {
        assertEquals(matchSecond("ada").match(new Matcher<String, String, String>() {
            @Override
            public String first(String s) {
                return "match first: " + s;
            }
            @Override
            public String second(String s) {
                return "match second: " + s;
            }
        }),
                "match second: ada");
    }

    @Test
    public void testMatchFirstWithBuilder() {
        assertEquals(matchFirst("megalovania")
                             .match(new Match()
                                             .first(s -> "match first: " + s)
                                             .second(s -> "match second: " + s)),
                "match first: megalovania");
    }

    @Test
    public void testMatchSecondWithBuilder() {
        assertEquals(matchSecond("megalomania")
                             .match(new Match()
                                             .first(s -> "match first: " + s)
                                             .second(s -> "match second: " + s)),
                "match second: megalomania");
    }

    @Test
    public void testVisitFirstWithCustomClass() {
        List<String> result = new ArrayList<>();
        visitFirst("high").visit(new Visitor<String, String>() {
            @Override
            public void first(String s) {
                result.add("match first: " + s);
            }
            @Override
            public void second(String s) {
                result.add("match second: " + s);
            }
        });
        assertThat(result, contains("match first: high"));
    }

    @Test
    public void testVisitSecondWithCustomClass() {
        List<String> result = new ArrayList<>();
        visitSecond("quality").visit(new Visitor<String, String>() {
            @Override
            public void first(String s) {
                result.add("match first: " + s);
            }
            @Override
            public void second(String s) {
                result.add("match second: " + s);
            }
        });
        assertThat(result, contains("match second: quality"));
    }

    @Test
    public void testVisitFirstWithBuilder() {
        List<String> result = new ArrayList<>();
        visitFirst("rip").visit(new Visit()
                                        .first(s -> result.add("match first: " + s))
                                        .second(s -> result.add("match second: " + s)));
        assertThat(result, contains("match first: rip"));
    }

    @Test
    public void testVisitSecondWithBuilder() {
        List<String> result = new ArrayList<>();
        visitSecond("grand dad")
                .visit(new Visit()
                                .first(s -> result.add("match first: " + s))
                                .second(s -> result.add("match second: " + s)));
        assertThat(result, contains("match second: grand dad"));
    }
}
