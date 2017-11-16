// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.testing.local;

import org.robolectric.annotation.Config;
import org.robolectric.internal.ManifestFactory;
import org.robolectric.internal.ManifestIdentifier;
import org.robolectric.manifest.AndroidManifest;
import org.robolectric.res.Fs;
import org.robolectric.res.FsFile;
import org.robolectric.res.ResourcePath;

import java.io.File;
import java.net.MalformedURLException;
import java.util.ArrayList;
import java.util.List;

/**
 * Class that manages passing Android manifest information to Robolectric.
 */
public class GNManifestFactory implements ManifestFactory {
    private static final String CHROMIUM_MANIFEST_PATH = "chromium.robolectric.manifest";
    private static final String CHROMIUM_RES_DIRECTORIES = "chromium.robolectric.resource.dirs";
    private static final String CHROMIUM_RES_PACKAGES = "chromium.robolectric.resource.packages";

    @Override
    public ManifestIdentifier identify(Config config) {
        if (config.resourceDir() != null
                && !config.resourceDir().equals(Config.DEFAULT_RES_FOLDER)) {
            throw new RuntimeException("Resource dirs should be generated automatically by GN. "
                    + "Make sure you specify the correct app package_name in the GN build file. "
                    + "Make sure you run the tests using the generated run_<test name> scripts.");
        }

        if (config.manifest() != null && !config.manifest().equals(Config.NONE)) {
            throw new RuntimeException("Specify manifest path in GN build file.");
        }

        return new ManifestIdentifier(null, null, null, config.packageName(), null);
    }

    @Override
    public AndroidManifest create(ManifestIdentifier manifestIdentifier) {
        String manifestPath = System.getProperty(CHROMIUM_MANIFEST_PATH);
        String resourceDirs = System.getProperty(CHROMIUM_RES_DIRECTORIES);
        String resourcePackages = System.getProperty(CHROMIUM_RES_PACKAGES);

        final List<Class> resourceRClassList = new ArrayList<>();
        final List<FsFile> resourceDirsList = new ArrayList<>();
        if (resourceDirs != null) {
            for (String packageName : resourcePackages.split(":")) {
                try {
                    resourceRClassList.add(Class.forName(packageName + ".R"));
                } catch (ClassNotFoundException e) {
                    throw new RuntimeException(e);
                }
            }
            for (String resourceDir : resourceDirs.split(":")) {
                try {
                    resourceDirsList.add(Fs.fromURL(new File(resourceDir).toURI().toURL()));
                } catch (MalformedURLException e) {
                    throw new RuntimeException(e);
                }
            }
        }

        FsFile manifestFile = null;
        if (manifestPath != null) {
            try {
                manifestFile = Fs.fromURL(new File(manifestPath).toURI().toURL());
            } catch (MalformedURLException e) {
                throw new RuntimeException(e);
            }
        }

        return new AndroidManifest(manifestFile, null, manifestIdentifier.getAssetDir(), manifestIdentifier.getPackageName()) {
            @Override
            public List<ResourcePath> getIncludedResourcePaths() {
                List<ResourcePath> paths = super.getIncludedResourcePaths();
                Class mainRClass = getRClass();
                for (FsFile resourceDir : resourceDirsList) {
                    paths.add(new ResourcePath(mainRClass, resourceDir, null));
                }

                // Add all non-primary R classes, but set their resources to null to avoid them from
                // being re-parsed.
                for (Class rClass : resourceRClassList) {
                    if (rClass != mainRClass) {
                        paths.add(new ResourcePath(rClass, null, null));
                    }
                }
                return paths;
            }
        };
    }
}
