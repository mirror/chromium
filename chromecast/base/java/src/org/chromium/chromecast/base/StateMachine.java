// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chromecast.base;

import org.chromium.base.Log;
import org.chromium.base.ObserverList;
import org.chromium.chromecast.base.CompositeTypes.Both;
import org.chromium.chromecast.base.CompositeTypes.Either;
import org.chromium.chromecast.base.CompositeTypes.Maybe;
import org.chromium.chromecast.base.CompositeTypes.Transform;

/**
 * Provides simple composable primitives for monitoring states.
 *
 * The Android framework commonly makes clients register listeners for pairs of events, where one
 * event represents entering some state, and the other event represents exiting that state. When
 * these events accumulate, it can be difficult to manage all the states and often requires a lot of
 * null checks or other workarounds to get the states to properly interact. This class aims to solve
 * the problem by providing simple wrapper classes that can easily adapt these event pairs to
 * functional strategies with compiler-enforceable type safety.
 *
 * Compare to RAII in C++: just as objects in C++ have a constructor and destructor, which are
 * implicitly run before and after the object is in scope, respectively, States in this framework
 * are associated with an interface that runs some side effects (like a C++ constructor) and returns
 * a handle to an AutoCloseable interface that runs other side effects (like a C++ destructor).
 * The constructor/destructor function pair is called a "scope", and the interface that constructs
 * them is called ScopeFactory.
 *
 * Traditional events, like an Activity's onStart() and onStop(), for example, can be adapted to
 * States using a Controller object. In the Activity example, one can set() the state in onStart()
 * and reset() the state in onStop(). Any State created from that Controller will have the proper
 * lifetime for the scopes associated with it.
 *
 * Example:
 *
 *  import static org.chromium.chromecast.base.CompositeTypes.unit;
 *  import org.chromium.chromecast.base.CompositeTypes.Unit;
 *  import org.chromium.chromecast.base.StateMachine.Controller;
 *  class MyActivity extends Activity {
 *      private final Controller<Unit> mController = new Controller<>();
 *      public void onCreate(...) {
 *          mController.watch((Unit unused) -> {
 *              Log.d(TAG, "onStart");
 *              return () -> Log.d(TAG, "onStop");
 *          });
 *      }
 *      public void onStart() {
 *          mController.set(unit());
 *      }
 *      public void onStop() {
 *          mController.reset();
 *      }
 *  }
 *
 * The above example uses the Unit type in CompositeTypes as a way to circumvent the requirement
 * that Controllers and States have some associated type. For simple events where we only care about
 * the state itself, and not any data associated with it, use the Unit type.
 *
 * Importantly, States are composable in several different ways. Two independent States (or
 * Controllers) can be combined with State.both() to create a new State that activates only when
 * both of the original states are activated. Two states can similarly be combined with
 * State.either() to create a new State that activates whenever either of the original states are
 * activated. In addition, an arbitrary number of States can monitor a single State or Controller.
 * With these simple composable tools, complex behaviors can be created while avoiding nasty null
 * checks, maybe*() or *IfNecessary() helper methods, and other hacks that make it difficult to
 * reason about the code.
 */
public class StateMachine {
    private static final String TAG = "StateMachine";
    /**
     * Simple interface representing the actions to perform when entering and exiting a State.
     *
     * The create() implementation is called when entering the State, and the AutoCloseable that it
     * returns is invoked when leaving the State. The side-effects are like a constructor, and the
     * side-effects of the AutoCloseable are like a destructor.
     *
     * @param <T> The argument type that the constructor takes.
     * @param <C> A type that implements AutoCloseable, to be closed when the state exits.
     */
    public interface ScopeFactory<T, C extends AutoCloseable> { public C create(T data); }

    /**
     * Interface for Observable state. This can be a State or a Controller.
     *
     * Observables can have some data associated with them, which is provided to observers (States)
     * when the state activates. The <T> parameter determines the type of this data.
     *
     * Only this class has access to addObserver() and removeObserver(). Clients should use States
     * to track the life cycle of Observables.
     *
     * @param <T> The type of the state data.
     */
    public static abstract class Observable<T> {
        protected abstract void addObserver(StateObserver<T> observer);
        protected abstract void removeObserver(StateObserver<T> observer);

        /**
         * Creates a State that tracks the given observable with the given scope factory.
         *
         * When the State is entered, the factory will be invoked to produce a scope. When the State
         * is exited, that scope will have its close() method invoked. In this way, one can define
         * state transitions from the ScopeFactory and its return value's close() method.
         */
        public final State<T> watch(ScopeFactory<T, ?> factory) {
            return new State<>(this, factory);
        }

        /**
         * Creates an Observable that activates listening States only if both `this` and `other` are
         * activated, and deactivates listening States if either of `this` or `other` are
         * deactivated.
         *
         * This is useful for creating an event handler that should only activate when two events
         * have occurred, but those events may occur in any order.
         *
         * This is composable (returns an Observable), so one can use this to create a State that
         * observes the intersection of arbitrarily many Observables.
         */
        public final <U> Observable<Both<T, U>> and(Observable<U> other) {
            return new PendingBothState<>(this, other).asObservable();
        }

        /**
         * Creates an Observable that is activated if either of `this` or `other` are activated.
         *
         * If this is already activated by one of the Observables, and the other Observable is
         * then activated, then this is deactivated and reactivated with the second Observable's
         * data. This way, only one scope is created at a time: the scope for the most recently
         * activated Observable.
         *
         * This is useful for creating an event handler that only cares about the most recent of two
         * events, which may occur in any order.
         *
         * This is composable (returns an Observable), so one can use this to create a State that
         * observes the union of arbitrarily many Observables.
         */
        public final <U> Observable<Either<T, U>> or(Observable<U> other) {
            return new PendingEitherState<>(this, other).asObservable();
        }

        /**
         * Creates an Observable that is activated if `this` is activated AND the predicate returns
         * true for the activation value.
         *
         * This can be used to create states that respond to a subset of activations of an existing
         * state.
         */
        public final Observable<T> onlyIf(Predicate<T> predicate) {
            Controller<T> controller = new Controller<>();
            watch((T value) -> {
                if (predicate.test(value)) {
                    controller.set(value);
                }
                return () -> controller.reset();
            });
            return controller;
        }

        /**
         * Used for the onlyIf() method.
         *
         * TODO(sanfin): Replace with java.util.function.Predicate if we're ever allowed to use the
         * Java 8 library.
         *
         * @param <T> The argument type.
         */
        public interface Predicate<T> { public boolean test(T t); }

        /**
         * Returns an Observable that applies the given Transform to this Observable's activation
         * values.
         */
        public final <R> Observable<R> transform(Transform<T, R> transform) {
            Controller<R> controller = new Controller<>();
            watch((T value) -> {
                controller.set(transform.transform(value));
                return () -> controller.reset();
            });
            return controller;
        }
    }

    private static abstract class StateObserver<T> extends Observable<T> {
        protected abstract void onEnter(T data);
        protected abstract void onExit();
    }

    /**
     * Associates a ScopeFactory with the Observable.
     *
     * The Observable tells the constructed State when the state is activated and deactivated.
     * When activated, the ScopeFactory will be called to construct a scope object, whose close()
     * method will be called when the state is deactivated.
     *
     * Since State itself implements Observable, States can be constructed from other States
     * in the same way that they can be constructed from Controllers. This allows creating a linked
     * chain of States all tracking the same controller.
     *
     * @param <T> The type of the state data.
     */
    public static class State<T> extends StateObserver<T> implements AutoCloseable {
        private final ObserverList<StateObserver<T>> mObservers;
        private final Observable<T> mObservable;
        private final ScopeFactory<T, ? extends AutoCloseable> mFactory;
        private Maybe<Both<T, AutoCloseable>> mState;

        private State(Observable<T> observable, ScopeFactory<T, ? extends AutoCloseable> factory) {
            super();
            mObservers = new ObserverList<>();
            mObservable = observable;
            mFactory = factory;
            mState = CompositeTypes.none();
            mObservable.addObserver(this);
        }

        @Override
        protected void addObserver(StateObserver<T> observer) {
            mObservers.addObserver(observer);
            mState.ifPresent((Both<T, AutoCloseable> state) -> observer.onEnter(state.first));
        }

        @Override
        protected void removeObserver(StateObserver<T> observer) {
            mObservers.removeObserver(observer);
        }

        @Override
        protected void onEnter(T data) {
            mState = CompositeTypes.some(CompositeTypes.both(data, mFactory.create(data)));
            for (StateObserver<T> observer : mObservers) {
                observer.onEnter(data);
            }
        }

        @Override
        protected void onExit() {
            for (StateObserver<T> observer : Itertools.reverse(mObservers)) {
                observer.onExit();
            }
            mState.ifPresent((Both<T, AutoCloseable> state) -> {
                try {
                    state.second.close();
                } catch (Exception e) {
                    Log.e(TAG, "Exception closing State scope", e);
                }
                mState = CompositeTypes.none();
            });
        }

        /**
         * Deactivates if activated, and stops receiving further updates from the Observable that
         * this State is watching.
         */
        @Override
        public void close() {
            mObservable.removeObserver(this);
            onExit();
        }
    }

    /**
     * An Observable with public mutators that can control the state that it represents.
     *
     * The two mutators are set() and reset(). The set() method sets the state, and can inject
     * arbitrary data of the parameterized type, which will be forwarded to any States, and by
     * extension, their scopes, observing this Controller. The reset() method deactivates the state.
     *
     * This class also implements AutoCloseable, so that it can implicitly call reset() at the end
     * of a try-with-resources statement. Calling reset() while the state is deactivated is a no-op.
     *
     * @param <T> The type of the state data.
     */
    public static class Controller<T> extends Observable<T> implements AutoCloseable {
        private final ObserverList<StateObserver<T>> mObservers;
        private Maybe<T> mData;
        private boolean mClosed;

        public Controller() {
            mObservers = new ObserverList<>();
            mData = CompositeTypes.none();
            mClosed = false;
        }

        @Override
        protected void addObserver(StateObserver<T> observer) {
            mObservers.addObserver(observer);
            mData.ifPresent((T data) -> observer.onEnter(data));
        }

        @Override
        protected void removeObserver(StateObserver<T> observer) {
            mObservers.removeObserver(observer);
        }

        /**
         * Activates all States observing this Controller.
         *
         * If this controller is already set(), an implicit reset() is called. This allows States'
         * scopes to properly clean themselves up before the scope for the new activation is
         * created.
         * Must not be called after close().
         */
        public void set(T data) {
            if (mClosed) throw new IllegalStateException("Cannot set() a closed Controller");
            if (mData.isPresent()) {
                reset();
            }
            mData = CompositeTypes.some(data);
            for (StateObserver<T> observer : mObservers) {
                observer.onEnter(data);
            }
        }

        /**
         * Deactivates all States observing this Controller.
         *
         * If this Controller is already reset(), this is a no-op.
         * Must not be called after close().
         */
        public void reset() {
            if (mClosed) throw new IllegalStateException("Cannot reset() a closed Controller");
            if (!mData.isPresent()) return;
            mData = CompositeTypes.none();
            for (StateObserver<T> observer : Itertools.reverse(mObservers)) {
                observer.onExit();
            }
        }

        /**
         * Implementation for AutoCloseable. Resets the Controller if set.
         *
         * This method should not be called more than once. It is not necessary to call this if
         * reset() is known to be called at the right time.
         */
        @Override
        public void close() {
            if (mClosed) {
                throw new IllegalStateException("Cannot close() a Controller twice");
            }
            reset();
            mObservers.clear();
            mClosed = true;
        }
    }

    // Owns a Controller that is activated only when both Observables are activated.
    private static class PendingBothState<A, B> {
        private final Controller<Both<A, B>> mController;
        private Maybe<A> mA;
        private Maybe<B> mB;
        private Maybe<Both<A, B>> mBoth;

        private PendingBothState(Observable<A> observingA, Observable<B> observingB) {
            mController = new Controller<>();
            mA = CompositeTypes.none();
            mB = CompositeTypes.none();
            mBoth = CompositeTypes.none();
            observingA.watch((A a) -> {
                mB.ifPresent((B b) -> setBoth(a, b));
                mA = CompositeTypes.some(a);
                return () -> {
                    mA = CompositeTypes.none();
                    resetBoth();
                };
            });
            observingB.watch((B b) -> {
                mA.ifPresent((A a) -> setBoth(a, b));
                mB = CompositeTypes.some(b);
                return () -> {
                    mB = CompositeTypes.none();
                    resetBoth();
                };
            });
        }

        private void setBoth(A a, B b) {
            Both<A, B> both = CompositeTypes.both(a, b);
            mBoth = CompositeTypes.some(both);
            mController.set(both);
        }

        private void resetBoth() {
            if (mBoth.isPresent()) {
                mController.reset();
                mBoth = CompositeTypes.none();
            }
        }

        private Observable<Both<A, B>> asObservable() {
            return mController;
        }
    }

    // Owns a Controller that is activated when either Observable is activated.
    private static class PendingEitherState<A, B> {
        private final Controller<Either<A, B>> mController;
        private Maybe<A> mA;
        private Maybe<B> mB;

        private PendingEitherState(Observable<A> observingA, Observable<B> observingB) {
            mController = new Controller<>();
            mA = CompositeTypes.none();
            mB = CompositeTypes.none();
            observingA.watch((A a) -> {
                mA = CompositeTypes.some(a);
                mController.set(CompositeTypes.first(a));
                return () -> {
                    mA = CompositeTypes.none();
                    if (!mB.isPresent()) {
                        mController.reset();
                    }
                };
            });
            observingB.watch((B b) -> {
                mB = CompositeTypes.some(b);
                mController.set(CompositeTypes.second(b));
                return () -> {
                    mB = CompositeTypes.none();
                    if (!mA.isPresent()) {
                        mController.reset();
                    }
                };
            });
        }

        private Observable<Either<A, B>> asObservable() {
            return mController;
        }
    }
}
