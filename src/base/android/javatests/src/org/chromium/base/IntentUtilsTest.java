// Copyright 2017 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.base;

import static org.junit.Assert.assertEquals;

import android.content.ComponentName;
import android.content.Intent;
import android.os.Bundle;

import androidx.test.filters.SmallTest;

import org.junit.Test;
import org.junit.runner.RunWith;

import org.chromium.base.test.BaseJUnit4ClassRunner;
import org.chromium.base.test.util.Batch;
import org.chromium.build.BuildConfig;

/** Tests for {@link IntentUtils}. */
@RunWith(BaseJUnit4ClassRunner.class)
@Batch(Batch.UNIT_TESTS)
public class IntentUtilsTest {
    private void assertTargetsSelf(boolean targetsSelf, Intent intent, boolean expectAssertion) {
        boolean asserted = false;
        try {
            assertEquals(targetsSelf, IntentUtils.intentTargetsSelf(intent));
        } catch (AssertionError e) {
            asserted = true;
            if (!expectAssertion) throw e;
        }
        if (BuildConfig.ENABLE_ASSERTS) assertEquals(expectAssertion, asserted);
    }

    @Test
    @SmallTest
    public void testIntentTargetsSelf() {
        String packageName = BuildInfo.getInstance().hostPackageName;
        assertTargetsSelf(false, new Intent(), false);
        assertTargetsSelf(
                true,
                new Intent(ContextUtils.getApplicationContext(), IntentUtilsTest.class),
                false);

        Intent intent = new Intent();
        intent.setComponent(new ComponentName(packageName, ""));
        assertTargetsSelf(true, intent, false);

        intent.setComponent(
                new ComponentName("other.package", "org.chromium.base.IntentUtilsTest"));
        assertTargetsSelf(false, intent, false);

        intent.setPackage(packageName);
        assertTargetsSelf(false, intent, true);

        intent.setComponent(null);
        assertTargetsSelf(true, intent, false);

        intent.setPackage("other.package");
        assertTargetsSelf(false, intent, false);

        intent.setComponent(new ComponentName(packageName, ""));
        assertTargetsSelf(false, intent, true);

        intent.setPackage(null);
        assertTargetsSelf(true, intent, false);
    }

    @Test
    @SmallTest
    public void testBundleSafeGetter() {
        final long defaultLong = 24L;
        Bundle bundle = new Bundle();
        bundle.putString("some_string", "chromium");
        bundle.putLong("some_long", 42L);

        assertEquals(defaultLong, IntentUtils.safeGetLong(bundle, "some_string", defaultLong));
        assertEquals(42L, IntentUtils.safeGetLong(bundle, "some_long", defaultLong));
    }
}
