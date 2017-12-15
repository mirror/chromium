// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

import org.gradle.api.Plugin
import org.gradle.api.Project

class ChromiumPlugin implements Plugin<Project> {
    void apply(Project project) {
        // The configurations here are going to be used in ChromiumDepGraph. Keep it up to date
        // with the declarations below.
        project.configurations {
            /** Main type of configuration, use it for libraries that the APK depends on. */
            compile

            /**
             * Marks that the dependency will only be used during building, as an annotation
             * processor. It will not be usable in the APK, and not marked as supporting android.
             */
            annotationProcessor

        }
    }
}