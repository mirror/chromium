// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chromecast.base;

import org.chromium.chromecast.base.Functions.Consumer;
import org.chromium.chromecast.base.Functions.Function;
import org.chromium.chromecast.base.Matching.Match;
import org.chromium.chromecast.base.Matching.Matchable;
import org.chromium.chromecast.base.Matching.Matcher;
import org.chromium.chromecast.base.Matching.Visit;
import org.chromium.chromecast.base.Matching.Visitable;
import org.chromium.chromecast.base.Matching.Visitor;

/**
 * Simple types that compose other types. Inspired by algebraic type systems in functional
 * programming languages, these are used with the Visitor pattern to provide simple and elegant
 * composable building blocks for types and generic functions.
 */
public class CompositeTypes {
    /**
     * Represents a type with only one possible instance.
     *
     * Retrieved by calling the static unit() function.
     * In algebraic type theory, this is the 1 (or "Singleton") type.
     *
     * Useful as a type parameter for Vistable or Matchable types that include a dummy type.
     * For example, Maybe<T> matches `T` or `Unit`, where Unit represents the possibility of the
     * object being empty.
     */
    public static final class Unit {
        private static Unit sInstance = null;
        private Unit() {}
        private static Unit getInstance() {
            if (sInstance == null) sInstance = new Unit();
            return sInstance;
        }

        @Override
        public String toString() {
            return "()";
        }
    }

    /**
     * Represents a structure containing an instance of both A and B.
     *
     * When a Visitor visits an instance of this object, both first() and second() are called.
     * In algebraic type theory, this is the Product type.
     *
     * This is useful for representing many objects in one without having to create a new class.
     * The type itself is composable, so the type parameters can themselves be Boths, and so on
     * recursively. This way, one can make a binary tree of types and wrap it in a single type that
     * represents the composition of all the leaf types.
     *
     * @param <A> The first type.
     * @param <B> The second type.
     */
    public static class Both<A, B> implements Visitable<A, B> {
        public final A first;
        public final B second;

        private Both(A a, B b) {
            this.first = a;
            this.second = b;
        }

        // Can be used as a method reference.
        public A getFirst() {
            return this.first;
        }

        // Can be used as a method reference.
        public B getSecond() {
            return this.second;
        }

        @Override
        public void visit(Visitor<? super A, ? super B> visitor) {
            visitor.first(this.first);
            visitor.second(this.second);
        }

        @Override
        public String toString() {
            return new StringBuilder()
                    .append(this.first.toString())
                    .append(", ")
                    .append(this.second.toString())
                    .toString();
        }

        @Override
        public boolean equals(Object otherObject) {
            if (otherObject instanceof Both) {
                Both<?, ?> other = (Both<?, ?>) otherObject;
                return this.first.equals(other.first) && this.second.equals(other.second);
            }
            return false;
        }

        @Override
        public int hashCode() {
            return this.first.hashCode() + 2 * this.second.hashCode();
        }
    }

    /**
     * Represents a structure containing either an instance of A or an instance of B.
     *
     * Construct this type by calling the first() or second() static methods.
     *
     * When a Visitor visits an instance of this object, only one of first() or second() are called.
     * In algebraic type theory, this is the Sum type.
     *
     * This is useful for representing a type that might be an instance of one type or another,
     * without having to create a new class. The Visitor/Matcher pattern provides type safety when
     * inspecting the contents.
     *
     * @param <A> The first type.
     * @param <B> The second type.
     */
    public static class Either<A, B> implements Matchable<A, B>, Visitable<A, B> {
        private final A mFirst;
        private final B mSecond;

        // Exactly one of `a` and `b` is null.
        private Either(A a, B b) {
            mFirst = a;
            mSecond = b;
        }

        @Override
        public <R> R match(Matcher<? super A, ? super B, R> matcher) {
            if (mFirst != null) {
                return matcher.first(mFirst);
            }
            return matcher.second(mSecond);
        }

        @Override
        public void visit(Visitor<? super A, ? super B> visitor) {
            if (mFirst != null) {
                visitor.first(mFirst);
            } else {
                visitor.second(mSecond);
            }
        }

        @Override
        public String toString() {
            if (mFirst != null) return mFirst.toString();
            return mSecond.toString();
        }

        @Override
        public boolean equals(Object otherObject) {
            if (otherObject instanceof Either) {
                Either<?, ?> other = (Either<?, ?>) otherObject;
                return match(new Match()
                                     .first(a1 -> {
                                         return other.match(new Match()
                                                                    .first(a2 -> a1.equals(a2))
                                                                    .second(x -> false));
                                     })
                                     .second(b1 -> {
                                         return other.match(new Match()
                                                                    .first(x -> false)
                                                                    .second(b2 -> b1.equals(b2)));
                                     }));
            }
            return false;
        }

        @Override
        public int hashCode() {
            return match(new Match().first(a -> 2 * a.hashCode()).second(b -> 3 * b.hashCode()));
        }
    }

    /**
     * Equivalent to Either<T, Unit>. Compare to java.util.Optional.
     * (java.util.Optional is only available in API 24 and up.)
     *
     * Construct this type by calling the some() or none() static methods.
     *
     * The Visitable and Matchable implementations are equivalent to the underlying Either<T, Unit>
     * implementation. The first() callback is invoked if this was constructed with some(), and the
     * second() callback is invoked if this was constructed with none().
     * In algebraic type theory, this is the sum of T and Unit.
     *
     * This is useful anywhere that one might use java.util.Optional, or a variable that might be
     * null. The only way for clients to get at the internal object is by using the visitor pattern,
     * adding type safety that isn't present when using nullable variables. It is best practice to
     * avoid null altogether when working with CompositeTypes.
     *
     * @param <T> The underlying type.
     */
    public static class Maybe<T> implements Matchable<T, Unit>, Visitable<T, Unit> {
        private final Either<T, Unit> mData;

        private Maybe(Either<T, Unit> data) {
            mData = data;
        }

        @Override
        public <R> R match(Matcher<? super T, ? super Unit, R> matcher) {
            return mData.match(matcher);
        }

        @Override
        public void visit(Visitor<? super T, ? super Unit> visitor) {
            mData.visit(visitor);
        }

        /**
         * Returns whether this was constructed with some().
         */
        public boolean isPresent() {
            return match(new Match().first(some -> true).second(none -> false));
        }

        /**
         * If this has a value, apply the action and return the result in a Maybe container.
         * If this has no value, return none().
         *
         * Since this returns a Maybe instance, this is composable in a Builder-like pattern.
         */
        public <R> Maybe<R> map(Function<? super T, ? extends R> function) {
            return match(new Match()
                                 .first((T some) -> some(function.apply(some)))
                                 .second(none -> none()));
        }

        /**
         * If this has a value, apply the function and return the result.
         * If this has no value, return none().
         *
         * This is different from map() in that the function must return an instance of Maybe,
         * which allows clients to return none() from an function even if this matched some().
         */
        public <R> Maybe<R> flatMap(Function<? super T, Maybe<R>> function) {
            return match(
                    new Match().first((T some) -> function.apply(some)).second(none -> none()));
        }

        /**
         * If this has a value, execute the action with the value as input. Otherwise, do nothing.
         */
        public void ifPresent(Consumer<T> action) {
            visit(new Visit().first((T some) -> action.accept(some)).second(none -> {}));
        }

        /**
         * Returns null if none(), or the unwrapped value if some().
         */
        public T unwrap() {
            return match(new Match().first((T some) -> some).second(none -> null));
        }

        @Override
        public String toString() {
            return mData.toString();
        }

        @Override
        public boolean equals(Object otherObject) {
            if (otherObject instanceof Maybe) {
                Maybe<?> other = (Maybe<?>) otherObject;
                return mData.equals(other.mData);
            }
            return false;
        }

        @Override
        public int hashCode() {
            return mData.hashCode();
        }
    }

    /**
     * Retrieves the one and only instance of the Unit type.
     */
    public static Unit unit() {
        return Unit.getInstance();
    }

    /**
     * Constructs a Both object containing both `a` and `b`.
     */
    public static <A, B> Both<A, B> both(A a, B b) {
        assert a != null;
        assert b != null;
        return new Both<>(a, b);
    }

    /**
     * Constructs an Either object with `a` in the first position.
     */
    public static <A, B> Either<A, B> first(A a) {
        assert a != null;
        return new Either<>(a, null);
    }

    /**
     * Constructs an Either object with `b` in the second position.
     */
    public static <A, B> Either<A, B> second(B b) {
        assert b != null;
        return new Either<>(null, b);
    }

    /**
     * Constructs a Maybe object with no value (similar to java.util.Optional.empty()).
     */
    public static <T> Maybe<T> none() {
        return new Maybe<>(second(unit()));
    }

    /**
     * Constructs a Maybe object with a value (similar to java.util.Optional.of()).
     */
    public static <T> Maybe<T> some(T value) {
        assert value != null;
        return new Maybe<>(first(value));
    }

    /**
     * Constructs a Maybe object with a nullable value. If `value` is null, returns none().
     */
    public static <T> Maybe<T> wrapMaybe(T value) {
        return value != null ? some(value) : none();
    }
}
