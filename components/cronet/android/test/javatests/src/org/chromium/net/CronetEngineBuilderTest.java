// Copyright 2017 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.net;

import static org.chromium.net.CronetProvider.PROVIDER_NAME_APP_PACKAGED;
import static org.chromium.net.CronetProvider.PROVIDER_NAME_FALLBACK;

import android.content.Context;
import android.support.test.InstrumentationRegistry;
import android.support.test.filters.SmallTest;

import org.junit.Assert;
import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;

import org.chromium.base.annotations.SuppressFBWarnings;
import org.chromium.base.test.BaseJUnit4ClassRunner;
import org.chromium.base.test.util.Feature;

import java.util.ArrayList;
import java.util.Arrays;
import java.util.List;

/**
 * Tests {@link CronetEngine.Builder}.
 */
@RunWith(BaseJUnit4ClassRunner.class)
public class CronetEngineBuilderTest {
    @SuppressFBWarnings("URF_UNREAD_PUBLIC_OR_PROTECTED_FIELD")
    @Rule
    public CronetTestRule mTestRule = new CronetTestRule();

    /**
     * Tests the comparison of two strings that contain versions.
     */
    @Test
    @SmallTest
    @Feature({"Cronet"})
    @CronetTestRule.OnlyRunNativeCronet
    public void testVersionComparison() {
        assertVersionIsHigher("22.44", "22.43.12");
        assertVersionIsLower("22.43.12", "022.124");
        assertVersionIsLower("22.99", "22.100");
        assertVersionIsHigher("22.100", "22.99");
        assertVersionIsEqual("11.2.33", "11.2.33");
        assertIllegalArgumentException(null, "1.2.3");
        assertIllegalArgumentException("1.2.3", null);
        assertIllegalArgumentException("1.2.3", "1.2.3x");
    }

    /**
     * Tests the correct ordering of the providers. The platform provider should be
     * the last in the list. Other providers should be ordered by placing providers
     * with the higher version first.
     */
    @Test
    @SmallTest
    @Feature({"Cronet"})
    public void testProviderOrdering() {
        final CronetProvider[] availableProviders = new CronetProvider[] {
                new FakeProvider(InstrumentationRegistry.getTargetContext(),
                        PROVIDER_NAME_APP_PACKAGED, "99.77", true),
                new FakeProvider(InstrumentationRegistry.getTargetContext(), PROVIDER_NAME_FALLBACK,
                        "99.99", true),
                new FakeProvider(InstrumentationRegistry.getTargetContext(), "Some other provider",
                        "99.88", true),
        };

        ArrayList<CronetProvider> providers = new ArrayList<>(Arrays.asList(availableProviders));
        List<CronetProvider> orderedProviders = CronetEngine.Builder.getEnabledCronetProviders(
                InstrumentationRegistry.getTargetContext(), providers);

        // Check the result
        Assert.assertEquals(availableProviders[2], orderedProviders.get(0));
        Assert.assertEquals(availableProviders[0], orderedProviders.get(1));
        Assert.assertEquals(availableProviders[1], orderedProviders.get(2));
    }

    /**
     * Tests that the providers that are disabled are not included in the list of available
     * providers when the provider is selected by the default selection logic.
     */
    @Test
    @SmallTest
    @Feature({"Cronet"})
    public void testThatDisabledProvidersAreExcluded() {
        final CronetProvider[] availableProviders = new CronetProvider[] {
                new FakeProvider(InstrumentationRegistry.getTargetContext(), PROVIDER_NAME_FALLBACK,
                        "99.99", true),
                new FakeProvider(InstrumentationRegistry.getTargetContext(),
                        PROVIDER_NAME_APP_PACKAGED, "99.77", true),
                new FakeProvider(InstrumentationRegistry.getTargetContext(), "Some other provider",
                        "99.88", false),
        };

        ArrayList<CronetProvider> providers = new ArrayList<>(Arrays.asList(availableProviders));
        List<CronetProvider> orderedProviders = CronetEngine.Builder.getEnabledCronetProviders(
                InstrumentationRegistry.getTargetContext(), providers);

        Assert.assertEquals(
                "Unexpected number of providers in the list", 2, orderedProviders.size());
        Assert.assertEquals(PROVIDER_NAME_APP_PACKAGED, orderedProviders.get(0).getName());
        Assert.assertEquals(PROVIDER_NAME_FALLBACK, orderedProviders.get(1).getName());
    }

    private void assertVersionIsHigher(String s1, String s2) {
        Assert.assertEquals(1, CronetEngine.Builder.compareVersions(s1, s2));
    }

    private void assertVersionIsLower(String s1, String s2) {
        Assert.assertEquals(-1, CronetEngine.Builder.compareVersions(s1, s2));
    }

    private void assertVersionIsEqual(String s1, String s2) {
        Assert.assertEquals(0, CronetEngine.Builder.compareVersions(s1, s2));
    }

    private void assertIllegalArgumentException(String s1, String s2) {
        try {
            CronetEngine.Builder.compareVersions(s1, s2);
        } catch (IllegalArgumentException e) {
            // Do nothing. It is expected.
            return;
        }
        Assert.fail("Expected IllegalArgumentException");
    }

    // TODO(kapishnikov): Replace with a mock when mockito is supported.
    private static class FakeProvider extends CronetProvider {
        private final String mName;
        private final String mVersion;
        private final boolean mEnabled;

        protected FakeProvider(Context context, String name, String version, boolean enabled) {
            super(context);
            mName = name;
            mVersion = version;
            mEnabled = enabled;
        }

        @Override
        public CronetEngine.Builder createBuilder() {
            return new CronetEngine.Builder((ICronetEngineBuilder) null);
        }

        @Override
        public String getName() {
            return mName;
        }

        @Override
        public String getVersion() {
            return mVersion;
        }

        @Override
        public boolean isEnabled() {
            return mEnabled;
        }

        @Override
        public String toString() {
            return mName;
        }
    }
}
