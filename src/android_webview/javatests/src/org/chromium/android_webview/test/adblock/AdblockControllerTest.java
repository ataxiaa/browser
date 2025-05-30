/*
 * This file is part of eyeo Chromium SDK,
 * Copyright (C) 2006-present eyeo GmbH
 *
 * eyeo Chromium SDK is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 3 as
 * published by the Free Software Foundation.
 *
 * eyeo Chromium SDK is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with eyeo Chromium SDK.  If not, see <http://www.gnu.org/licenses/>.
 */

package org.chromium.android_webview.test.adblock;

import org.junit.Before;
import org.junit.Rule;
import org.junit.runner.RunWith;

import org.chromium.android_webview.AwBrowserContext;
import org.chromium.android_webview.AwContents;
import org.chromium.android_webview.test.AwActivityTestRule;
import org.chromium.android_webview.test.AwJUnit4ClassRunner;
import org.chromium.android_webview.test.TestAwContentsClient;
import org.chromium.base.ThreadUtils;
import org.chromium.components.adblock.AdblockController;
import org.chromium.components.adblock.AdblockControllerTestBase;

@RunWith(AwJUnit4ClassRunner.class)
public class AdblockControllerTest extends AdblockControllerTestBase {
    @Rule public AwActivityTestRule mActivityTestRule = new AwActivityTestRule();
    private TestAwContentsClient mContentsClient;
    private AwContents mAwContents;

    @Before
    public void setUp() {
        mContentsClient = new TestAwContentsClient();
        mAwContents =
                mActivityTestRule
                        .createAwTestContainerViewOnMainSync(mContentsClient)
                        .getAwContents();
        ThreadUtils.runOnUiThreadBlocking(
                () -> {
                    mAdblockController =
                            AdblockController.getInstance(AwBrowserContext.getDefault());
                });
    }
}
