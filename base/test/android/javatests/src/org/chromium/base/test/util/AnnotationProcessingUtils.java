// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.base.test.util;

import org.junit.rules.ExternalResource;
import org.junit.runner.Description;
import org.junit.runners.model.Statement;

import java.lang.annotation.Annotation;
import java.lang.reflect.AnnotatedElement;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collections;
import java.util.Comparator;
import java.util.Deque;
import java.util.HashSet;
import java.util.LinkedList;
import java.util.List;
import java.util.Set;

/**
 * Utility class to help with processing annotations, going around the code to collect them, etc.
 */
public abstract class AnnotationProcessingUtils {
    /**
     * Returns the closest instance of the requested annotation or null if there is none.
     See {@link AnnotationsExtractor} for context of "closest".
     */
    @SuppressWarnings("unchecked")
    public static <A extends Annotation> A getAnnotation(Description description, Class<A> clazz) {
        List<Annotation> annotations =
                new AnnotationsExtractor(clazz).getMatchingAnnotations(description);
        return annotations.isEmpty() ? null : (A) annotations.get(annotations.size() - 1);
    }

    /**
     * Returns the closest instance of the requested annotation or null if there is none.
     * See {@link AnnotationsExtractor} for context of "closest".
     */
    @SuppressWarnings("unchecked")
    public static <A extends Annotation> A getAnnotation(AnnotatedElement element, Class<A> clazz) {
        List<Annotation> annotations =
                new AnnotationsExtractor(clazz).getMatchingAnnotations(element);
        return annotations.isEmpty() ? null : (A) annotations.get(annotations.size() - 1);
    }

    /** See {@link AnnotationsExtractor} for details about the output sorting order. */
    @SuppressWarnings("unchecked")
    public static <A extends Annotation> List<A> getAnnotations(
            Description description, Class<A> annotationType) {
        return (List<A>) new AnnotationsExtractor(annotationType)
                .getMatchingAnnotations(description);
    }

    /** See {@link AnnotationsExtractor} for details about the output sorting order. */
    @SuppressWarnings("unchecked")
    public static <A extends Annotation> List<A> getAnnotations(
            AnnotatedElement annotatedElement, Class<A> annotationType) {
        return (List<A>) new AnnotationsExtractor(annotationType)
                .getMatchingAnnotations(annotatedElement);
    }

    private static boolean isChromiumAnnotation(Annotation annotation) {
        Package pkg = annotation.annotationType().getPackage();
        return pkg != null && pkg.getName().startsWith("org.chromium");
    }

    /**
     * Collects annotations matching the type(s) specified in the constructor, and provides
     * them through {@link #getAnnotations()}.
     */
    public static class AnnotationProcessor extends ExternalResource {
        private final AnnotationsExtractor mAnnotationExtractor;
        private List<Annotation> mCollectedAnnotations;
        private Description mTestDescription;

        @SafeVarargs
        public AnnotationProcessor(Class<? extends Annotation> firstAnnotationType,
                Class<? extends Annotation>... additionalTypes) {
            List<Class<? extends Annotation>> mAnnotationTypes = new ArrayList<>();
            mAnnotationTypes.add(firstAnnotationType);
            if (additionalTypes.length > 0) mAnnotationTypes.addAll(Arrays.asList(additionalTypes));
            mAnnotationExtractor = new AnnotationsExtractor(mAnnotationTypes);
        }

        @Override
        public Statement apply(Statement base, Description description) {
            mTestDescription = description;

            mCollectedAnnotations = mAnnotationExtractor.getMatchingAnnotations(description);
            if (mCollectedAnnotations.isEmpty()) return base;

            // Return the wrapped statement to execute before() and after().
            return super.apply(base, description);
        }

        /** @return {@link Description} of the current test. */
        protected Description getTestDescription() {
            return mTestDescription;
        }

        /**
         * @return The collected annotations that match the declared type(s).
         * @throws NullPointerException if this is called before annotations have been collected,
         * which happens when the rule is applied to the {@link Statement}.
         */
        protected List<Annotation> getAnnotations() {
            return Collections.unmodifiableList(mCollectedAnnotations);
        }
    }

    /**
     * Processes various types of annotated elements ({@link Class}es, {@link Annotation}s,
     * {@link Description}s, etc.) and extracts the targeted annotations from it. The order will be
     * returned in BFS order.
     *
     * For example, for a method we would get in reverse order:
     * - the method annotations
     * - the class annotations
     * - the meta-annotations present on the method annotations,
     * - the meta-annotations present on the class annotations,
     * - etc.
     *
     * The class hierarchy is not examined, but an annotation marked with
     * {@link java.lang.annotation.Inherited} will be transferred to the derived class.
     *
     * When multiple annotations are targeted, if more than one is picked up at a given level (for
     * example directly on the method), they will be returned in the reverse order that they were
     * provided to the constructor.
     *
     * Note: We return the annotations in reverse order because we assume that if some processing
     * is going to be made on related annotations, the later annotations would likely override
     * modifications made by the former.
     */
    public static class AnnotationsExtractor {
        private final List<Class<? extends Annotation>> mAnnotationTypes;
        private final Comparator<Annotation> mComparator;

        @SafeVarargs
        public AnnotationsExtractor(Class<? extends Annotation>... additionalTypes) {
            this(Arrays.asList(additionalTypes));
        }

        public AnnotationsExtractor(List<Class<? extends Annotation>> additionalTypes) {
            assert !additionalTypes.isEmpty();
            mAnnotationTypes = Collections.unmodifiableList(additionalTypes);
            mComparator = (a1, a2)
                    -> mAnnotationTypes.indexOf(a1.annotationType())
                    - mAnnotationTypes.indexOf(a2.annotationType());
        }

        public List<Annotation> getMatchingAnnotations(Description description) {
            List<Annotation> workingSet = new LinkedList<>();
            workingSet.addAll(getAnnotationsInOrder(description));
            System.out.println("working set from method: " + workingSet);
            workingSet.addAll(getAnnotationsInOrder(description.getTestClass()));
            System.out.println("full initial working set:" + workingSet);

            return getMatchingAnnotations(workingSet);
        }

        public List<Annotation> getMatchingAnnotations(AnnotatedElement annotatedElement) {
            return getMatchingAnnotations(Arrays.asList(annotatedElement.getAnnotations()));
        }

        public List<Annotation> getMatchingAnnotations(List<Annotation> allAnnotations) {
            List<Annotation> collectedAnnotations = new ArrayList<>();
            Deque<Annotation> workingSet = new LinkedList<>(allAnnotations);
            while (!workingSet.isEmpty()) {
                sweepAnnotations(collectedAnnotations, workingSet, new HashSet<>());
            }
            return collectedAnnotations;
        }

        private void sweepAnnotations(List<Annotation> collectedAnnotations,
                Deque<Annotation> workingSet, Set<Class<? extends Annotation>> visited) {
            System.out.println("Set size: " + workingSet.size());
            if (workingSet.isEmpty()) return;

            Annotation annotation = workingSet.pop();
            System.out.println("Processing " + annotation);
            if (mAnnotationTypes.contains(annotation.annotationType())) {
                collectedAnnotations.add(0, annotation);
                // TODO return here? should we explore annotations that we are looking for?
            }
            if (!visited.add(annotation.annotationType())) return;
            if (!isChromiumAnnotation(annotation)) return;

            workingSet.addAll(getAnnotationsInOrder(annotation.annotationType()));
            sweepAnnotations(collectedAnnotations, workingSet, visited);
        }

        private List<Annotation> getAnnotationsInOrder(Description description) {
            List<Annotation> annotations = new ArrayList<>(description.getAnnotations());
            Collections.sort(annotations, mComparator);
            return annotations;
        }

        private List<Annotation> getAnnotationsInOrder(AnnotatedElement annotatedElement) {
            List<Annotation> annotations =
                    new ArrayList<>(Arrays.asList(annotatedElement.getAnnotations()));
            Collections.sort(annotations, mComparator);
            return annotations;
        }
    }
}
