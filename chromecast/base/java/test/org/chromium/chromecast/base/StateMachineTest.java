// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chromecast.base;

import static org.hamcrest.Matchers.contains;
import static org.hamcrest.Matchers.emptyIterable;
import static org.junit.Assert.assertThat;

import org.junit.Test;
import org.junit.runner.RunWith;
import org.junit.runners.BlockJUnit4ClassRunner;

import org.chromium.chromecast.base.CompositeTypes.Maybe;
import org.chromium.chromecast.base.StateMachine.Controller;
import org.chromium.chromecast.base.StateMachine.ScopeFactory;
import org.chromium.chromecast.base.StateMachine.State;

import java.util.ArrayList;
import java.util.List;

/**
 * Tests for StateMachine.
 */
@RunWith(BlockJUnit4ClassRunner.class)
public class StateMachineTest {
    // Convenience method to create a scope that mutates a list of strings on state transitions.
    // When entering the state, it will append "enter ${id} ${data}" to the result list, where
    // `data` is the String that is associated with the state activation. When exiting the state,
    // it will append "exit ${id}" to the result list. This provides a readable way to track and
    // verify the behavior of States in response to the Observables they are linked to.
    private static <T> ScopeFactory<T> report(List<String> result, String id) {
        // Did you know that lambdas are awesome.
        return (T data) -> {
            result.add("enter " + id + ": " + data);
            return () -> result.add("exit " + id);
        };
    }

    @Test
    public void testNoStateTransitionAfterRegisteringWithInactiveController() {
        Controller<String> controller = new Controller<>();
        List<String> result = new ArrayList<>();
        controller.watch(report(result, "a"));
        assertThat(result, emptyIterable());
    }

    @Test
    public void testStateIsEnteredWhenControllerIsSet() {
        Controller<String> controller = new Controller<>();
        List<String> result = new ArrayList<>();
        controller.watch(report(result, "a"));
        // Activate the state by setting the controller.
        controller.set("cool");
        assertThat(result, contains("enter a: cool"));
    }

    @Test
    public void testBasicStateFromController() {
        Controller<String> controller = new Controller<>();
        List<String> result = new ArrayList<>();
        controller.watch(report(result, "a"));
        controller.set("fun");
        // Deactivate the state by resetting the controller.
        controller.reset();
        assertThat(result, contains("enter a: fun", "exit a"));
    }

    @Test
    public void testSetStateTwicePerformsImplicitReset() {
        Controller<String> controller = new Controller<>();
        List<String> result = new ArrayList<>();
        controller.watch(report(result, "a"));
        // Activate the state for the first time.
        controller.set("first");
        // Activate the state for the second time.
        controller.set("second");
        // If set() is called without a reset() in-between, the tracking state exits, then re-enters
        // with the new data. So we expect to find an "exit" call between the two enter calls.
        assertThat(result, contains("enter a: first", "exit a", "enter a: second"));
    }

    @Test
    public void testResetWhileStateIsNotEnteredIsNoOp() {
        Controller<String> controller = new Controller<>();
        List<String> result = new ArrayList<>();
        controller.watch(report(result, "a"));
        controller.reset();
        assertThat(result, emptyIterable());
    }

    @Test
    public void testStateObservingState() {
        // Construct a State that watches another State. Verify both States' events are triggered.
        Controller<String> controller = new Controller<>();
        List<String> result = new ArrayList<>();
        State<String> state = controller.watch(report(result, "a"));
        State<String> metastate = state.watch(report(result, "b"));
        // Activate the controller, which should propagate a state transition to both states.
        // Both states should be updated, so we should get two enter events.
        controller.set("groovy");
        controller.reset();
        // Note that exit handlers are done in the reverse order that they were registered.
        // In other words, the last State to get activated is the first State to get deactivated.
        assertThat(result, contains("enter a: groovy", "enter b: groovy", "exit b", "exit a"));
    }

    @Test
    public void testMultipleStatesObservingSingleController() {
        // Construct two states that watch the same Controller. Verify both States' events are
        // triggered.
        Controller<String> controller = new Controller<>();
        List<String> result = new ArrayList<>();
        State<String> a = controller.watch(report(result, "a"));
        State<String> b = controller.watch(report(result, "b"));
        // Activate the controller, which should propagate a state transition to both states.
        // Both states should be updated, so we should get two enter events.
        controller.set("neat");
        controller.reset();
        assertThat(result, contains("enter a: neat", "enter b: neat", "exit b", "exit a"));
    }

    @Test
    public void testNewStateIsActivatedImmediatelyIfObservingAlreadyActiveObservable() {
        Controller<String> controller = new Controller<>();
        List<String> result = new ArrayList<>();
        controller.set("surprise");
        controller.watch(report(result, "a"));
        assertThat(result, contains("enter a: surprise"));
    }

    @Test
    public void testNewStateIsNotActivatedIfObservingObservableThatHasBeenDeactivated() {
        Controller<String> controller = new Controller<>();
        List<String> result = new ArrayList<>();
        controller.set("surprise");
        controller.reset();
        controller.watch(report(result, "a"));
        assertThat(result, emptyIterable());
    }

    @Test
    public void testResetWhileAlreadyDeactivatedIsANoOp() {
        Controller<String> controller = new Controller<>();
        List<String> result = new ArrayList<>();
        controller.watch(report(result, "a"));
        controller.set("radical");
        controller.reset();
        // Resetting again after already resetting should not notify the State.
        controller.reset();
        assertThat(result, contains("enter a: radical", "exit a"));
    }

    @Test
    public void testClosedStateNoLongerReceivesUpdates() {
        Controller<String> controller = new Controller<>();
        List<String> result = new ArrayList<>();
        controller.watch(report(result, "a")).close();
        controller.set("beware");
        controller.reset();
        assertThat(result, emptyIterable());
    }

    @Test
    public void testClosedStateDoesNotNotifyObservers() {
        Controller<String> controller = new Controller<>();
        List<String> result = new ArrayList<>();
        State<String> closed = controller.watch(report(result, "closed"));
        State<String> watchClosed = closed.watch(report(result, "unexpected"));
        closed.close();
        controller.set("look out");
        assertThat(result, emptyIterable());
    }

    @Test
    public void testCloseStateWhileActivatedImplicitlyDeactivatesState() {
        Controller<String> controller = new Controller<>();
        List<String> result = new ArrayList<>();
        State<String> state = controller.watch(report(result, "state"));
        controller.set("boom");
        state.close();
        assertThat(result, contains("enter state: boom", "exit state"));
    }

    @Test
    public void testCloseControllerImplicitlyDeactivatesListeningStates() {
        Controller<String> controller = new Controller<>();
        List<String> result = new ArrayList<>();
        controller.watch(report(result, "a"));
        controller.watch(report(result, "b"));
        controller.set("yes");
        controller.close();
        assertThat(result, contains("enter a: yes", "enter b: yes", "exit b", "exit a"));
    }

    @Test(expected = IllegalStateException.class)
    public void testCannotSetClosedController() {
        Controller<String> controller = new Controller<>();
        controller.close();
        controller.set("no");
    }

    @Test(expected = IllegalStateException.class)
    public void testCannotResetClosedController() {
        Controller<String> controller = new Controller<>();
        controller.close();
        controller.reset();
    }

    @Test
    public void testBothState_activateFirstDoesNotTrigger() {
        Controller<String> a = new Controller<>();
        Controller<String> b = new Controller<>();
        List<String> result = new ArrayList<>();
        a.and(b).watch(report(result, "both"));
        a.set("A");
        assertThat(result, emptyIterable());
    }

    @Test
    public void testBothState_activateSecondDoesNotTrigger() {
        Controller<String> a = new Controller<>();
        Controller<String> b = new Controller<>();
        List<String> result = new ArrayList<>();
        a.and(b).watch(report(result, "both"));
        b.set("B");
        assertThat(result, emptyIterable());
    }

    @Test
    public void testBothState_activateBothTriggers() {
        Controller<String> a = new Controller<>();
        Controller<String> b = new Controller<>();
        List<String> result = new ArrayList<>();
        a.and(b).watch(report(result, "both"));
        a.set("A");
        b.set("B");
        assertThat(result, contains("enter both: A, B"));
    }

    @Test
    public void testBothState_deactivateFirstAfterTrigger() {
        Controller<String> a = new Controller<>();
        Controller<String> b = new Controller<>();
        List<String> result = new ArrayList<>();
        a.and(b).watch(report(result, "both"));
        a.set("A");
        b.set("B");
        a.reset();
        assertThat(result, contains("enter both: A, B", "exit both"));
    }

    @Test
    public void testBothState_deactivateSecondAfterTrigger() {
        Controller<String> a = new Controller<>();
        Controller<String> b = new Controller<>();
        List<String> result = new ArrayList<>();
        a.and(b).watch(report(result, "both"));
        a.set("A");
        b.set("B");
        b.reset();
        assertThat(result, contains("enter both: A, B", "exit both"));
    }

    @Test
    public void testBothState_resetFirstBeforeSettingSecond_doesNotTrigger() {
        Controller<String> a = new Controller<>();
        Controller<String> b = new Controller<>();
        List<String> result = new ArrayList<>();
        a.and(b).watch(report(result, "both"));
        a.set("A");
        a.reset();
        b.set("B");
        assertThat(result, emptyIterable());
    }

    @Test
    public void testBothState_resetSecondBeforeSettingFirst_doesNotTrigger() {
        Controller<String> a = new Controller<>();
        Controller<String> b = new Controller<>();
        List<String> result = new ArrayList<>();
        a.and(b).watch(report(result, "both"));
        b.set("B");
        b.reset();
        a.set("A");
        assertThat(result, emptyIterable());
    }

    @Test
    public void testBothState_setOneControllerAfterTrigger_implicitlyResetsAndSets() {
        Controller<String> a = new Controller<>();
        Controller<String> b = new Controller<>();
        List<String> result = new ArrayList<>();
        a.and(b).watch(report(result, "both"));
        a.set("A1");
        b.set("B1");
        a.set("A2");
        b.set("B2");
        assertThat(result,
                contains("enter both: A1, B1", "exit both", "enter both: A2, B1", "exit both",
                        "enter both: A2, B2"));
    }

    @Test
    public void testEitherState_isNotActivatedRightAway() {
        Controller<String> a = new Controller<>();
        Controller<String> b = new Controller<>();
        List<String> result = new ArrayList<>();
        a.or(b).watch(report(result, "either"));
        assertThat(result, emptyIterable());
    }

    @Test
    public void testEitherState_isActivatedWhenFirstIsSet() {
        Controller<String> a = new Controller<>();
        Controller<String> b = new Controller<>();
        List<String> result = new ArrayList<>();
        a.or(b).watch(report(result, "either"));
        a.set("a");
        assertThat(result, contains("enter either: a"));
    }

    @Test
    public void testEitherState_isActivatedWhenSecondIsSet() {
        Controller<String> a = new Controller<>();
        Controller<String> b = new Controller<>();
        List<String> result = new ArrayList<>();
        a.or(b).watch(report(result, "either"));
        b.set("b");
        assertThat(result, contains("enter either: b"));
    }

    @Test
    public void testEitherState_isDeactivatedAndReactivatedWithOtherStateActivates() {
        Controller<String> a = new Controller<>();
        Controller<String> b = new Controller<>();
        List<String> result = new ArrayList<>();
        a.or(b).watch(report(result, "either"));
        a.set("A");
        b.set("B");
        assertThat(result, contains("enter either: A", "exit either", "enter either: B"));
    }

    @Test
    public void testNotState_isActivatedImmediatelyWhenRegisteredToDeactivatedState() {
        Controller<String> a = new Controller<>();
        List<String> result = new ArrayList<>();
        StateMachine.not(a).watch(report(result, "not"));
        assertThat(result, contains("enter not: ()"));
    }

    @Test
    public void testNotState_isNotActivatedImmediatelyWhenRegisteredToActivatedState() {
        Controller<String> a = new Controller<>();
        List<String> result = new ArrayList<>();
        a.set("quick");
        StateMachine.not(a).watch(report(result, "not"));
        assertThat(result, emptyIterable());
    }

    @Test
    public void testNotState_isActivatedWhenObservingStateDeactivates() {
        Controller<String> a = new Controller<>();
        List<String> result = new ArrayList<>();
        a.set("fast");
        StateMachine.not(a).watch(report(result, "not"));
        a.reset();
        assertThat(result, contains("enter not: ()"));
    }

    @Test
    public void testNotState_isDeactivatedWhenObservingStateActivates() {
        Controller<String> a = new Controller<>();
        List<String> result = new ArrayList<>();
        StateMachine.not(a).watch(report(result, "not"));
        a.set("now");
        a.reset();
        assertThat(result, contains("enter not: ()", "exit not", "enter not: ()"));
    }

    @Test
    public void testOnActivated() {
        Controller<String> a = new Controller<>();
        List<String> result = new ArrayList<>();
        a.onActivated().watch(report(result, "onActivated"));
        a.set("one");
        a.reset();
        assertThat(result, contains("enter onActivated: one"));
        result.clear();
        a.set("two");
        assertThat(result, contains("exit onActivated", "enter onActivated: two"));
    }

    @Test
    public void testOnDeactivated() {
        Controller<String> a = new Controller<>();
        List<String> result = new ArrayList<>();
        a.onDeactivated().watch(report(result, "onDeactivated"));
        a.set("first");
        assertThat(result, emptyIterable());
        a.reset();
        assertThat(result, contains("enter onDeactivated: first"));
        result.clear();
        a.set("second");
        assertThat(result, emptyIterable());
        a.set("third");
        assertThat(result, contains("exit onDeactivated", "enter onDeactivated: second"));
    }

    @Test
    public void testComposeBoth() {
        Controller<String> a = new Controller<>();
        Controller<String> b = new Controller<>();
        Controller<String> c = new Controller<>();
        Controller<String> d = new Controller<>();
        List<String> result = new ArrayList<>();
        a.and(b).and(c).and(d).watch(report(result, "all four"));
        a.set("A");
        b.set("B");
        c.set("C");
        d.set("D");
        a.reset();
        assertThat(result, contains("enter all four: A, B, C, D", "exit all four"));
    }

    @Test
    public void testPowerUnlimitedPower() {
        Controller<String> a = new Controller<>();
        Controller<String> b = new Controller<>();
        Controller<String> c = new Controller<>();
        Controller<String> d = new Controller<>();
        List<String> result = new ArrayList<>();
        (a.and(b)).or(c.and(d)).watch(report(result, "pair"));
        a.set("a");
        b.set("b");
        c.set("c");
        d.set("d");
        a.set("A");
        d.set("D");
        a.reset();
        c.reset();
        assertThat(result,
                contains("enter pair: a, b", "exit pair", "enter pair: c, d", "exit pair",
                        "enter pair: A, b", "exit pair", "enter pair: c, D", "exit pair"));
    }

    @Test
    public void testOnlyIf() {
        Controller<String> a = new Controller<>();
        List<String> result = new ArrayList<>();
        a.watch(report(result, "anyString"))
                .onlyIf((String s) -> s.length() >= 3)
                .watch(report(result, "atLeastThree"));
        a.set("a");
        a.set("ab");
        a.set("abc");
        a.set("abcd");
        assertThat(result,
                contains("enter anyString: a", "exit anyString", "enter anyString: ab",
                        "exit anyString", "enter anyString: abc", "enter atLeastThree: abc",
                        "exit atLeastThree", "exit anyString", "enter anyString: abcd",
                        "enter atLeastThree: abcd"));
    }

    @Test
    public void testTransform() {
        Controller<String> a = new Controller<>();
        List<String> result = new ArrayList<>();
        a.watch(report(result, "unchanged"))
                .transform(String::toLowerCase)
                .watch(report(result, "lower"))
                .transform(String::toUpperCase)
                .watch(report(result, "upper"));
        a.set("sImPlY sTeAmEd KaLe");
        assertThat(result,
                contains("enter unchanged: sImPlY sTeAmEd KaLe", "enter lower: simply steamed kale",
                        "enter upper: SIMPLY STEAMED KALE"));
    }

    @Test
    public void testMaybe() {
        Controller<String> a = new Controller<>();
        List<String> result = new ArrayList<>();
        a.watch(report(result, "unchanged"))
                .maybe((String s) -> {
                    if (s.startsWith("a")) {
                        return CompositeTypes.some(s + s);
                    }
                    return CompositeTypes.none();
                })
                .watch(report(result, "startsWithAAndDoubled"));
        a.set("blah");
        a.set("abracadabra");
        assertThat(result,
                contains("enter unchanged: blah", "exit unchanged", "enter unchanged: abracadabra",
                        "enter startsWithAAndDoubled: abracadabraabracadabra"));
    }

    @Test(expected = IllegalArgumentException.class)
    public void testSetControllerWithNullThrowsError() {
        new Controller<String>().set(null);
    }

    @Test
    public void testIfNotNoneWrapper() {
        Controller<Maybe<String>> a = new Controller<>();
        List<String> result = new ArrayList<>();
        StateMachine.ifNotNone(a.watch(report(result, "acceptsNone")))
                .watch(report(result, "notNone"));
        a.set(CompositeTypes.wrapMaybe(null));
        a.set(CompositeTypes.wrapMaybe("not null"));
        a.set(CompositeTypes.wrapMaybe(null));
        assertThat(result,
                contains("enter acceptsNone: ()", "exit acceptsNone", "enter acceptsNone: not null",
                        "enter notNone: not null", "exit notNone", "exit acceptsNone",
                        "enter acceptsNone: ()"));
    }

    @Test
    public void testResetControllerInActivationHandler() {
        Controller<String> a = new Controller<>();
        List<String> result = new ArrayList<>();
        a.watch((String s) -> {
            result.add("enter " + s);
            a.reset();
            result.add("after reset");
            return () -> {
                result.add("exit");
            };
        });
        a.set("immediately retracted");
        assertThat(result, contains("enter immediately retracted", "after reset", "exit"));
    }

    @Test
    public void testSetControllerInActivationHandler() {
        Controller<String> a = new Controller<>();
        List<String> result = new ArrayList<>();
        a.watch(report(result, "weirdness")).watch((String s) -> {
            // If the activation handler always calls set() on the source controller, you will have
            // an infinite loop, which is not cool. However, if the activation handler only
            // conditionally calls set() on its source controller, then the case where set() is not
            // called will break the loop. It is the responsibility of the programmer to solve the
            // halting problem for activation handlers.
            if (s.equals("first")) {
                a.set("second");
            }
            return () -> {
                result.add("haha");
            };
        });
        a.set("first");
        assertThat(result,
                contains("enter weirdness: first", "haha", "exit weirdness",
                        "enter weirdness: second"));
    }

    @Test
    public void testResetControllerInDeactivationHandler() {
        Controller<String> a = new Controller<>();
        List<String> result = new ArrayList<>();
        a.watch(report(result, "bizzareness")).watch(StateMachine.onExit((String s) -> a.reset()));
        a.set("yo");
        a.reset();
        // The reset() called by the deactivation handler should be a no-op.
        assertThat(result, contains("enter bizzareness: yo", "exit bizzareness"));
    }

    @Test
    public void testSetControllerInDeactivationHandler() {
        Controller<String> a = new Controller<>();
        List<String> result = new ArrayList<>();
        a.watch(report(result, "astoundingness"))
                .watch(StateMachine.onExit((String s) -> a.set("never mind")));
        a.set("retract this");
        a.reset();
        // The set() called by the deactivation handler should immediately set the controller back.
        assertThat(result,
                contains("enter astoundingness: retract this", "exit astoundingness",
                        "enter astoundingness: never mind"));
    }

    @Test
    public void testOnEnter() {
        Controller<String> a = new Controller<>();
        List<String> result = new ArrayList<>();
        // onEnter() is shorthand for creating a ScopeFactory with no side effects when exiting.
        a.watch(StateMachine.onEnter((String s) -> result.add("enter " + s)));
        a.set("a");
        a.set("b");
        a.set("c");
        assertThat(result, contains("enter a", "enter b", "enter c"));
    }

    @Test
    public void testOnExit() {
        Controller<String> a = new Controller<>();
        List<String> result = new ArrayList<>();
        // onExit() is shorthand for creating a ScopeFactory with no side effects when entering.
        a.watch(StateMachine.onExit((String s) -> result.add("exit " + s)));
        a.set("a");
        a.set("b");
        a.set("c");
        assertThat(result, contains("exit a", "exit b"));
    }
}
