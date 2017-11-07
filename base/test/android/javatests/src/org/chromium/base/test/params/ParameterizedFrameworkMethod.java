// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.base.test.params;

import org.junit.runners.model.FrameworkMethod;

import java.lang.annotation.Annotation;
import java.lang.reflect.Method;
import java.util.ArrayList;
import java.util.Collection;
import java.util.List;

/**
 * Custom FrameworkMethod that includes a {@code ParameterSet} that
 * represents the parameters for this test method
 */
public class ParameterizedFrameworkMethod extends FrameworkMethod {
    private final ParameterSet mParameterSet;
    private final String mName;

    public ParameterizedFrameworkMethod(
            Method method, ParameterSet parameterSet, String classParameterSetName) {
        super(method);
        mParameterSet = parameterSet;
        StringBuilder name = new StringBuilder(method.getName());
        if (classParameterSetName != null && !classParameterSetName.isEmpty()) {
            name.append("_" + classParameterSetName);
        }
        if (parameterSet != null && !parameterSet.getName().isEmpty()) {
            name.append("_" + parameterSet.getName());
        }
        mName = name.toString();
    }

    @Override
    public String getName() {
        return mName;
    }

    @Override
    public Object invokeExplosively(Object target, Object... params) throws Throwable {
        if (mParameterSet != null) {
            return super.invokeExplosively(target, mParameterSet.getValues().toArray());
        }
        return super.invokeExplosively(target, params);
    }

    static List<FrameworkMethod> wrapAllFrameworkMethods(
            Collection<FrameworkMethod> frameworkMethods, String classParameterSetName) {
        List<FrameworkMethod> results = new ArrayList<>();
        for (FrameworkMethod frameworkMethod : frameworkMethods) {
            results.add(new ParameterizedFrameworkMethod(
                    frameworkMethod.getMethod(), null, classParameterSetName));
        }
        return results;
    }

    @Override
    public boolean equals(Object obj) {
        if (obj instanceof ParameterizedFrameworkMethod) {
            ParameterizedFrameworkMethod method = (ParameterizedFrameworkMethod) obj;
            return super.equals(obj) && method.mParameterSet.equals(mParameterSet)
                    && method.getName().equals(getName());
        }
        return false;
    }

    /**
     * Override hashCode method to distinguish two ParameterizedFrameworkmethod with same
     * Method object.
     */
    @Override
    public int hashCode() {
        int result = 17;
        result = 31 * result + super.hashCode();
        result = 31 * result + getName().hashCode();
        if (mParameterSet != null) {
            result = 31 * result + mParameterSet.hashCode();
        }
        return result;
    }

    Annotation[] getTestAnnotations() {
        // TODO(yolandyan): add annotation from the ParameterSet, enable
        // test writing to add SkipCheck for an individual parameter
        return getMethod().getAnnotations();
    }

    public ParameterSet getParameterSet() {
        return mParameterSet;
    }
}
