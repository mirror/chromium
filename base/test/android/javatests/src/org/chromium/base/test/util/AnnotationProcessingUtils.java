// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.base.test.util;

import android.support.annotation.Nullable;

import org.junit.runner.Description;

import java.lang.annotation.Annotation;
import java.lang.reflect.AnnotatedElement;
import java.lang.reflect.Method;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collections;
import java.util.Comparator;
import java.util.HashSet;
import java.util.LinkedList;
import java.util.List;
import java.util.Queue;
import java.util.Set;

/**
 * Utility class to help with processing annotations, going around the code to collect them, etc.
 */
public abstract class AnnotationProcessingUtils {
    /**
     * Returns the closest instance of the requested annotation or null if there is none.
     * See {@link AnnotationsExtractor} for context of "closest".
     */
    @SuppressWarnings("unchecked")
    public static <A extends Annotation> A getAnnotation(Description description, Class<A> clazz) {
        AnnotationsExtractor extractor = new AnnotationsExtractor(clazz);
        return (A) extractor.getClosest(extractor.getMatchingAnnotations(description));
    }

    /**
     * Returns the closest instance of the requested annotation or null if there is none.
     * See {@link AnnotationsExtractor} for context of "closest".
     */
    @SuppressWarnings("unchecked")
    public static <A extends Annotation> A getAnnotation(AnnotatedElement element, Class<A> clazz) {
        AnnotationsExtractor extractor = new AnnotationsExtractor(clazz);
        return (A) extractor.getClosest(extractor.getMatchingAnnotations(element));
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
     * Processes various types of annotated elements ({@link Class}es, {@link Annotation}s,
     * {@link Description}s, etc.) and extracts the targeted annotations from it. The order will be
     * returned in BFS order.
     *
     * For example, for a method we would get in reverse order:
     * - the method annotations
     * - the class annotations
     * - the meta-annotations present on the method annotations,
     * - the annotations present on the super class,
     * - the meta-annotations present on the class annotations,
     * - etc.
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
            return getMatchingAnnotations(new AnnotatedNode.FromDescription(description));
        }

        public List<Annotation> getMatchingAnnotations(AnnotatedElement annotatedElement) {
            AnnotatedNode annotatedNode;
            if (annotatedElement instanceof Method) {
                annotatedNode = new AnnotatedNode.FromMethod((Method) annotatedElement);
            } else if (annotatedElement instanceof Class) {
                annotatedNode = new AnnotatedNode.FromClass((Class) annotatedElement);
            } else {
                throw new IllegalArgumentException("Unsupported type for " + annotatedElement);
            }

            return getMatchingAnnotations(annotatedNode);
        }

        /**
         * For a given list obtained from the extractor, returns the {@link Annotation} that would
         * be closest from the extraction point, or {@code null} if the list is empty.
         */
        @Nullable
        public Annotation getClosest(List<Annotation> annotationList) {
            return annotationList.isEmpty() ? null : annotationList.get(annotationList.size() - 1);
        }

        private List<Annotation> getMatchingAnnotations(AnnotatedNode annotatedNode) {
            List<Annotation> collectedAnnotations = new ArrayList<>();
            Queue<AnnotatedNode> workingSet = new LinkedList<>();
            workingSet.add(annotatedNode);
            Set<Class<? extends Annotation>> visited = new HashSet<>();
            while (!workingSet.isEmpty()) {
                sweepAnnotations(collectedAnnotations, workingSet, visited);
            }
            return collectedAnnotations;
        }

        private void sweepAnnotations(List<Annotation> collectedAnnotations,
                Queue<AnnotatedNode> workingSet, Set<Class<? extends Annotation>> visited) {
            AnnotatedNode node = workingSet.remove();
            if (node instanceof AnnotatedNode.FromAnnotation) {
                Annotation annotation = ((AnnotatedNode.FromAnnotation) node).mAnnotation;
                if (mAnnotationTypes.contains(annotation.annotationType())) {
                    collectedAnnotations.add(0, annotation);
                }
                if (!visited.add(annotation.annotationType())) return;
                if (!isChromiumAnnotation(annotation)) return;
            }

            node.collectChildNodes(mComparator, workingSet);
        }
    }

    private static abstract class AnnotatedNode {
        @Nullable
        abstract protected AnnotatedNode getParent();

        abstract protected List<Annotation> getAnnotations();

        public void collectChildNodes(
                Comparator<Annotation> comparator, Queue<AnnotatedNode> workingSet) {
            List<Annotation> annotations = getAnnotations();
            Collections.sort(annotations, comparator);

            for (Annotation annotation : annotations) {
                workingSet.add(new FromAnnotation(annotation));
            }
            AnnotatedNode parent = getParent();
            if (parent != null) workingSet.add(parent);
        }

        @Nullable
        public Annotation match(List<Class<? extends Annotation>> reference) {
            return null;
        }

        public static class FromDescription extends AnnotatedNode {
            private final Description mDescription;

            FromDescription(Description description) {
                mDescription = description;
            }

            @Nullable
            @Override
            protected AnnotatedNode getParent() {
                return new AnnotatedNode.FromClass(mDescription.getTestClass());
            }

            @Override
            protected List<Annotation> getAnnotations() {
                return new ArrayList<>(mDescription.getAnnotations());
            }
        }

        public static class FromClass extends AnnotatedNode {
            private final Class<?> mClass;

            public FromClass(Class<?> clazz) {
                mClass = clazz;
            }

            @Nullable
            @Override
            protected AnnotatedNode getParent() {
                Class<?> superClass = mClass.getSuperclass();
                return superClass == null ? null : new AnnotatedNode.FromClass(superClass);
            }

            @Override
            protected List<Annotation> getAnnotations() {
                return Arrays.asList(mClass.getDeclaredAnnotations());
            }
        }

        public static class FromMethod extends AnnotatedNode {
            private final Method mMethod;

            public FromMethod(Method method) {
                mMethod = method;
            }

            @Nullable
            @Override
            protected AnnotatedNode getParent() {
                return new AnnotatedNode.FromClass(mMethod.getDeclaringClass());
            }

            @Override
            protected List<Annotation> getAnnotations() {
                return Arrays.asList(mMethod.getDeclaredAnnotations());
            }
        }

        public static class FromAnnotation extends AnnotatedNode {
            private final Annotation mAnnotation;

            public FromAnnotation(Annotation annotation) {
                mAnnotation = annotation;
            }

            @Nullable
            @Override
            protected AnnotatedNode getParent() {
                return null;
            }

            @Override
            protected List<Annotation> getAnnotations() {
                return Arrays.asList(mAnnotation.annotationType().getDeclaredAnnotations());
            }

            @Override
            public Annotation match(List<Class<? extends Annotation>> reference) {
                return reference.contains(mAnnotation.annotationType()) ? mAnnotation : null;
            }
        }
    }
}
