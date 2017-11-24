// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.base.test.util;

import org.junit.runner.Description;

import java.lang.annotation.Annotation;
import java.lang.reflect.AnnotatedElement;
import java.lang.reflect.Method;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collection;
import java.util.HashSet;
import java.util.List;
import java.util.Set;

/**
 * Utility class to help with processing annotations, going around the code to collect them, etc.
 */
public abstract class AnnotationProcessingUtils {
    /**
     * Returns the closest instance of the requested annotation or null if there is none.
     * @see #getAnnotations(Description, Class) for context of "closest".
     */
    public static <T extends Annotation> T getAnnotation(Description description, Class<T> clazz) {
        List<T> annotations = getAnnotations(description, clazz);
        return annotations.isEmpty() ? null : annotations.get(annotations.size() - 1);
    }

    /**
     * Returns the closest instance of the requested annotation or null if there is none.
     * @see #getAnnotations(AnnotatedElement, Class) for context of "closest".
     */
    public static <T extends Annotation> T getAnnotation(AnnotatedElement element, Class<T> clazz) {
        List<T> annotations = getAnnotations(element, clazz);
        return annotations.isEmpty() ? null : annotations.get(annotations.size() - 1);
    }

    /**
     * Returns annotations found by walking the code, ordered from the furthest to the closest.
     *
     * Annotation proximity is evaluated as follows, from the closest to the furthest:
     * - Annotation declared straight on the method
     * - Annotation declared on an annotation present on the method
     * - Annotation declared on the class
     * - Annotation declared on an annotation present on the class
     */
    public static <T extends Annotation> List<T> getAnnotations(
            Description description, Class<T> annotationClass) {
        List<T> collectedAnnotations = new ArrayList<>();
        collectAnnotations(description, annotationClass, new HashSet<>(), collectedAnnotations);
        return collectedAnnotations;
    }

    /**
     * Returns annotations found by walking the code, ordered from the furthest to the closest.
     *
     * Annotation proximity is evaluated as follows, from the closest to the furthest:
     * - Annotation declared straight on the method
     * - Annotation declared on an annotation present on the method
     * - Annotation declared on the class
     * - Annotation declared on an annotation present on the class
     */
    public static <T extends Annotation> List<T> getAnnotations(
            AnnotatedElement annotatedElement, Class<T> annotationClass) {
        List<T> collectedAnnotations = new ArrayList<>();
        collectAnnotations(
                annotatedElement, annotationClass, new HashSet<>(), collectedAnnotations);
        return collectedAnnotations;
    }

    private static <A extends Annotation> void collectAnnotations(AnnotatedElement annotatedElement,
            Class<A> annotationType, Set<Class<? extends Annotation>> visited,
            List<A> collectedAnnotations) {
        collectAnnotations(Arrays.asList(annotatedElement.getDeclaredAnnotations()), annotationType,
                visited, collectedAnnotations);

        if (annotatedElement instanceof Method) {
            collectAnnotations(((Method) annotatedElement).getDeclaringClass(), annotationType,
                    visited, collectedAnnotations);
        } else if (annotatedElement instanceof Class<?>) {
            Class<?> annotatedClass = (Class<?>) annotatedElement;
            if (annotatedClass.getSuperclass() != null) {
                collectAnnotations(annotatedClass.getSuperclass(), annotationType, visited,
                        collectedAnnotations);
            }
        }

        // TODO(dgn): collectAnnotations on Rules? (currently implemented for CommandLineFlags).
    }

    private static <A extends Annotation> void collectAnnotations(Description description,
            Class<A> annotationType, Set<Class<? extends Annotation>> visited,
            List<A> collectedAnnotations) {
        collectAnnotations(
                description.getAnnotations(), annotationType, visited, collectedAnnotations);
        collectAnnotations(
                description.getTestClass(), annotationType, visited, collectedAnnotations);
    }

    /**
     * Walks through {@code declaredAnnotations}, collecting the ones that match
     * {@code annotationType}, then does the same on annotation that are declared on each of the
     * declared ones.
     *
     * @param declaredAnnotations Annotations that will be searched for {@code annotationType}.
     * @param annotationType Type of the annotations we are looking for.
     * @param visited Types of annotations we already visited and will not need to search again.
     * @param collectedAnnotations Holds the found instances of the annotation we are looking for.
     */
    @SuppressWarnings("unchecked")
    private static <A extends Annotation> void collectAnnotations(
            Collection<Annotation> declaredAnnotations, Class<A> annotationType,
            Set<Class<? extends Annotation>> visited, List<A> collectedAnnotations) {
        for (Annotation annotation : declaredAnnotations) {
            if (annotation.annotationType() == annotationType) {
                collectedAnnotations.add(0, (A) annotation);
                visited.add(annotation.annotationType());
            }
        }

        // Order the annotations by layer, we now process the ones that did not match and go deeper.
        for (Annotation annotation : declaredAnnotations) {
            Class<? extends Annotation> evaluatedType = annotation.annotationType();
            if (isChromiumAnnotation(annotation) && visited.add(evaluatedType)) {
                collectAnnotations(evaluatedType, annotationType, visited, collectedAnnotations);
            }
        }
    }

    private static boolean isChromiumAnnotation(Annotation annotation) {
        return annotation.annotationType().getPackage().getName().startsWith("org.chromium");
    }
}
