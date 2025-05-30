// Copyright 2022 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.chrome.browser.privacy_guide;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;
import static org.mockito.ArgumentMatchers.eq;
import static org.mockito.Mockito.times;
import static org.mockito.Mockito.verify;
import static org.mockito.Mockito.when;

import android.os.Bundle;
import android.widget.TextView;

import androidx.fragment.app.testing.FragmentScenario;

import org.junit.After;
import org.junit.Before;
import org.junit.Rule;
import org.junit.Test;
import org.junit.runner.RunWith;
import org.mockito.Mock;
import org.mockito.junit.MockitoJUnit;
import org.mockito.junit.MockitoRule;

import org.chromium.base.test.BaseRobolectricTestRunner;
import org.chromium.base.test.util.UserActionTester;
import org.chromium.chrome.browser.sync.SyncServiceFactory;
import org.chromium.components.browser_ui.widget.MaterialSwitchWithText;
import org.chromium.components.sync.SyncService;
import org.chromium.components.sync.UserSelectableType;

import java.util.HashSet;
import java.util.Set;

/** JUnit tests of the class {@link HistorySyncFragment}. */
@RunWith(BaseRobolectricTestRunner.class)
public class HistorySyncFragmentTest {
    private static final String CHANGE_HISTORY_SYNC_ON_USER_ACTION =
            "Settings.PrivacyGuide.ChangeHistorySyncOn";
    private static final String CHANGE_HISTORY_SYNC_OFF_USER_ACTION =
            "Settings.PrivacyGuide.ChangeHistorySyncOff";

    @Rule public MockitoRule mMockitoRule = MockitoJUnit.rule();

    @Mock private SyncService mSyncService;

    private FragmentScenario mScenario;
    private MaterialSwitchWithText mHistorySyncButton;
    private final UserActionTester mActionTester = new UserActionTester();

    @Before
    public void setUp() {
        SyncServiceFactory.setInstanceForTesting(mSyncService);
    }

    @After
    public void tearDown() {
        if (mScenario != null) {
            mScenario.close();
        }
        mActionTester.tearDown();
    }

    private void initFragmentWithSyncState(boolean historySync) {
        initSyncState(historySync);
        mScenario =
                FragmentScenario.launchInContainer(
                        HistorySyncFragment.class, Bundle.EMPTY, R.style.Theme_MaterialComponents);
        mScenario.onFragment(
                fragment ->
                        mHistorySyncButton =
                                fragment.getView().findViewById(R.id.history_sync_switch));
    }

    private void initSyncState(boolean historySync) {
        Set<Integer> syncTypes = new HashSet<>();
        if (historySync) {
            syncTypes.add(UserSelectableType.HISTORY);
        }
        when(mSyncService.getSelectedTypes()).thenReturn(syncTypes);
    }

    @Test
    public void testIsSwitchOffWhenHistorySyncOff() {
        initFragmentWithSyncState(false);
        assertFalse(mHistorySyncButton.isChecked());
    }

    @Test
    public void testIsSwitchOnWhenHistorySyncOnSyncAllOff() {
        initFragmentWithSyncState(true);
        assertTrue(mHistorySyncButton.isChecked());
    }

    @Test
    public void testIsSwitchOnWhenHistorySyncOnSyncAllOn() {
        initFragmentWithSyncState(true);
        assertTrue(mHistorySyncButton.isChecked());
    }

    @Test
    public void testToggleOn() throws Exception {
        initFragmentWithSyncState(false);
        assertFalse(mHistorySyncButton.isChecked());

        mHistorySyncButton.performClick();
        verify(mSyncService, times(1)).setSelectedType(eq(UserSelectableType.HISTORY), eq(true));
        verify(mSyncService, times(1)).setSelectedType(eq(UserSelectableType.TABS), eq(true));
    }

    @Test
    public void testToggleOff() throws Exception {
        initFragmentWithSyncState(true);
        assertTrue(mHistorySyncButton.isChecked());

        mHistorySyncButton.performClick();
        verify(mSyncService, times(1)).setSelectedType(eq(UserSelectableType.HISTORY), eq(false));
        verify(mSyncService, times(1)).setSelectedType(eq(UserSelectableType.TABS), eq(false));
    }

    @Test
    public void testTurnHistorySyncOn_changeHistorySyncOnUserAction() {
        initFragmentWithSyncState(false);
        mHistorySyncButton.performClick();
        assertTrue(mActionTester.getActions().contains(CHANGE_HISTORY_SYNC_ON_USER_ACTION));
    }

    @Test
    public void testTurnHistorySyncOffWhenSyncAllOn_changeHistorySyncOffUserAction() {
        initFragmentWithSyncState(true);
        mHistorySyncButton.performClick();
        assertTrue(mActionTester.getActions().contains(CHANGE_HISTORY_SYNC_OFF_USER_ACTION));
    }

    @Test
    public void testTurnHistorySyncOffWhenSyncAllOff_changeHistorySyncOffUserAction() {
        initFragmentWithSyncState(true);
        mHistorySyncButton.performClick();
        assertTrue(mActionTester.getActions().contains(CHANGE_HISTORY_SYNC_OFF_USER_ACTION));
    }

    @Test
    public void testTurnHistorySyncOffThenOnWhenSyncAllOn_changeHistorySyncOffOnUserAction() {
        initFragmentWithSyncState(true);
        mHistorySyncButton.performClick();
        assertTrue(mActionTester.getActions().contains(CHANGE_HISTORY_SYNC_OFF_USER_ACTION));
        mHistorySyncButton.performClick();
        assertTrue(mActionTester.getActions().contains(CHANGE_HISTORY_SYNC_ON_USER_ACTION));
    }

    @Test
    public void testTurnHistorySyncOffThenOnWhenSyncAllOff_changeHistorySyncOffOnUserAction() {
        initFragmentWithSyncState(true);
        mHistorySyncButton.performClick();
        assertTrue(mActionTester.getActions().contains(CHANGE_HISTORY_SYNC_OFF_USER_ACTION));
        mHistorySyncButton.performClick();
        assertTrue(mActionTester.getActions().contains(CHANGE_HISTORY_SYNC_ON_USER_ACTION));
    }

    @Test
    public void testStrings() throws Exception {
        initSyncState(true);
        mScenario =
                FragmentScenario.launchInContainer(
                        HistorySyncFragment.class, Bundle.EMPTY, R.style.Theme_MaterialComponents);
        mScenario.onFragment(
                fragment -> {
                    mHistorySyncButton = fragment.getView().findViewById(R.id.history_sync_switch);
                    assertEquals(
                            ((TextView) mHistorySyncButton.findViewById(R.id.switch_text))
                                    .getText(),
                            fragment.getContext()
                                    .getString(
                                            R.string.privacy_guide_history_and_tabs_sync_toggle));
                    assertEquals(
                            ((PrivacyGuideExplanationItem)
                                            fragment.getView()
                                                    .findViewById(R.id.history_sync_item_one))
                                    .getSummaryTextForTesting(),
                            fragment.getContext()
                                    .getString(
                                            R.string.privacy_guide_history_and_tabs_sync_item_one));
                });
    }

    @Test
    public void testToggleAccountsForTabsSync() throws Exception {
        Set<Integer> syncTypes = new HashSet<>();
        syncTypes.add(UserSelectableType.TABS);
        when(mSyncService.getSelectedTypes()).thenReturn(syncTypes);
        mScenario =
                FragmentScenario.launchInContainer(
                        HistorySyncFragment.class, Bundle.EMPTY, R.style.Theme_MaterialComponents);
        mScenario.onFragment(
                fragment -> {
                    mHistorySyncButton = fragment.getView().findViewById(R.id.history_sync_switch);
                    assertTrue(mHistorySyncButton.isChecked());
                });
    }
}
