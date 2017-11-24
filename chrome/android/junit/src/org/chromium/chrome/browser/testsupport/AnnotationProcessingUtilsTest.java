// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.testsupport;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertNotNull;
import static org.junit.Assert.assertNull;
import static org.junit.Assert.assertTrue;
import static org.junit.Assert.fail;
import static org.junit.runner.Description.createTestDescription;

import org.junit.Ignore;
import org.junit.Rule;
import org.junit.Test;
import org.junit.rules.TestRule;
import org.junit.runner.Description;
import org.junit.runner.RunWith;
import org.junit.runners.BlockJUnit4ClassRunner;
import org.junit.runners.model.FrameworkMethod;
import org.junit.runners.model.InitializationError;
import org.junit.runners.model.Statement;

import org.chromium.chrome.test.util.browser.AnnotationProcessingUtils;

import java.lang.annotation.ElementType;
import java.lang.annotation.Retention;
import java.lang.annotation.RetentionPolicy;
import java.lang.annotation.Target;
import java.util.List;

@RunWith(BlockJUnit4ClassRunner.class)
public class AnnotationProcessingUtilsTest {
    @Test
    public void testGetSimpleAnnotation_NotOnClassNorMethod() {
        SimpleAnnotation retrievedAnnotation;

        retrievedAnnotation = AnnotationProcessingUtils.getAnnotation(
                createTestDescription(ClassWithoutAnnotation.class, "methodWithoutAnnotation"),
                SimpleAnnotation.class);
        assertNull(retrievedAnnotation);
    }

    @Test
    public void testGetSimpleAnnotation_NotOnClassButOnMethod() {
        SimpleAnnotation retrievedAnnotation;

        retrievedAnnotation = AnnotationProcessingUtils.getAnnotation(
                getTest(ClassWithoutAnnotation.class, "methodWithSimpleAnnotation"),
                SimpleAnnotation.class);
        assertNotNull(retrievedAnnotation);
    }

    @Test
    public void testGetSimpleAnnotation_NotOnClassDifferentOneOnMethod() {
        SimpleAnnotation retrievedAnnotation;

        retrievedAnnotation = AnnotationProcessingUtils.getAnnotation(
                getTest(ClassWithoutAnnotation.class, "methodWithAnnotatedAnnotation"),
                SimpleAnnotation.class);
        assertNull(retrievedAnnotation);
    }

    @Test
    public void testGetSimpleAnnotation_OnClassButNotOnMethod() {
        SimpleAnnotation retrievedAnnotation;

        retrievedAnnotation = AnnotationProcessingUtils.getAnnotation(
                getTest(ClassWithAnnotation.class, "methodWithoutAnnotation"),
                SimpleAnnotation.class);
        assertNotNull(retrievedAnnotation);
        assertTrue(retrievedAnnotation.value());
    }

    @Test
    public void testGetSimpleAnnotation_OnClassAndMethod() {
        SimpleAnnotation retrievedAnnotation;

        retrievedAnnotation = AnnotationProcessingUtils.getAnnotation(
                getTest(ClassWithAnnotation.class, "methodWithSimpleAnnotation"),
                SimpleAnnotation.class);
        assertNotNull(retrievedAnnotation);
        assertFalse(retrievedAnnotation.value());
    }

    @Test
    @Ignore("Rules not supported yet.")
    public void testGetSimpleAnnotation_OnRuleButNotOnMethod() {
        SimpleAnnotation retrievedAnnotation;

        retrievedAnnotation = AnnotationProcessingUtils.getAnnotation(
                getTest(ClassWithRule.class, "methodWithoutAnnotation"), SimpleAnnotation.class);
        assertNotNull(retrievedAnnotation);
        assertFalse(retrievedAnnotation.value());
    }

    @Test
    @Ignore("Rules not supported yet.")
    public void testGetSimpleAnnotation_OnRuleAndMethod() {
        SimpleAnnotation retrievedAnnotation;

        retrievedAnnotation = AnnotationProcessingUtils.getAnnotation(
                getTest(ClassWithRule.class, "methodWithSimpleAnnotation"), SimpleAnnotation.class);
        assertNotNull(retrievedAnnotation);
        assertTrue(retrievedAnnotation.value());
    }

    @Test
    public void testGetMetaAnnotation_Indirectly() {
        MetaAnnotation retrievedAnnotation;

        retrievedAnnotation = AnnotationProcessingUtils.getAnnotation(
                getTest(ClassWithoutAnnotation.class, "methodWithAnnotatedAnnotation"),
                MetaAnnotation.class);
        assertNotNull(retrievedAnnotation);
    }

    @Test
    public void testGetAllSimpleAnnotations() {
        List<SimpleAnnotation> retrievedAnnotations;

        retrievedAnnotations = AnnotationProcessingUtils.getAnnotations(
                getTest(ClassWithAnnotation.class, "methodWithSimpleAnnotation"),
                SimpleAnnotation.class);
        assertEquals(2, retrievedAnnotations.size());
        assertFalse(retrievedAnnotations.get(0).value()); // Annotation on method first.
        assertTrue(retrievedAnnotations.get(1).value()); // Annotation on class comes after.
    }

    private static Description getTest(Class<?> klass, String testName) {
        Description description = null;
        try {
            description = new DummyTestRunner(klass).describe(testName);
        } catch (InitializationError initializationError) {
            initializationError.printStackTrace();
            fail("DummyTestRunner initialization failed:" + initializationError.getMessage());
        }
        if (description == null) {
            fail("Not test named '" + testName + "' in class" + klass.getSimpleName());
        }
        return description;
    }

    // region Test Data: Annotations and dummy test classes
    @Retention(RetentionPolicy.RUNTIME)
    @Target({ElementType.TYPE, ElementType.METHOD})
    private @interface SimpleAnnotation {
        boolean value() default true;
    }

    @Retention(RetentionPolicy.RUNTIME)
    @Target({ElementType.ANNOTATION_TYPE, ElementType.TYPE, ElementType.METHOD})
    private @interface MetaAnnotation {}

    @Retention(RetentionPolicy.RUNTIME)
    @Target({ElementType.TYPE, ElementType.METHOD})
    @MetaAnnotation
    private @interface AnnotatedAnnotation {}

    private static class ClassWithoutAnnotation {
        @Test
        public void methodWithoutAnnotation() {}

        @Test
        @SimpleAnnotation
        public void methodWithSimpleAnnotation() {}

        @Test
        @AnnotatedAnnotation
        public void methodWithAnnotatedAnnotation() {}
    }

    @SimpleAnnotation
    private static class ClassWithAnnotation {
        @Test
        public void methodWithoutAnnotation() {}

        @Test
        @SimpleAnnotation(false)
        public void methodWithSimpleAnnotation() {}

        @Test
        @MetaAnnotation
        public void methodWithMetaAnnotation() {}

        @Test
        @AnnotatedAnnotation
        public void methodWithAnnotatedAnnotation() {}
    }

    private static class ClassWithRule {
        @Rule
        Rule1 mRule = new Rule1();

        @Test
        public void methodWithoutAnnotation() {}

        @Test
        @SimpleAnnotation
        public void methodWithSimpleAnnotation() {}
    }

    @SimpleAnnotation(false)
    @MetaAnnotation
    private static class Rule1 implements TestRule {
        @Override
        public Statement apply(Statement statement, Description description) {
            return null;
        }
    }

    private static class DummyTestRunner extends BlockJUnit4ClassRunner {
        public DummyTestRunner(Class<?> klass) throws InitializationError {
            super(klass);
        }

        @Override
        protected void collectInitializationErrors(List<Throwable> errors) {
            // Do nothing. BlockJUnit4ClassRunner requires the class to be public, but we don't
            // want/need it.
        }

        public Description describe(String testName) {
            List<FrameworkMethod> tests = getTestClass().getAnnotatedMethods(Test.class);
            for (FrameworkMethod testMethod : tests) {
                if (testMethod.getName().equals(testName)) return describeChild(testMethod);
            }
            return null;
        }
    }

    // endregion
}