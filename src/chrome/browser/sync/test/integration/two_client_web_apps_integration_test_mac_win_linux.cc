// Copyright 2021 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "build/build_config.h"
#include "chrome/browser/sync/test/integration/two_client_web_apps_integration_test_base.h"
#include "content/public/test/browser_test.h"

namespace web_app::integration_tests {

// This test is a part of the web app integration test suite, which is
// documented in //chrome/browser/ui/views/web_apps/README.md. For information
// about diagnosing, debugging and/or disabling tests, please look to the
// README file.

namespace {

using WebAppIntegration = TwoClientWebAppsIntegrationTestBase;

IN_PROC_BROWSER_TEST_F(
    WebAppIntegration,
    WAI_31Standalone_79StandaloneStandaloneOriginal_24_12Standalone_7Standalone_112StandaloneNotShown_40Client2_45Standalone_46Standalone_7Standalone_12Standalone_37Standalone_17_20) {
  // Test contents are generated by script. Please do not modify!
  // See `docs/webapps/why-is-this-test-failing.md` or
  // `docs/webapps/integration-testing-framework` for more info.
  // Gardeners: Disabling this test is supported.
  helper_.InstallOmniboxIcon(InstallableSite::kStandalone);
  helper_.CheckAppTitle(Site::kStandalone, Title::kStandaloneOriginal);
  helper_.CheckWindowCreated();
  helper_.CheckAppInListWindowed(Site::kStandalone);
  helper_.CheckPlatformShortcutAndIcon(Site::kStandalone);
  helper_.CheckWindowControlsOverlayToggle(Site::kStandalone,
                                           IsShown::kNotShown);
  helper_.SwitchProfileClients(ProfileClient::kClient2);
  helper_.CheckAppInListNotLocallyInstalled(Site::kStandalone);
  helper_.InstallLocally(Site::kStandalone);
  helper_.CheckPlatformShortcutAndIcon(Site::kStandalone);
  helper_.CheckAppInListWindowed(Site::kStandalone);
  helper_.NavigateBrowser(Site::kStandalone);
  helper_.CheckInstallIconNotShown();
  helper_.CheckLaunchIconShown();
}

IN_PROC_BROWSER_TEST_F(
    WebAppIntegration,
    WAI_31Standalone_79StandaloneStandaloneOriginal_24_12Standalone_7Standalone_112StandaloneNotShown_40Client2_45Standalone_46Standalone_7Standalone_12Standalone_69Standalone_24) {
  // Test contents are generated by script. Please do not modify!
  // See `docs/webapps/why-is-this-test-failing.md` or
  // `docs/webapps/integration-testing-framework` for more info.
  // Gardeners: Disabling this test is supported.
  helper_.InstallOmniboxIcon(InstallableSite::kStandalone);
  helper_.CheckAppTitle(Site::kStandalone, Title::kStandaloneOriginal);
  helper_.CheckWindowCreated();
  helper_.CheckAppInListWindowed(Site::kStandalone);
  helper_.CheckPlatformShortcutAndIcon(Site::kStandalone);
  helper_.CheckWindowControlsOverlayToggle(Site::kStandalone,
                                           IsShown::kNotShown);
  helper_.SwitchProfileClients(ProfileClient::kClient2);
  helper_.CheckAppInListNotLocallyInstalled(Site::kStandalone);
  helper_.InstallLocally(Site::kStandalone);
  helper_.CheckPlatformShortcutAndIcon(Site::kStandalone);
  helper_.CheckAppInListWindowed(Site::kStandalone);
  helper_.LaunchFromMenuOption(Site::kStandalone);
  helper_.CheckWindowCreated();
}

IN_PROC_BROWSER_TEST_F(
    WebAppIntegration,
    WAI_31Standalone_79StandaloneStandaloneOriginal_24_12Standalone_7Standalone_112StandaloneNotShown_40Client2_45Standalone_46Standalone_7Standalone_12Standalone_35Standalone_24) {
  // Test contents are generated by script. Please do not modify!
  // See `docs/webapps/why-is-this-test-failing.md` or
  // `docs/webapps/integration-testing-framework` for more info.
  // Gardeners: Disabling this test is supported.
  helper_.InstallOmniboxIcon(InstallableSite::kStandalone);
  helper_.CheckAppTitle(Site::kStandalone, Title::kStandaloneOriginal);
  helper_.CheckWindowCreated();
  helper_.CheckAppInListWindowed(Site::kStandalone);
  helper_.CheckPlatformShortcutAndIcon(Site::kStandalone);
  helper_.CheckWindowControlsOverlayToggle(Site::kStandalone,
                                           IsShown::kNotShown);
  helper_.SwitchProfileClients(ProfileClient::kClient2);
  helper_.CheckAppInListNotLocallyInstalled(Site::kStandalone);
  helper_.InstallLocally(Site::kStandalone);
  helper_.CheckPlatformShortcutAndIcon(Site::kStandalone);
  helper_.CheckAppInListWindowed(Site::kStandalone);
  helper_.LaunchFromLaunchIcon(Site::kStandalone);
  helper_.CheckWindowCreated();
}

IN_PROC_BROWSER_TEST_F(
    WebAppIntegration,
    WAI_31Standalone_79StandaloneStandaloneOriginal_24_12Standalone_7Standalone_112StandaloneNotShown_40Client2_45Standalone_46Standalone_7Standalone_12Standalone_34Standalone_24) {
  // Test contents are generated by script. Please do not modify!
  // See `docs/webapps/why-is-this-test-failing.md` or
  // `docs/webapps/integration-testing-framework` for more info.
  // Gardeners: Disabling this test is supported.
  helper_.InstallOmniboxIcon(InstallableSite::kStandalone);
  helper_.CheckAppTitle(Site::kStandalone, Title::kStandaloneOriginal);
  helper_.CheckWindowCreated();
  helper_.CheckAppInListWindowed(Site::kStandalone);
  helper_.CheckPlatformShortcutAndIcon(Site::kStandalone);
  helper_.CheckWindowControlsOverlayToggle(Site::kStandalone,
                                           IsShown::kNotShown);
  helper_.SwitchProfileClients(ProfileClient::kClient2);
  helper_.CheckAppInListNotLocallyInstalled(Site::kStandalone);
  helper_.InstallLocally(Site::kStandalone);
  helper_.CheckPlatformShortcutAndIcon(Site::kStandalone);
  helper_.CheckAppInListWindowed(Site::kStandalone);
  helper_.LaunchFromChromeApps(Site::kStandalone);
  helper_.CheckWindowCreated();
}

IN_PROC_BROWSER_TEST_F(
    WebAppIntegration,
    WAI_31Standalone_79StandaloneStandaloneOriginal_24_12Standalone_7Standalone_112StandaloneNotShown_40Client2_45Standalone_10Standalone_15Standalone_40Client1_15Standalone) {
  // Test contents are generated by script. Please do not modify!
  // See `docs/webapps/why-is-this-test-failing.md` or
  // `docs/webapps/integration-testing-framework` for more info.
  // Gardeners: Disabling this test is supported.
  helper_.InstallOmniboxIcon(InstallableSite::kStandalone);
  helper_.CheckAppTitle(Site::kStandalone, Title::kStandaloneOriginal);
  helper_.CheckWindowCreated();
  helper_.CheckAppInListWindowed(Site::kStandalone);
  helper_.CheckPlatformShortcutAndIcon(Site::kStandalone);
  helper_.CheckWindowControlsOverlayToggle(Site::kStandalone,
                                           IsShown::kNotShown);
  helper_.SwitchProfileClients(ProfileClient::kClient2);
  helper_.CheckAppInListNotLocallyInstalled(Site::kStandalone);
  helper_.UninstallFromList(Site::kStandalone);
  helper_.CheckAppNotInList(Site::kStandalone);
  helper_.SwitchProfileClients(ProfileClient::kClient1);
  helper_.CheckAppNotInList(Site::kStandalone);
}

IN_PROC_BROWSER_TEST_F(
    WebAppIntegration,
    WAI_31Standalone_79StandaloneStandaloneOriginal_24_12Standalone_7Standalone_112StandaloneNotShown_40Client2_45Standalone_37Standalone_18_19) {
  // Test contents are generated by script. Please do not modify!
  // See `docs/webapps/why-is-this-test-failing.md` or
  // `docs/webapps/integration-testing-framework` for more info.
  // Gardeners: Disabling this test is supported.
  helper_.InstallOmniboxIcon(InstallableSite::kStandalone);
  helper_.CheckAppTitle(Site::kStandalone, Title::kStandaloneOriginal);
  helper_.CheckWindowCreated();
  helper_.CheckAppInListWindowed(Site::kStandalone);
  helper_.CheckPlatformShortcutAndIcon(Site::kStandalone);
  helper_.CheckWindowControlsOverlayToggle(Site::kStandalone,
                                           IsShown::kNotShown);
  helper_.SwitchProfileClients(ProfileClient::kClient2);
  helper_.CheckAppInListNotLocallyInstalled(Site::kStandalone);
  helper_.NavigateBrowser(Site::kStandalone);
  helper_.CheckInstallIconShown();
  helper_.CheckLaunchIconNotShown();
}

IN_PROC_BROWSER_TEST_F(
    WebAppIntegration,
    WAI_InstallOmniboxIconStandalone_SwitchProfileClientsClient2_SyncTurnOff_SyncTurnOn) {
  // Test contents are generated by script. Please do not modify!
  // See `docs/webapps/why-is-this-test-failing.md` or
  // `docs/webapps/integration-testing-framework` for more info.
  // Gardeners: Disabling this test is supported.
  helper_.InstallOmniboxIcon(InstallableSite::kStandalone);
  helper_.CheckAppTitle(Site::kStandalone, Title::kStandaloneOriginal);
  helper_.CheckWindowCreated();
  helper_.CheckAppInListWindowed(Site::kStandalone);
  helper_.CheckPlatformShortcutAndIcon(Site::kStandalone);
  helper_.CheckWindowControlsOverlayToggle(Site::kStandalone,
                                           IsShown::kNotShown);
  helper_.SwitchProfileClients(ProfileClient::kClient2);
  helper_.CheckAppInListNotLocallyInstalled(Site::kStandalone);
  helper_.SyncTurnOff();
  helper_.CheckAppNotInList(Site::kStandalone);
  helper_.SyncTurnOn();
  helper_.CheckAppInListNotLocallyInstalled(Site::kStandalone);
}

IN_PROC_BROWSER_TEST_F(
    WebAppIntegration,
    WAI_InstallOmniboxIconStandalone_SwitchProfileClientsClient2_SwitchProfileClientsClient1_SyncTurnOff_UninstallFromListStandalone_SwitchProfileClientsClient2) {
  // Test contents are generated by script. Please do not modify!
  // See `docs/webapps/why-is-this-test-failing.md` or
  // `docs/webapps/integration-testing-framework` for more info.
  // Gardeners: Disabling this test is supported.
  helper_.InstallOmniboxIcon(InstallableSite::kStandalone);
  helper_.CheckAppTitle(Site::kStandalone, Title::kStandaloneOriginal);
  helper_.CheckWindowCreated();
  helper_.CheckAppInListWindowed(Site::kStandalone);
  helper_.CheckPlatformShortcutAndIcon(Site::kStandalone);
  helper_.CheckWindowControlsOverlayToggle(Site::kStandalone,
                                           IsShown::kNotShown);
  helper_.SwitchProfileClients(ProfileClient::kClient2);
  helper_.CheckAppInListNotLocallyInstalled(Site::kStandalone);
  helper_.SwitchProfileClients(ProfileClient::kClient1);
  helper_.SyncTurnOff();
  helper_.UninstallFromList(Site::kStandalone);
  helper_.SwitchProfileClients(ProfileClient::kClient2);
  helper_.CheckAppNotInList(Site::kStandalone);
}

IN_PROC_BROWSER_TEST_F(
    WebAppIntegration,
    WAI_InstallOmniboxIconStandalone_SwitchProfileClientsClient2_SwitchProfileClientsClient1_SyncTurnOff_UninstallFromMenuStandalone_SwitchProfileClientsClient2) {
  // Test contents are generated by script. Please do not modify!
  // See `docs/webapps/why-is-this-test-failing.md` or
  // `docs/webapps/integration-testing-framework` for more info.
  // Gardeners: Disabling this test is supported.
  helper_.InstallOmniboxIcon(InstallableSite::kStandalone);
  helper_.CheckAppTitle(Site::kStandalone, Title::kStandaloneOriginal);
  helper_.CheckWindowCreated();
  helper_.CheckAppInListWindowed(Site::kStandalone);
  helper_.CheckPlatformShortcutAndIcon(Site::kStandalone);
  helper_.CheckWindowControlsOverlayToggle(Site::kStandalone,
                                           IsShown::kNotShown);
  helper_.SwitchProfileClients(ProfileClient::kClient2);
  helper_.CheckAppInListNotLocallyInstalled(Site::kStandalone);
  helper_.SwitchProfileClients(ProfileClient::kClient1);
  helper_.SyncTurnOff();
  helper_.UninstallFromMenu(Site::kStandalone);
  helper_.SwitchProfileClients(ProfileClient::kClient2);
  helper_.CheckAppNotInList(Site::kStandalone);
}

IN_PROC_BROWSER_TEST_F(
    WebAppIntegration,
    WAI_InstallOmniboxIconStandalone_SwitchProfileClientsClient2_SwitchProfileClientsClient1_SyncTurnOff_UninstallFromAppSettingsStandalone_SwitchProfileClientsClient2) {
  // Test contents are generated by script. Please do not modify!
  // See `docs/webapps/why-is-this-test-failing.md` or
  // `docs/webapps/integration-testing-framework` for more info.
  // Gardeners: Disabling this test is supported.
  helper_.InstallOmniboxIcon(InstallableSite::kStandalone);
  helper_.CheckAppTitle(Site::kStandalone, Title::kStandaloneOriginal);
  helper_.CheckWindowCreated();
  helper_.CheckAppInListWindowed(Site::kStandalone);
  helper_.CheckPlatformShortcutAndIcon(Site::kStandalone);
  helper_.CheckWindowControlsOverlayToggle(Site::kStandalone,
                                           IsShown::kNotShown);
  helper_.SwitchProfileClients(ProfileClient::kClient2);
  helper_.CheckAppInListNotLocallyInstalled(Site::kStandalone);
  helper_.SwitchProfileClients(ProfileClient::kClient1);
  helper_.SyncTurnOff();
  helper_.UninstallFromAppSettings(Site::kStandalone);
  helper_.SwitchProfileClients(ProfileClient::kClient2);
  helper_.CheckAppNotInList(Site::kStandalone);
}

IN_PROC_BROWSER_TEST_F(
    WebAppIntegration,
    WAI_SyncSignOut_InstallOmniboxIconStandalone_SyncSignIn_SwitchProfileClientsClient2) {
  // Test contents are generated by script. Please do not modify!
  // See `docs/webapps/why-is-this-test-failing.md` or
  // `docs/webapps/integration-testing-framework` for more info.
  // Gardeners: Disabling this test is supported.
  helper_.SyncSignOut();
  helper_.InstallOmniboxIcon(InstallableSite::kStandalone);
  helper_.SyncSignIn();
  helper_.SwitchProfileClients(ProfileClient::kClient2);
  helper_.CheckAppNotInList(Site::kStandalone);
}

IN_PROC_BROWSER_TEST_F(
    WebAppIntegration,
    WAI_InstallOmniboxIconStandalone_SwitchProfileClientsClient2_SwitchProfileClientsClient2_ApplyRunOnOsLoginPolicyRunWindowedStandalone_InstallLocallyStandalone) {
  // Test contents are generated by script. Please do not modify!
  // See `docs/webapps/why-is-this-test-failing.md` or
  // `docs/webapps/integration-testing-framework` for more info.
  // Gardeners: Disabling this test is supported.
  helper_.InstallOmniboxIcon(InstallableSite::kStandalone);
  helper_.CheckAppTitle(Site::kStandalone, Title::kStandaloneOriginal);
  helper_.CheckWindowCreated();
  helper_.CheckAppInListWindowed(Site::kStandalone);
  helper_.CheckPlatformShortcutAndIcon(Site::kStandalone);
  helper_.CheckWindowControlsOverlayToggle(Site::kStandalone,
                                           IsShown::kNotShown);
  helper_.SwitchProfileClients(ProfileClient::kClient2);
  helper_.CheckAppInListNotLocallyInstalled(Site::kStandalone);
  helper_.SwitchProfileClients(ProfileClient::kClient2);
  helper_.ApplyRunOnOsLoginPolicyRunWindowed(Site::kStandalone);
  helper_.CheckRunOnOsLoginDisabled(Site::kStandalone);
  helper_.InstallLocally(Site::kStandalone);
  helper_.CheckRunOnOsLoginEnabled(Site::kStandalone);
}

IN_PROC_BROWSER_TEST_F(
    WebAppIntegration,
    WAI_InstallOmniboxIconStandalone_SwitchProfileClientsClient2_SyncTurnOff_SwitchProfileClientsClient1_UninstallFromListStandalone_SyncTurnOn_SwitchProfileClientsClient2) {
  // Test contents are generated by script. Please do not modify!
  // See `docs/webapps/why-is-this-test-failing.md` or
  // `docs/webapps/integration-testing-framework` for more info.
  // Gardeners: Disabling this test is supported.
  helper_.InstallOmniboxIcon(InstallableSite::kStandalone);
  helper_.CheckAppTitle(Site::kStandalone, Title::kStandaloneOriginal);
  helper_.CheckWindowCreated();
  helper_.CheckAppInListWindowed(Site::kStandalone);
  helper_.CheckPlatformShortcutAndIcon(Site::kStandalone);
  helper_.CheckWindowControlsOverlayToggle(Site::kStandalone,
                                           IsShown::kNotShown);
  helper_.SwitchProfileClients(ProfileClient::kClient2);
  helper_.CheckAppInListNotLocallyInstalled(Site::kStandalone);
  helper_.SyncTurnOff();
  helper_.CheckAppNotInList(Site::kStandalone);
  helper_.SwitchProfileClients(ProfileClient::kClient1);
  helper_.UninstallFromList(Site::kStandalone);
  helper_.SyncTurnOn();
  helper_.SwitchProfileClients(ProfileClient::kClient2);
  helper_.CheckAppInListNotLocallyInstalled(Site::kStandalone);
}

IN_PROC_BROWSER_TEST_F(
    WebAppIntegration,
    WAI_InstallOmniboxIconStandalone_SwitchProfileClientsClient2_SyncTurnOff_SwitchProfileClientsClient1_UninstallFromMenuStandalone_SyncTurnOn_SwitchProfileClientsClient2) {
  // Test contents are generated by script. Please do not modify!
  // See `docs/webapps/why-is-this-test-failing.md` or
  // `docs/webapps/integration-testing-framework` for more info.
  // Gardeners: Disabling this test is supported.
  helper_.InstallOmniboxIcon(InstallableSite::kStandalone);
  helper_.CheckAppTitle(Site::kStandalone, Title::kStandaloneOriginal);
  helper_.CheckWindowCreated();
  helper_.CheckAppInListWindowed(Site::kStandalone);
  helper_.CheckPlatformShortcutAndIcon(Site::kStandalone);
  helper_.CheckWindowControlsOverlayToggle(Site::kStandalone,
                                           IsShown::kNotShown);
  helper_.SwitchProfileClients(ProfileClient::kClient2);
  helper_.CheckAppInListNotLocallyInstalled(Site::kStandalone);
  helper_.SyncTurnOff();
  helper_.CheckAppNotInList(Site::kStandalone);
  helper_.SwitchProfileClients(ProfileClient::kClient1);
  helper_.UninstallFromMenu(Site::kStandalone);
  helper_.SyncTurnOn();
  helper_.SwitchProfileClients(ProfileClient::kClient2);
  helper_.CheckAppInListNotLocallyInstalled(Site::kStandalone);
}

IN_PROC_BROWSER_TEST_F(
    WebAppIntegration,
    WAI_InstallOmniboxIconStandalone_SwitchProfileClientsClient2_SyncTurnOff_SwitchProfileClientsClient1_UninstallFromAppSettingsStandalone_SyncTurnOn_SwitchProfileClientsClient2) {
  // Test contents are generated by script. Please do not modify!
  // See `docs/webapps/why-is-this-test-failing.md` or
  // `docs/webapps/integration-testing-framework` for more info.
  // Gardeners: Disabling this test is supported.
  helper_.InstallOmniboxIcon(InstallableSite::kStandalone);
  helper_.CheckAppTitle(Site::kStandalone, Title::kStandaloneOriginal);
  helper_.CheckWindowCreated();
  helper_.CheckAppInListWindowed(Site::kStandalone);
  helper_.CheckPlatformShortcutAndIcon(Site::kStandalone);
  helper_.CheckWindowControlsOverlayToggle(Site::kStandalone,
                                           IsShown::kNotShown);
  helper_.SwitchProfileClients(ProfileClient::kClient2);
  helper_.CheckAppInListNotLocallyInstalled(Site::kStandalone);
  helper_.SyncTurnOff();
  helper_.CheckAppNotInList(Site::kStandalone);
  helper_.SwitchProfileClients(ProfileClient::kClient1);
  helper_.UninstallFromAppSettings(Site::kStandalone);
  helper_.SyncTurnOn();
  helper_.SwitchProfileClients(ProfileClient::kClient2);
  helper_.CheckAppInListNotLocallyInstalled(Site::kStandalone);
}

IN_PROC_BROWSER_TEST_F(
    WebAppIntegration,
    WAI_InstallMenuStandalone_SwitchProfileClientsClient2_SwitchProfileClientsClient1_SyncTurnOff_UninstallFromListStandalone_SwitchProfileClientsClient2) {
  // Test contents are generated by script. Please do not modify!
  // See `docs/webapps/why-is-this-test-failing.md` or
  // `docs/webapps/integration-testing-framework` for more info.
  // Gardeners: Disabling this test is supported.
  helper_.InstallMenuOption(Site::kStandalone);
  helper_.CheckAppTitle(Site::kStandalone, Title::kStandaloneOriginal);
  helper_.CheckWindowCreated();
  helper_.CheckAppInListWindowed(Site::kStandalone);
  helper_.CheckPlatformShortcutAndIcon(Site::kStandalone);
  helper_.CheckWindowControlsOverlayToggle(Site::kStandalone,
                                           IsShown::kNotShown);
  helper_.SwitchProfileClients(ProfileClient::kClient2);
  helper_.CheckAppInListNotLocallyInstalled(Site::kStandalone);
  helper_.SwitchProfileClients(ProfileClient::kClient1);
  helper_.SyncTurnOff();
  helper_.UninstallFromList(Site::kStandalone);
  helper_.SwitchProfileClients(ProfileClient::kClient2);
  helper_.CheckAppNotInList(Site::kStandalone);
}

IN_PROC_BROWSER_TEST_F(
    WebAppIntegration,
    WAI_InstallMenuStandalone_SwitchProfileClientsClient2_SwitchProfileClientsClient1_SyncTurnOff_UninstallFromMenuStandalone_SwitchProfileClientsClient2) {
  // Test contents are generated by script. Please do not modify!
  // See `docs/webapps/why-is-this-test-failing.md` or
  // `docs/webapps/integration-testing-framework` for more info.
  // Gardeners: Disabling this test is supported.
  helper_.InstallMenuOption(Site::kStandalone);
  helper_.CheckAppTitle(Site::kStandalone, Title::kStandaloneOriginal);
  helper_.CheckWindowCreated();
  helper_.CheckAppInListWindowed(Site::kStandalone);
  helper_.CheckPlatformShortcutAndIcon(Site::kStandalone);
  helper_.CheckWindowControlsOverlayToggle(Site::kStandalone,
                                           IsShown::kNotShown);
  helper_.SwitchProfileClients(ProfileClient::kClient2);
  helper_.CheckAppInListNotLocallyInstalled(Site::kStandalone);
  helper_.SwitchProfileClients(ProfileClient::kClient1);
  helper_.SyncTurnOff();
  helper_.UninstallFromMenu(Site::kStandalone);
  helper_.SwitchProfileClients(ProfileClient::kClient2);
  helper_.CheckAppNotInList(Site::kStandalone);
}

IN_PROC_BROWSER_TEST_F(
    WebAppIntegration,
    WAI_InstallMenuStandalone_SwitchProfileClientsClient2_SwitchProfileClientsClient1_SyncTurnOff_UninstallFromAppSettingsStandalone_SwitchProfileClientsClient2) {
  // Test contents are generated by script. Please do not modify!
  // See `docs/webapps/why-is-this-test-failing.md` or
  // `docs/webapps/integration-testing-framework` for more info.
  // Gardeners: Disabling this test is supported.
  helper_.InstallMenuOption(Site::kStandalone);
  helper_.CheckAppTitle(Site::kStandalone, Title::kStandaloneOriginal);
  helper_.CheckWindowCreated();
  helper_.CheckAppInListWindowed(Site::kStandalone);
  helper_.CheckPlatformShortcutAndIcon(Site::kStandalone);
  helper_.CheckWindowControlsOverlayToggle(Site::kStandalone,
                                           IsShown::kNotShown);
  helper_.SwitchProfileClients(ProfileClient::kClient2);
  helper_.CheckAppInListNotLocallyInstalled(Site::kStandalone);
  helper_.SwitchProfileClients(ProfileClient::kClient1);
  helper_.SyncTurnOff();
  helper_.UninstallFromAppSettings(Site::kStandalone);
  helper_.SwitchProfileClients(ProfileClient::kClient2);
  helper_.CheckAppNotInList(Site::kStandalone);
}

IN_PROC_BROWSER_TEST_F(
    WebAppIntegration,
    WAI_InstallMenuStandalone_SwitchProfileClientsClient2_SwitchProfileClientsClient2_ApplyRunOnOsLoginPolicyRunWindowedStandalone_InstallLocallyStandalone) {
  // Test contents are generated by script. Please do not modify!
  // See `docs/webapps/why-is-this-test-failing.md` or
  // `docs/webapps/integration-testing-framework` for more info.
  // Gardeners: Disabling this test is supported.
  helper_.InstallMenuOption(Site::kStandalone);
  helper_.CheckAppTitle(Site::kStandalone, Title::kStandaloneOriginal);
  helper_.CheckWindowCreated();
  helper_.CheckAppInListWindowed(Site::kStandalone);
  helper_.CheckPlatformShortcutAndIcon(Site::kStandalone);
  helper_.CheckWindowControlsOverlayToggle(Site::kStandalone,
                                           IsShown::kNotShown);
  helper_.SwitchProfileClients(ProfileClient::kClient2);
  helper_.CheckAppInListNotLocallyInstalled(Site::kStandalone);
  helper_.SwitchProfileClients(ProfileClient::kClient2);
  helper_.ApplyRunOnOsLoginPolicyRunWindowed(Site::kStandalone);
  helper_.CheckRunOnOsLoginDisabled(Site::kStandalone);
  helper_.InstallLocally(Site::kStandalone);
  helper_.CheckRunOnOsLoginEnabled(Site::kStandalone);
}

IN_PROC_BROWSER_TEST_F(
    WebAppIntegration,
    WAI_InstallMenuStandalone_SwitchProfileClientsClient2_InstallLocallyStandalone_NavigateBrowserStandalone) {
  // Test contents are generated by script. Please do not modify!
  // See `docs/webapps/why-is-this-test-failing.md` or
  // `docs/webapps/integration-testing-framework` for more info.
  // Gardeners: Disabling this test is supported.
  helper_.InstallMenuOption(Site::kStandalone);
  helper_.CheckAppTitle(Site::kStandalone, Title::kStandaloneOriginal);
  helper_.CheckWindowCreated();
  helper_.CheckAppInListWindowed(Site::kStandalone);
  helper_.CheckPlatformShortcutAndIcon(Site::kStandalone);
  helper_.CheckWindowControlsOverlayToggle(Site::kStandalone,
                                           IsShown::kNotShown);
  helper_.SwitchProfileClients(ProfileClient::kClient2);
  helper_.CheckAppInListNotLocallyInstalled(Site::kStandalone);
  helper_.InstallLocally(Site::kStandalone);
  helper_.CheckPlatformShortcutAndIcon(Site::kStandalone);
  helper_.CheckAppInListWindowed(Site::kStandalone);
  helper_.NavigateBrowser(Site::kStandalone);
  helper_.CheckInstallIconNotShown();
  helper_.CheckLaunchIconShown();
}

IN_PROC_BROWSER_TEST_F(
    WebAppIntegration,
    WAI_InstallMenuStandalone_SwitchProfileClientsClient2_InstallLocallyStandalone_LaunchFromMenuOptionStandalone) {
  // Test contents are generated by script. Please do not modify!
  // See `docs/webapps/why-is-this-test-failing.md` or
  // `docs/webapps/integration-testing-framework` for more info.
  // Gardeners: Disabling this test is supported.
  helper_.InstallMenuOption(Site::kStandalone);
  helper_.CheckAppTitle(Site::kStandalone, Title::kStandaloneOriginal);
  helper_.CheckWindowCreated();
  helper_.CheckAppInListWindowed(Site::kStandalone);
  helper_.CheckPlatformShortcutAndIcon(Site::kStandalone);
  helper_.CheckWindowControlsOverlayToggle(Site::kStandalone,
                                           IsShown::kNotShown);
  helper_.SwitchProfileClients(ProfileClient::kClient2);
  helper_.CheckAppInListNotLocallyInstalled(Site::kStandalone);
  helper_.InstallLocally(Site::kStandalone);
  helper_.CheckPlatformShortcutAndIcon(Site::kStandalone);
  helper_.CheckAppInListWindowed(Site::kStandalone);
  helper_.LaunchFromMenuOption(Site::kStandalone);
  helper_.CheckWindowCreated();
}

IN_PROC_BROWSER_TEST_F(
    WebAppIntegration,
    WAI_InstallMenuStandalone_SwitchProfileClientsClient2_InstallLocallyStandalone_LaunchFromLaunchIconStandalone) {
  // Test contents are generated by script. Please do not modify!
  // See `docs/webapps/why-is-this-test-failing.md` or
  // `docs/webapps/integration-testing-framework` for more info.
  // Gardeners: Disabling this test is supported.
  helper_.InstallMenuOption(Site::kStandalone);
  helper_.CheckAppTitle(Site::kStandalone, Title::kStandaloneOriginal);
  helper_.CheckWindowCreated();
  helper_.CheckAppInListWindowed(Site::kStandalone);
  helper_.CheckPlatformShortcutAndIcon(Site::kStandalone);
  helper_.CheckWindowControlsOverlayToggle(Site::kStandalone,
                                           IsShown::kNotShown);
  helper_.SwitchProfileClients(ProfileClient::kClient2);
  helper_.CheckAppInListNotLocallyInstalled(Site::kStandalone);
  helper_.InstallLocally(Site::kStandalone);
  helper_.CheckPlatformShortcutAndIcon(Site::kStandalone);
  helper_.CheckAppInListWindowed(Site::kStandalone);
  helper_.LaunchFromLaunchIcon(Site::kStandalone);
  helper_.CheckWindowCreated();
}

IN_PROC_BROWSER_TEST_F(
    WebAppIntegration,
    WAI_InstallMenuStandalone_SwitchProfileClientsClient2_InstallLocallyStandalone_LaunchFromChromeAppsStandalone) {
  // Test contents are generated by script. Please do not modify!
  // See `docs/webapps/why-is-this-test-failing.md` or
  // `docs/webapps/integration-testing-framework` for more info.
  // Gardeners: Disabling this test is supported.
  helper_.InstallMenuOption(Site::kStandalone);
  helper_.CheckAppTitle(Site::kStandalone, Title::kStandaloneOriginal);
  helper_.CheckWindowCreated();
  helper_.CheckAppInListWindowed(Site::kStandalone);
  helper_.CheckPlatformShortcutAndIcon(Site::kStandalone);
  helper_.CheckWindowControlsOverlayToggle(Site::kStandalone,
                                           IsShown::kNotShown);
  helper_.SwitchProfileClients(ProfileClient::kClient2);
  helper_.CheckAppInListNotLocallyInstalled(Site::kStandalone);
  helper_.InstallLocally(Site::kStandalone);
  helper_.CheckPlatformShortcutAndIcon(Site::kStandalone);
  helper_.CheckAppInListWindowed(Site::kStandalone);
  helper_.LaunchFromChromeApps(Site::kStandalone);
  helper_.CheckWindowCreated();
}

IN_PROC_BROWSER_TEST_F(
    WebAppIntegration,
    WAI_InstallMenuStandalone_SwitchProfileClientsClient2_UninstallFromListStandalone_SwitchProfileClientsClient1) {
  // Test contents are generated by script. Please do not modify!
  // See `docs/webapps/why-is-this-test-failing.md` or
  // `docs/webapps/integration-testing-framework` for more info.
  // Gardeners: Disabling this test is supported.
  helper_.InstallMenuOption(Site::kStandalone);
  helper_.CheckAppTitle(Site::kStandalone, Title::kStandaloneOriginal);
  helper_.CheckWindowCreated();
  helper_.CheckAppInListWindowed(Site::kStandalone);
  helper_.CheckPlatformShortcutAndIcon(Site::kStandalone);
  helper_.CheckWindowControlsOverlayToggle(Site::kStandalone,
                                           IsShown::kNotShown);
  helper_.SwitchProfileClients(ProfileClient::kClient2);
  helper_.CheckAppInListNotLocallyInstalled(Site::kStandalone);
  helper_.UninstallFromList(Site::kStandalone);
  helper_.CheckAppNotInList(Site::kStandalone);
  helper_.SwitchProfileClients(ProfileClient::kClient1);
  helper_.CheckAppNotInList(Site::kStandalone);
}

IN_PROC_BROWSER_TEST_F(
    WebAppIntegration,
    WAI_InstallMenuStandalone_SwitchProfileClientsClient2_NavigateBrowserStandalone) {
  // Test contents are generated by script. Please do not modify!
  // See `docs/webapps/why-is-this-test-failing.md` or
  // `docs/webapps/integration-testing-framework` for more info.
  // Gardeners: Disabling this test is supported.
  helper_.InstallMenuOption(Site::kStandalone);
  helper_.CheckAppTitle(Site::kStandalone, Title::kStandaloneOriginal);
  helper_.CheckWindowCreated();
  helper_.CheckAppInListWindowed(Site::kStandalone);
  helper_.CheckPlatformShortcutAndIcon(Site::kStandalone);
  helper_.CheckWindowControlsOverlayToggle(Site::kStandalone,
                                           IsShown::kNotShown);
  helper_.SwitchProfileClients(ProfileClient::kClient2);
  helper_.CheckAppInListNotLocallyInstalled(Site::kStandalone);
  helper_.NavigateBrowser(Site::kStandalone);
  helper_.CheckInstallIconShown();
  helper_.CheckLaunchIconNotShown();
}

IN_PROC_BROWSER_TEST_F(
    WebAppIntegration,
    WAI_InstallMenuStandalone_SwitchProfileClientsClient2_SyncTurnOff_SyncTurnOn) {
  // Test contents are generated by script. Please do not modify!
  // See `docs/webapps/why-is-this-test-failing.md` or
  // `docs/webapps/integration-testing-framework` for more info.
  // Gardeners: Disabling this test is supported.
  helper_.InstallMenuOption(Site::kStandalone);
  helper_.CheckAppTitle(Site::kStandalone, Title::kStandaloneOriginal);
  helper_.CheckWindowCreated();
  helper_.CheckAppInListWindowed(Site::kStandalone);
  helper_.CheckPlatformShortcutAndIcon(Site::kStandalone);
  helper_.CheckWindowControlsOverlayToggle(Site::kStandalone,
                                           IsShown::kNotShown);
  helper_.SwitchProfileClients(ProfileClient::kClient2);
  helper_.CheckAppInListNotLocallyInstalled(Site::kStandalone);
  helper_.SyncTurnOff();
  helper_.CheckAppNotInList(Site::kStandalone);
  helper_.SyncTurnOn();
  helper_.CheckAppInListNotLocallyInstalled(Site::kStandalone);
}

IN_PROC_BROWSER_TEST_F(
    WebAppIntegration,
    WAI_InstallMenuStandalone_SwitchProfileClientsClient2_SyncTurnOff_SwitchProfileClientsClient1_UninstallFromListStandalone_SyncTurnOn_SwitchProfileClientsClient2) {
  // Test contents are generated by script. Please do not modify!
  // See `docs/webapps/why-is-this-test-failing.md` or
  // `docs/webapps/integration-testing-framework` for more info.
  // Gardeners: Disabling this test is supported.
  helper_.InstallMenuOption(Site::kStandalone);
  helper_.CheckAppTitle(Site::kStandalone, Title::kStandaloneOriginal);
  helper_.CheckWindowCreated();
  helper_.CheckAppInListWindowed(Site::kStandalone);
  helper_.CheckPlatformShortcutAndIcon(Site::kStandalone);
  helper_.CheckWindowControlsOverlayToggle(Site::kStandalone,
                                           IsShown::kNotShown);
  helper_.SwitchProfileClients(ProfileClient::kClient2);
  helper_.CheckAppInListNotLocallyInstalled(Site::kStandalone);
  helper_.SyncTurnOff();
  helper_.CheckAppNotInList(Site::kStandalone);
  helper_.SwitchProfileClients(ProfileClient::kClient1);
  helper_.UninstallFromList(Site::kStandalone);
  helper_.SyncTurnOn();
  helper_.SwitchProfileClients(ProfileClient::kClient2);
  helper_.CheckAppInListNotLocallyInstalled(Site::kStandalone);
}

IN_PROC_BROWSER_TEST_F(
    WebAppIntegration,
    WAI_InstallMenuStandalone_SwitchProfileClientsClient2_SyncTurnOff_SwitchProfileClientsClient1_UninstallFromMenuStandalone_SyncTurnOn_SwitchProfileClientsClient2) {
  // Test contents are generated by script. Please do not modify!
  // See `docs/webapps/why-is-this-test-failing.md` or
  // `docs/webapps/integration-testing-framework` for more info.
  // Gardeners: Disabling this test is supported.
  helper_.InstallMenuOption(Site::kStandalone);
  helper_.CheckAppTitle(Site::kStandalone, Title::kStandaloneOriginal);
  helper_.CheckWindowCreated();
  helper_.CheckAppInListWindowed(Site::kStandalone);
  helper_.CheckPlatformShortcutAndIcon(Site::kStandalone);
  helper_.CheckWindowControlsOverlayToggle(Site::kStandalone,
                                           IsShown::kNotShown);
  helper_.SwitchProfileClients(ProfileClient::kClient2);
  helper_.CheckAppInListNotLocallyInstalled(Site::kStandalone);
  helper_.SyncTurnOff();
  helper_.CheckAppNotInList(Site::kStandalone);
  helper_.SwitchProfileClients(ProfileClient::kClient1);
  helper_.UninstallFromMenu(Site::kStandalone);
  helper_.SyncTurnOn();
  helper_.SwitchProfileClients(ProfileClient::kClient2);
  helper_.CheckAppInListNotLocallyInstalled(Site::kStandalone);
}

IN_PROC_BROWSER_TEST_F(
    WebAppIntegration,
    WAI_InstallMenuStandalone_SwitchProfileClientsClient2_SyncTurnOff_SwitchProfileClientsClient1_UninstallFromAppSettingsStandalone_SyncTurnOn_SwitchProfileClientsClient2) {
  // Test contents are generated by script. Please do not modify!
  // See `docs/webapps/why-is-this-test-failing.md` or
  // `docs/webapps/integration-testing-framework` for more info.
  // Gardeners: Disabling this test is supported.
  helper_.InstallMenuOption(Site::kStandalone);
  helper_.CheckAppTitle(Site::kStandalone, Title::kStandaloneOriginal);
  helper_.CheckWindowCreated();
  helper_.CheckAppInListWindowed(Site::kStandalone);
  helper_.CheckPlatformShortcutAndIcon(Site::kStandalone);
  helper_.CheckWindowControlsOverlayToggle(Site::kStandalone,
                                           IsShown::kNotShown);
  helper_.SwitchProfileClients(ProfileClient::kClient2);
  helper_.CheckAppInListNotLocallyInstalled(Site::kStandalone);
  helper_.SyncTurnOff();
  helper_.CheckAppNotInList(Site::kStandalone);
  helper_.SwitchProfileClients(ProfileClient::kClient1);
  helper_.UninstallFromAppSettings(Site::kStandalone);
  helper_.SyncTurnOn();
  helper_.SwitchProfileClients(ProfileClient::kClient2);
  helper_.CheckAppInListNotLocallyInstalled(Site::kStandalone);
}

IN_PROC_BROWSER_TEST_F(
    WebAppIntegration,
    WAI_InstallMenuNotPromotable_SwitchProfileClientsClient2_InstallLocallyNotPromotable_NavigateBrowserNotPromotable) {
  // Test contents are generated by script. Please do not modify!
  // See `docs/webapps/why-is-this-test-failing.md` or
  // `docs/webapps/integration-testing-framework` for more info.
  // Gardeners: Disabling this test is supported.
  helper_.InstallMenuOption(Site::kNotPromotable);
  helper_.CheckAppInListWindowed(Site::kNotPromotable);
  helper_.CheckPlatformShortcutAndIcon(Site::kNotPromotable);
  helper_.SwitchProfileClients(ProfileClient::kClient2);
  helper_.CheckAppInListNotLocallyInstalled(Site::kNotPromotable);
  helper_.InstallLocally(Site::kNotPromotable);
  helper_.CheckAppInListWindowed(Site::kNotPromotable);
  helper_.CheckPlatformShortcutAndIcon(Site::kNotPromotable);
  helper_.NavigateBrowser(Site::kNotPromotable);
  helper_.CheckInstallIconNotShown();
  helper_.CheckLaunchIconShown();
}

IN_PROC_BROWSER_TEST_F(
    WebAppIntegration,
    WAI_SyncSignOut_InstallMenuStandalone_SyncSignIn_SwitchProfileClientsClient2) {
  // Test contents are generated by script. Please do not modify!
  // See `docs/webapps/why-is-this-test-failing.md` or
  // `docs/webapps/integration-testing-framework` for more info.
  // Gardeners: Disabling this test is supported.
  helper_.SyncSignOut();
  helper_.InstallMenuOption(Site::kStandalone);
  helper_.SyncSignIn();
  helper_.SwitchProfileClients(ProfileClient::kClient2);
  helper_.CheckAppNotInList(Site::kStandalone);
}

IN_PROC_BROWSER_TEST_F(
    WebAppIntegration,
    WAI_InstallOmniboxIconStandalone_SwitchProfileClientsClient2_LaunchFromChromeAppsStandalone) {
  // Test contents are generated by script. Please do not modify!
  // See `docs/webapps/why-is-this-test-failing.md` or
  // `docs/webapps/integration-testing-framework` for more info.
  // Gardeners: Disabling this test is supported.
  helper_.InstallOmniboxIcon(InstallableSite::kStandalone);
  helper_.CheckAppTitle(Site::kStandalone, Title::kStandaloneOriginal);
  helper_.CheckWindowCreated();
  helper_.CheckAppInListWindowed(Site::kStandalone);
  helper_.CheckPlatformShortcutAndIcon(Site::kStandalone);
  helper_.CheckWindowControlsOverlayToggle(Site::kStandalone,
                                           IsShown::kNotShown);
  helper_.SwitchProfileClients(ProfileClient::kClient2);
  helper_.CheckAppInListNotLocallyInstalled(Site::kStandalone);
  helper_.LaunchFromChromeApps(Site::kStandalone);
  helper_.CheckTabNotCreated();
  helper_.CheckSiteLoadedInTab(Site::kStandalone);
}

IN_PROC_BROWSER_TEST_F(
    WebAppIntegration,
    WAI_InstallMenuStandalone_SwitchProfileClientsClient2_LaunchFromChromeAppsStandalone) {
  // Test contents are generated by script. Please do not modify!
  // See `docs/webapps/why-is-this-test-failing.md` or
  // `docs/webapps/integration-testing-framework` for more info.
  // Gardeners: Disabling this test is supported.
  helper_.InstallMenuOption(Site::kStandalone);
  helper_.CheckAppTitle(Site::kStandalone, Title::kStandaloneOriginal);
  helper_.CheckWindowCreated();
  helper_.CheckAppInListWindowed(Site::kStandalone);
  helper_.CheckPlatformShortcutAndIcon(Site::kStandalone);
  helper_.CheckWindowControlsOverlayToggle(Site::kStandalone,
                                           IsShown::kNotShown);
  helper_.SwitchProfileClients(ProfileClient::kClient2);
  helper_.CheckAppInListNotLocallyInstalled(Site::kStandalone);
  helper_.LaunchFromChromeApps(Site::kStandalone);
  helper_.CheckTabNotCreated();
  helper_.CheckSiteLoadedInTab(Site::kStandalone);
}

}  // namespace
}  // namespace web_app::integration_tests
