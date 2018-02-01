// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.example_plugin_a;

// import org.chromium.android_plugins.PluginAPI;

public class ExamplePluginA implements ExamplePluginAInterface {
    @Override
    public int sum(int a, int b) {
        return a + b;
    }

    @Override
    public String hello() {
        return notPartOfApi();
    }

    public String notPartOfApi() {
        return "Howdy";
    }
}