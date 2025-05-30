// Copyright 2019 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.customtabs;

import androidx.annotation.IntDef;

import org.chromium.base.metrics.RecordHistogram;
import org.chromium.chrome.browser.browserservices.intents.BrowserServicesIntentDataProvider;
import org.chromium.chrome.browser.browserservices.intents.SessionHolder;
import org.chromium.chrome.browser.customtabs.content.CustomTabActivityTabProvider;
import org.chromium.chrome.browser.gsa.GSAUtils;
import org.chromium.chrome.browser.lifecycle.ActivityLifecycleDispatcher;
import org.chromium.chrome.browser.lifecycle.StartStopWithNativeObserver;
import org.chromium.chrome.browser.tab.Tab;
import org.chromium.chrome.browser.tab.TabAssociatedApp;

import java.lang.annotation.Retention;
import java.lang.annotation.RetentionPolicy;

/**
 * Keeps the client app alive, when possible, while CustomTabActivity is in foreground (see {@link
 * CustomTabsConnection#keepAliveForSession}).
 */
public class CustomTabActivityClientConnectionKeeper implements StartStopWithNativeObserver {
    @IntDef({
        ConnectionStatus.DISCONNECTED,
        ConnectionStatus.DISCONNECTED_KEEP_ALIVE,
        ConnectionStatus.CONNECTED,
        ConnectionStatus.CONNECTED_KEEP_ALIVE
    })
    @Retention(RetentionPolicy.SOURCE)
    private @interface ConnectionStatus {
        int DISCONNECTED = 0;
        int DISCONNECTED_KEEP_ALIVE = 1;
        int CONNECTED = 2;
        int CONNECTED_KEEP_ALIVE = 3;
        int NUM_ENTRIES = 4;
    }

    private final BrowserServicesIntentDataProvider mIntentDataProvider;
    private final CustomTabActivityTabProvider mTabProvider;

    private boolean mIsKeepingAlive;

    public CustomTabActivityClientConnectionKeeper(
            BrowserServicesIntentDataProvider intentDataProvider,
            CustomTabActivityTabProvider tabProvider,
            ActivityLifecycleDispatcher lifecycleDispatcher) {
        mIntentDataProvider = intentDataProvider;
        mTabProvider = tabProvider;
        lifecycleDispatcher.register(this);
    }

    @Override
    public void onStartWithNative() {
        mIsKeepingAlive =
                CustomTabsConnection.getInstance()
                        .keepAliveForSession(
                                mIntentDataProvider.getSession(),
                                mIntentDataProvider.getKeepAliveServiceIntent());
    }

    @Override
    public void onStopWithNative() {
        CustomTabsConnection.getInstance()
                .dontKeepAliveForSession(mIntentDataProvider.getSession());
        mIsKeepingAlive = false;
    }

    /** Records current client connection status. */
    public void recordClientConnectionStatus() {
        Tab tab = mTabProvider.getTab();
        String packageName = tab == null ? null : TabAssociatedApp.getAppId(tab);
        if (packageName == null) return; // No associated package

        SessionHolder<?> session = mIntentDataProvider.getSession();
        boolean isConnected =
                packageName.equals(
                        CustomTabsConnection.getInstance().getClientPackageNameForSession(session));
        int status = -1;
        if (isConnected) {
            if (mIsKeepingAlive) {
                status = ConnectionStatus.CONNECTED_KEEP_ALIVE;
            } else {
                status = ConnectionStatus.CONNECTED;
            }
        } else {
            if (mIsKeepingAlive) {
                status = ConnectionStatus.DISCONNECTED_KEEP_ALIVE;
            } else {
                status = ConnectionStatus.DISCONNECTED;
            }
        }
        assert status >= 0;

        if (GSAUtils.isGsaPackageName(packageName)) {
            RecordHistogram.recordEnumeratedHistogram(
                    "CustomTabs.ConnectionStatusOnReturn.GSA",
                    status,
                    ConnectionStatus.NUM_ENTRIES);
        } else {
            RecordHistogram.recordEnumeratedHistogram(
                    "CustomTabs.ConnectionStatusOnReturn.NonGSA",
                    status,
                    ConnectionStatus.NUM_ENTRIES);
        }
    }
}
