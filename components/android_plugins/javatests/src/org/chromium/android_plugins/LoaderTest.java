// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.android_plugins;

import android.content.Context;
import android.support.test.InstrumentationRegistry;
import android.support.test.filters.SmallTest;

import dalvik.system.DexClassLoader;

import org.junit.Assert;
import org.junit.Test;
import org.junit.runner.RunWith;

import org.chromium.base.test.BaseJUnit4ClassRunner;
import org.chromium.base.test.util.UrlUtils;
import org.chromium.example_plugin_a.ExamplePluginAInterface;

import java.io.File;

@RunWith(BaseJUnit4ClassRunner.class)
public class LoaderTest {
    @Test
    @SmallTest
    public void testNothing()
            throws ClassNotFoundException, InstantiationException, NoSuchMethodException,
                   IllegalAccessException, java.lang.reflect.InvocationTargetException {
        Context context = InstrumentationRegistry.getTargetContext();
        String apkFile = UrlUtils.getIsolatedTestFilePath("apks/example_plugin_a_apk");
        String className = "org.chromium.example_plugin_a.ExamplePluginA";
        File optimizedDexOutputPath = context.getDir("outdex", 0);
        DexClassLoader dLoader = new DexClassLoader(
                apkFile, optimizedDexOutputPath.getAbsolutePath(), null, context.getClassLoader());

        Class<?> loadedClass = dLoader.loadClass(className);
        ExamplePluginAInterface plugin =
                (ExamplePluginAInterface) loadedClass.getConstructor().newInstance();
        int s = plugin.sum(2, 3);

        Assert.assertEquals(5, s);
        Assert.assertEquals("Howdy", plugin.hello());
    }
}