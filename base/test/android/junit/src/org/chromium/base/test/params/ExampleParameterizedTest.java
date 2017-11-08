// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.base.test.params;

import static org.chromium.base.test.params.ParameterAnnotations.MethodParameter;
import static org.chromium.base.test.params.ParameterAnnotations.UseMethodParameter;
import static org.chromium.base.test.params.ParameterAnnotations.UseParameterGenerator;

import org.junit.Assert;
import org.junit.Test;
import org.junit.runner.RunWith;

import org.chromium.base.test.params.ParameterAnnotations.ClassParameter;
import org.chromium.base.test.params.ParameterAnnotations.UseRunnerDelegate;

import java.util.Arrays;
import java.util.List;

/**
 * Example test that uses ParamRunner
 */
@RunWith(ParameterizedRunner.class)
@UseRunnerDelegate(BlockJUnit4RunnerDelegate.class)
public class ExampleParameterizedTest {
    @ClassParameter
    private static List<ParameterSet> sClassParams =
            Arrays.asList(new ParameterSet().value("hello", "world").name("HelloWorld"),
                    new ParameterSet().value("Xxxx", "Yyyy").name("XxxxYyyy"),
                    new ParameterSet().value("aa", "yy").name("AaYy"));

    @MethodParameter("A")
    private static List<ParameterSet> sMethodParamA =
            Arrays.asList(new ParameterSet().value(1, 2).name("OneTwo"),
                    new ParameterSet().value(2, 3).name("TwoThree"),
                    new ParameterSet().value(3, 4).name("ThreeFour"));

    @MethodParameter("B")
    private static List<ParameterSet> sMethodParamB =
            Arrays.asList(new ParameterSet().value("a", "b").name("Ab"),
                    new ParameterSet().value("b", "c").name("Bc"),
                    new ParameterSet().value("c", "d").name("Cd"),
                    new ParameterSet().value("d", "e").name("De"));

    private static class MethodParamsC implements ParameterGenerator {
        @Override
        public List<ParameterSet> getParams() {
            return Arrays.asList(new ParameterSet().value(1, 2).name("OneTwo"),
                    new ParameterSet().value(2, 3).name("TwoThree"),
                    new ParameterSet().value(3, 4).name("ThreeFour"));
        }
    }

    private String mStringA;
    private String mStringB;

    public ExampleParameterizedTest(String a, String b) {
        mStringA = a;
        mStringB = b;
    }

    @Test
    public void testSimple() {
        Assert.assertEquals(
                "A and B string length aren't equal", mStringA.length(), mStringB.length());
    }

    @Test
    @UseMethodParameter("A")
    public void testWithOnlyA(int intA, int intB) {
        Assert.assertTrue(intA + 1 == intB);
    }

    @Test
    @UseMethodParameter("B")
    public void testWithOnlyB(String a, String b) {
        Assert.assertTrue(!a.equals(b));
    }

    @Test
    @UseParameterGenerator(MethodParamsC.class)
    public void testWithOnlyC(int intA, int intB) {
        Assert.assertTrue(intA + 1 == intB);
    }
}
