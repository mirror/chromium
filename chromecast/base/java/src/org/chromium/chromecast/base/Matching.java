// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chromecast.base;

import org.chromium.chromecast.base.Functions.Consumer;
import org.chromium.chromecast.base.Functions.Function;

/**
 * Abstract generic Visitor pattern.
 *
 * The Visitor pattern has a Visitable class with a visit() method, which takes a Visitor argument.
 * The visitor is used similarly to a switch statement: for each possible condition in the
 * visitiable object, it implements a method that the Visitable calls if the condition applies.
 *
 * This package provides two versions: Visitor and Matcher. Visitor methods return void, whereas
 * Matcher methods return a parameterized type. Where Visitors are useful in procedural-style
 * programs, Matchers are useful in functional-style programs.
 */
public class Matching {
    /**
     * Types that implement Matchable will be able to consume a Matcher and return a result based on
     * it. It is the generalized version of the Visitor pattern, with an arbitrary return type.
     *
     * @param <A> The first type.
     * @param <B> The second type.
     * @param <R> The type to return, regardless of whether the first or second is matched.
     */
    public interface Matcher<A, B, R> {
        public R first(A a);
        public R second(B b);
    }

    /**
     * Builder class for Matchers.
     *
     * Instead of creating an anonymous class that overrides Matcher, you can use this to construct
     * a Matcher using new Match().first(...).second(...).
     */
    public static class Match {
        public <A, R> PartialMatcher<A, R> first(Function<A, R> firstHandler) {
            return new PartialMatcher<>(firstHandler);
        }
    }

    /**
     * Incomplete Matcher built with Match.first(). Call second() to return a full Matcher.
     *
     * @param <A> The type provided to Matcher.first().
     * @param <R> The return type of the encapsulated functions.
     */
    public static final class PartialMatcher<A, R> {
        private final Function<A, R> mFirstHandler;

        private PartialMatcher(Function<A, R> firstHandler) {
            mFirstHandler = firstHandler;
        }

        public <B> Matcher<A, B, R> second(Function<B, ? extends R> secondHandler) {
            return new Matcher<A, B, R>() {
                @Override
                public R first(A a) {
                    return mFirstHandler.apply(a);
                }
                @Override
                public R second(B b) {
                    return secondHandler.apply(b);
                }
            };
        }
    }

    /**
     * Generic consumer of Matchers.
     *
     * Implementations should return the result of one of the Matcher's methods.
     *
     * @param <A> The first type that can be matched.
     * @param <B> The second type that can be matched.
     */
    public interface Matchable<A, B> {
        public <R> R match(Matcher<? super A, ? super B, R> matcher);
    }

    /**
     * Types that implement Visitable will be able to consume a Visitor and execute one or more of
     * its methods. Which Visitor methods get called is dependent on the type being visited.
     *
     * @param <A> The first type.
     * @param <B> The second type.
     */
    public interface Visitor<A, B> {
        public void first(A a);
        public void second(B b);
    }

    /**
     * Builder class for Visitors.
     *
     * Instead of creating an anonymous class that overrides Visitor, you can use this to construct
     * a Visitor using new Visit().first(...).second(...).
     */
    public static final class Visit {
        public <A> PartialVisitor<A> first(Consumer<A> firstHandler) {
            return new PartialVisitor<>(firstHandler);
        }
    }

    /**
     * Incomplete Visitor built with Visit.first(). Call second() to return a full Visitor.
     *
     * @param <A> The type provided to Visitor.first().
     */
    public static final class PartialVisitor<A> {
        private final Consumer<A> mFirstHandler;

        private PartialVisitor(Consumer<A> firstHandler) {
            mFirstHandler = firstHandler;
        }

        public <B> Visitor<A, B> second(Consumer<B> secondHandler) {
            return new Visitor<A, B>() {
                @Override
                public void first(A a) {
                    mFirstHandler.accept(a);
                }
                @Override
                public void second(B b) {
                    secondHandler.accept(b);
                }
            };
        }
    }

    /**
     * Generic consumer of Visitors.
     *
     * Implementations may call the Visitor's methods in any order, any number of times.
     *
     * @param <A> The first type that can be visited.
     * @param <B> The second type that can be visited.
     */
    public interface Visitable<A, B> { public void visit(Visitor<? super A, ? super B> visitor); }
}
