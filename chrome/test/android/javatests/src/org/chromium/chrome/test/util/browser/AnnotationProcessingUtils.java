// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.test.util.browser;

import org.junit.runner.Description;

import java.lang.annotation.Annotation;
import java.util.ArrayList;
import java.util.Arrays;
import java.util.Collection;
import java.util.HashSet;
import java.util.List;
import java.util.Set;

public abstract class AnnotationProcessingUtils {
    public static <T extends Annotation> T getAnnotation(Description description, Class<T> clazz) {
        List<T> annotations = getAnnotations(description, clazz);
        return annotations.isEmpty() ? null : annotations.get(0);
    }

    public static <T extends Annotation> List<T> getAnnotations(
            Description description, Class<T> clazz) {
        List<T> collectedAnnotations = new ArrayList<>();

        Set<Class<? extends Annotation>> visitedAnnotations = new HashSet<>();
        collectAnnotations(description, clazz, visitedAnnotations, collectedAnnotations);

        Class<?> testClass = description.getTestClass();
        collectAnnotations(testClass, clazz, visitedAnnotations, collectedAnnotations);

        // TODO(dgn): collectAnnotations on parent classes? We have a few cases where we use them.
        // TODO(dgn): collectAnnotations on Rules? (currently implemented for CommandLineFlags).

        return collectedAnnotations;
    }

    private static <A extends Annotation> void collectAnnotations(Description description,
            Class<A> annotationType, Set<Class<? extends Annotation>> visited,
            List<A> collectedAnnotations) {
        collectAnnotations(
                description.getAnnotations(), annotationType, visited, collectedAnnotations);
    }

    private static <A extends Annotation> void collectAnnotations(Class<?> recipient,
            Class<A> annotationType, Set<Class<? extends Annotation>> visited,
            List<A> collectedAnnotations) {
        collectAnnotations(Arrays.asList(recipient.getDeclaredAnnotations()), annotationType,
                visited, collectedAnnotations);
    }

    @SuppressWarnings("unchecked")
    private static <A extends Annotation> void collectAnnotations(
            Collection<Annotation> declaredAnnotations, Class<A> annotationType,
            Set<Class<? extends Annotation>> visited, List<A> collectedAnnotations) {
        for (Annotation annotation : declaredAnnotations) {
            if (annotation.annotationType() == annotationType) {
                collectedAnnotations.add((A) annotation);
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
