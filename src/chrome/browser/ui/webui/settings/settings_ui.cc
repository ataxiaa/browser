// Copyright 2015 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/webui/settings/settings_ui.h"

#include <stddef.h>

#include <memory>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "base/feature_list.h"
#include "base/memory/ptr_util.h"
#include "build/branding_buildflags.h"
#include "build/build_config.h"
#include "build/chromeos_buildflags.h"
#include "chrome/browser/browser_features.h"
#include "chrome/browser/commerce/shopping_service_factory.h"
#include "chrome/browser/compose/compose_enabling.h"
#include "chrome/browser/download/bubble/download_bubble_prefs.h"
#include "chrome/browser/history_embeddings/history_embeddings_utils.h"
#include "chrome/browser/optimization_guide/optimization_guide_keyed_service.h"
#include "chrome/browser/optimization_guide/optimization_guide_keyed_service_factory.h"
#include "chrome/browser/performance_manager/public/user_tuning/user_performance_tuning_manager.h"
#include "chrome/browser/performance_manager/public/user_tuning/user_tuning_utils.h"
#include "chrome/browser/preloading/preloading_features.h"
#include "chrome/browser/privacy_sandbox/privacy_sandbox_service.h"
#include "chrome/browser/privacy_sandbox/privacy_sandbox_service_factory.h"
#include "chrome/browser/privacy_sandbox/tracking_protection_onboarding_factory.h"
#include "chrome/browser/privacy_sandbox/tracking_protection_settings_factory.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/search_engine_choice/search_engine_choice_service_factory.h"
#include "chrome/browser/signin/identity_manager_factory.h"
#include "chrome/browser/ssl/https_upgrades_util.h"
#include "chrome/browser/ui/browser_element_identifiers.h"
#include "chrome/browser/ui/hats/hats_service.h"
#include "chrome/browser/ui/hats/hats_service_factory.h"
#include "chrome/browser/ui/managed_ui.h"
#include "chrome/browser/ui/passwords/ui_utils.h"
#include "chrome/browser/ui/tabs/organization/tab_organization_utils.h"
#include "chrome/browser/ui/ui_features.h"
#include "chrome/browser/ui/views/side_panel/customize_chrome/customize_chrome_utils.h"
#include "chrome/browser/ui/webui/cr_components/customize_color_scheme_mode/customize_color_scheme_mode_handler.h"
#include "chrome/browser/ui/webui/extension_control_handler.h"
#include "chrome/browser/ui/webui/favicon_source.h"
#include "chrome/browser/ui/webui/managed_ui_handler.h"
#include "chrome/browser/ui/webui/metrics_handler.h"
#include "chrome/browser/ui/webui/plural_string_handler.h"
#include "chrome/browser/ui/webui/search_engine_choice/icon_utils.h"
#include "chrome/browser/ui/webui/settings/about_handler.h"
#include "chrome/browser/ui/webui/settings/accessibility_main_handler.h"
#include "chrome/browser/ui/webui/settings/appearance_handler.h"
#include "chrome/browser/ui/webui/settings/browser_lifetime_handler.h"
#include "chrome/browser/ui/webui/settings/downloads_handler.h"
#include "chrome/browser/ui/webui/settings/font_handler.h"
#include "chrome/browser/ui/webui/settings/hats_handler.h"
#include "chrome/browser/ui/webui/settings/import_data_handler.h"
#include "chrome/browser/ui/webui/settings/metrics_reporting_handler.h"
#include "chrome/browser/ui/webui/settings/on_startup_handler.h"
#include "chrome/browser/ui/webui/settings/password_manager_handler.h"
#include "chrome/browser/ui/webui/settings/people_handler.h"
#include "chrome/browser/ui/webui/settings/performance_handler.h"
#include "chrome/browser/ui/webui/settings/privacy_sandbox_handler.h"
#include "chrome/browser/ui/webui/settings/profile_info_handler.h"
#include "chrome/browser/ui/webui/settings/protocol_handlers_handler.h"
#include "chrome/browser/ui/webui/settings/reset_settings_handler.h"
#include "chrome/browser/ui/webui/settings/safety_hub_handler.h"
#include "chrome/browser/ui/webui/settings/search_engines_handler.h"
#include "chrome/browser/ui/webui/settings/settings_clear_browsing_data_handler.h"
#include "chrome/browser/ui/webui/settings/settings_localized_strings_provider.h"
#include "chrome/browser/ui/webui/settings/settings_media_devices_selection_handler.h"
#include "chrome/browser/ui/webui/settings/settings_page_ui_handler.h"
#include "chrome/browser/ui/webui/settings/settings_secure_dns_handler.h"
#include "chrome/browser/ui/webui/settings/settings_security_key_handler.h"
#include "chrome/browser/ui/webui/settings/settings_startup_pages_handler.h"
#include "chrome/browser/ui/webui/settings/shared_settings_localized_strings_provider.h"
#include "chrome/browser/ui/webui/settings/site_settings_handler.h"
#include "chrome/common/chrome_features.h"
#include "chrome/common/pref_names.h"
#include "chrome/common/url_constants.h"
#include "chrome/grit/branded_strings.h"
#include "chrome/grit/generated_resources.h"
#include "chrome/grit/settings_resources.h"
#include "chrome/grit/settings_resources_map.h"
#include "components/account_manager_core/account_manager_facade.h"
#include "components/commerce/core/commerce_feature_list.h"
#include "components/commerce/core/feature_utils.h"
#include "components/commerce/core/shopping_service.h"
#include "components/compose/core/browser/compose_features.h"
#include "components/content_settings/core/common/features.h"
#include "components/favicon_base/favicon_url_parser.h"
#include "components/optimization_guide/core/optimization_guide_features.h"
#include "components/password_manager/core/common/password_manager_features.h"
#include "components/performance_manager/public/features.h"
#include "components/permissions/features.h"
#include "components/pref_registry/pref_registry_syncable.h"
#include "components/prefs/pref_service.h"
#include "components/privacy_sandbox/privacy_sandbox_features.h"
#include "components/privacy_sandbox/tracking_protection_settings.h"
#include "components/safe_browsing/core/common/features.h"
#include "components/safe_browsing/core/common/hashprefix_realtime/hash_realtime_utils.h"
#include "components/search_engines/search_engine_choice/search_engine_choice_service.h"
#include "components/search_engines/search_engine_choice/search_engine_choice_utils.h"
#include "components/signin/public/base/signin_pref_names.h"
#include "components/signin/public/base/signin_switches.h"
#include "components/sync/base/features.h"
#include "content/public/browser/url_data_source.h"
#include "content/public/browser/web_contents.h"
#include "content/public/browser/web_ui.h"
#include "content/public/browser/web_ui_data_source.h"
#include "content/public/common/content_features.h"
#include "crypto/crypto_buildflags.h"
#include "device/vr/buildflags/buildflags.h"
#include "printing/buildflags/buildflags.h"
#include "services/network/public/cpp/features.h"
#include "third_party/blink/public/common/features.h"
#include "ui/base/interaction/element_identifier.h"
#include "ui/webui/webui_util.h"

#if !BUILDFLAG(OPTIMIZE_WEBUI)
#include "chrome/grit/settings_shared_resources.h"
#include "chrome/grit/settings_shared_resources_map.h"
#endif

#if BUILDFLAG(IS_WIN) && BUILDFLAG(GOOGLE_CHROME_BRANDING)
#include "chrome/browser/ui/webui/settings/incompatible_applications_handler_win.h"
#include "chrome/browser/win/conflicts/incompatible_applications_updater.h"
#include "chrome/browser/win/conflicts/token_util.h"
#endif  // BUILDFLAG(IS_WIN) && BUILDFLAG(GOOGLE_CHROME_BRANDING)

#if BUILDFLAG(IS_WIN) || BUILDFLAG(IS_CHROMEOS_ASH)
#include "chrome/browser/ui/webui/settings/languages_handler.h"
#endif  // BUILDFLAG(IS_WIN) || BUILDFLAG(IS_CHROMEOS_ASH)

#if !BUILDFLAG(IS_CHROMEOS_ASH)
#include "components/language/core/common/language_experiments.h"
#endif  // !BUILDFLAG(IS_CHROMEOS_ASH)

#if BUILDFLAG(IS_CHROMEOS_ASH)
#include "ash/components/arc/arc_util.h"
#include "ash/webui/eche_app_ui/eche_app_manager.h"
#include "chrome/browser/ash/account_manager/account_apps_availability.h"
#include "chrome/browser/ash/account_manager/account_apps_availability_factory.h"
#include "chrome/browser/ash/account_manager/account_manager_util.h"
#include "chrome/browser/ash/eche_app/eche_app_manager_factory.h"
#include "chrome/browser/ash/multidevice_setup/multidevice_setup_client_factory.h"
#include "chrome/browser/ash/phonehub/phone_hub_manager_factory.h"
#include "chrome/browser/ash/profiles/profile_helper.h"
#include "chrome/browser/browser_process.h"
#include "chrome/browser/browser_process_platform_part.h"
#include "chrome/browser/ui/webui/ash/settings/pages/multidevice/multidevice_handler.h"
#include "chrome/browser/ui/webui/ash/settings/pages/people/account_manager_ui_handler.h"
#include "chrome/common/chrome_switches.h"
#include "chrome/grit/browser_resources.h"
#include "chromeos/ash/components/account_manager/account_manager_factory.h"
#include "chromeos/ash/components/login/auth/password_visibility_utils.h"
#include "chromeos/ash/components/phonehub/phone_hub_manager.h"
#include "components/account_manager_core/chromeos/account_manager.h"
#include "components/account_manager_core/chromeos/account_manager_facade_factory.h"
#include "components/user_manager/user.h"
#include "ui/base/ui_base_features.h"
#else  // !BUILDFLAG(IS_CHROMEOS_ASH)
#include "chrome/browser/search/background/ntp_custom_background_service_factory.h"
#include "chrome/browser/signin/account_consistency_mode_manager.h"
#include "chrome/browser/ui/webui/cr_components/theme_color_picker/theme_color_picker_handler.h"
#include "chrome/browser/ui/webui/settings/captions_handler.h"
#include "chrome/browser/ui/webui/settings/settings_default_browser_handler.h"
#include "chrome/browser/ui/webui/settings/settings_manage_profile_handler.h"
#include "chrome/browser/ui/webui/settings/system_handler.h"
#endif  // BUILDFLAG(IS_CHROMEOS_ASH)

#if BUILDFLAG(IS_CHROMEOS)
#include "chrome/browser/ui/webui/certificate_provisioning_ui_handler.h"
#include "chromeos/constants/chromeos_features.h"
#endif  // BUILDFLAG(IS_CHROMEOS)

#if BUILDFLAG(USE_NSS_CERTS)
#include "chrome/browser/ui/webui/certificates_handler.h"
#elif BUILDFLAG(IS_WIN) || BUILDFLAG(IS_MAC)
#include "chrome/browser/ui/webui/settings/native_certificates_handler.h"
#endif  // BUILDFLAG(USE_NSS_CERTS)

#if BUILDFLAG(IS_MAC)
#include "chrome/browser/ui/webui/settings/mac_system_settings_handler.h"
#endif

#if BUILDFLAG(ENABLE_VR)
#include "device/vr/public/cpp/features.h"
#endif

namespace settings {

using optimization_guide::UserVisibleFeatureKey;

// static
void SettingsUI::RegisterProfilePrefs(
    user_prefs::PrefRegistrySyncable* registry) {
  registry->RegisterBooleanPref(prefs::kImportDialogAutofillFormData, true);
  registry->RegisterBooleanPref(prefs::kImportDialogBookmarks, true);
  registry->RegisterBooleanPref(prefs::kImportDialogHistory, true);
  registry->RegisterBooleanPref(prefs::kImportDialogSavedPasswords, true);
  registry->RegisterBooleanPref(prefs::kImportDialogSearchEngine, true);
}

SettingsUI::SettingsUI(content::WebUI* web_ui)
    : ui::MojoWebUIController(web_ui, /*enable_chrome_send=*/true) {
  Profile* profile = Profile::FromWebUI(web_ui);
  content::WebUIDataSource* html_source =
      content::WebUIDataSource::CreateAndAdd(
          web_ui->GetWebContents()->GetBrowserContext(),
          chrome::kChromeUISettingsHost);
  html_source->OverrideContentSecurityPolicy(
      network::mojom::CSPDirectiveName::WorkerSrc,
      "worker-src blob: chrome://resources 'self';");

  AddSettingsPageUIHandler(std::make_unique<AppearanceHandler>(web_ui));

#if BUILDFLAG(USE_NSS_CERTS)
  AddSettingsPageUIHandler(
      std::make_unique<certificate_manager::CertificatesHandler>());
#elif BUILDFLAG(IS_WIN) || BUILDFLAG(IS_MAC)
  AddSettingsPageUIHandler(std::make_unique<NativeCertificatesHandler>());
#endif  // BUILDFLAG(USE_NSS_CERTS)

#if BUILDFLAG(CHROME_ROOT_STORE_CERT_MANAGEMENT_UI)
  // Chrome Certificate Management UI V2.
  html_source->AddBoolean(
      "enableCertManagementUIV2",
      base::FeatureList::IsEnabled(features::kEnableCertManagementUIV2));
#endif  // BUILDFLAG(CHROME_ROOT_STORE_CERT_MANAGEMENT_UI)

#if BUILDFLAG(IS_CHROMEOS)
  AddSettingsPageUIHandler(
      chromeos::cert_provisioning::CertificateProvisioningUiHandler::
          CreateForProfile(profile));
#endif  // BUILDFLAG(IS_CHROMEOS)

  AddSettingsPageUIHandler(std::make_unique<AccessibilityMainHandler>());
  AddSettingsPageUIHandler(std::make_unique<BrowserLifetimeHandler>());
  AddSettingsPageUIHandler(
      std::make_unique<ClearBrowsingDataHandler>(web_ui, profile));
  AddSettingsPageUIHandler(std::make_unique<SafetyHubHandler>(profile));
  AddSettingsPageUIHandler(std::make_unique<DownloadsHandler>(profile));
  AddSettingsPageUIHandler(std::make_unique<ExtensionControlHandler>());
  AddSettingsPageUIHandler(std::make_unique<FontHandler>(profile));
  AddSettingsPageUIHandler(std::make_unique<ImportDataHandler>());
  AddSettingsPageUIHandler(std::make_unique<HatsHandler>());

#if BUILDFLAG(IS_WIN)
  AddSettingsPageUIHandler(std::make_unique<LanguagesHandler>());
#endif  // BUILDFLAG(IS_WIN)
#if BUILDFLAG(IS_CHROMEOS_ASH)
  AddSettingsPageUIHandler(std::make_unique<LanguagesHandler>(profile));
#endif  // BUILDFLAG(IS_CHROMEOS_ASH)

  AddSettingsPageUIHandler(
      std::make_unique<MediaDevicesSelectionHandler>(profile));
#if BUILDFLAG(GOOGLE_CHROME_BRANDING) && !BUILDFLAG(IS_CHROMEOS_ASH)
  AddSettingsPageUIHandler(std::make_unique<MetricsReportingHandler>());
#endif
  AddSettingsPageUIHandler(std::make_unique<OnStartupHandler>(profile));
  AddSettingsPageUIHandler(std::make_unique<PeopleHandler>(profile));
  AddSettingsPageUIHandler(std::make_unique<ProfileInfoHandler>(profile));
  AddSettingsPageUIHandler(std::make_unique<ProtocolHandlersHandler>(profile));
  AddSettingsPageUIHandler(std::make_unique<PrivacySandboxHandler>());
  AddSettingsPageUIHandler(std::make_unique<SearchEnginesHandler>(profile));
  AddSettingsPageUIHandler(std::make_unique<SecureDnsHandler>());
  AddSettingsPageUIHandler(std::make_unique<SiteSettingsHandler>(profile));
  AddSettingsPageUIHandler(std::make_unique<StartupPagesHandler>(web_ui));
  AddSettingsPageUIHandler(std::make_unique<SecurityKeysPINHandler>());
  AddSettingsPageUIHandler(std::make_unique<SecurityKeysResetHandler>());
  AddSettingsPageUIHandler(std::make_unique<SecurityKeysCredentialHandler>());
  AddSettingsPageUIHandler(
      std::make_unique<SecurityKeysBioEnrollmentHandler>());
  AddSettingsPageUIHandler(std::make_unique<SecurityKeysPhonesHandler>());
  AddSettingsPageUIHandler(std::make_unique<PasswordManagerHandler>());
#if BUILDFLAG(IS_WIN) || BUILDFLAG(IS_MAC)
  AddSettingsPageUIHandler(std::make_unique<PasskeysHandler>());
#endif

#if BUILDFLAG(IS_CHROMEOS_ASH)
  InitBrowserSettingsWebUIHandlers();
#else
  AddSettingsPageUIHandler(
      std::make_unique<CaptionsHandler>(profile->GetPrefs()));
  AddSettingsPageUIHandler(std::make_unique<DefaultBrowserHandler>());
  AddSettingsPageUIHandler(std::make_unique<ManageProfileHandler>(profile));
  AddSettingsPageUIHandler(std::make_unique<SystemHandler>());

#endif

#if BUILDFLAG(IS_MAC)
  AddSettingsPageUIHandler(std::make_unique<MacSystemSettingsHandler>());
#endif

#if BUILDFLAG(IS_WIN) && BUILDFLAG(GOOGLE_CHROME_BRANDING)
  bool has_incompatible_applications =
      IncompatibleApplicationsUpdater::HasCachedApplications();
  html_source->AddBoolean("showIncompatibleApplications",
                          has_incompatible_applications);
  html_source->AddBoolean("hasAdminRights", HasAdminRights());

  if (has_incompatible_applications) {
    AddSettingsPageUIHandler(
        std::make_unique<IncompatibleApplicationsHandler>());
  }
#endif  // BUILDFLAG(IS_WIN) && BUILDFLAG(GOOGLE_CHROME_BRANDING)

  html_source->AddBoolean("signinAllowed", !profile->IsGuestSession() &&
                                               profile->GetPrefs()->GetBoolean(
                                                   prefs::kSigninAllowed));

  html_source->AddBoolean(
      "turnOffSyncAllowedForManagedProfiles",
      base::FeatureList::IsEnabled(kDisallowManagedProfileSignout));

  commerce::ShoppingService* shopping_service =
      commerce::ShoppingServiceFactory::GetForBrowserContext(profile);
  html_source->AddBoolean("changePriceEmailNotificationsEnabled",
                          shopping_service->IsShoppingListEligible());
  if (shopping_service->IsShoppingListEligible()) {
    commerce::ShoppingServiceFactory::GetForBrowserContext(profile)
        ->FetchPriceEmailPref();
  }

  search_engines::SearchEngineChoiceService*
      search_engine_choice_dialog_service =
          search_engines::SearchEngineChoiceServiceFactory::GetForProfile(
              profile);
  const bool is_eea_choice_country = search_engines::IsEeaChoiceCountry(
      search_engine_choice_dialog_service->GetCountryId());
  html_source->AddBoolean("isEeaChoiceCountry", is_eea_choice_country);

#if BUILDFLAG(IS_CHROMEOS_ASH)
  html_source->AddBoolean(
      "userCannotManuallyEnterPassword",
      !ash::password_visibility::AccountHasUserFacingPassword(
          g_browser_process->local_state(), ash::ProfileHelper::Get()
                                                ->GetUserByProfile(profile)
                                                ->GetAccountId()));

  // This is the browser settings page.
  html_source->AddBoolean("isOSSettings", false);
#endif  // BUILDFLAG(IS_CHROMEOS_ASH)

  bool show_privacy_guide =
      base::FeatureList::IsEnabled(features::kPrivacyGuideForceAvailable) ||
      (!chrome::ShouldDisplayManagedUi(profile) && !profile->IsChild());
  html_source->AddBoolean("showPrivacyGuide", show_privacy_guide);

  html_source->AddBoolean("enableHandTrackingContentSetting",
#if BUILDFLAG(ENABLE_VR)
                          device::features::IsHandTrackingEnabled());
#else
                          false);
#endif

  html_source->AddBoolean(
      "enableEsbAiStringUpdate",
      base::FeatureList::IsEnabled(safe_browsing::kEsbAiStringUpdate));

  html_source->AddBoolean("enableHashPrefixRealTimeLookups",
                          safe_browsing::hash_realtime_utils::
                              IsHashRealTimeLookupEligibleInSession());

  html_source->AddBoolean("enableHttpsFirstModeNewSettings",
                          IsBalancedModeAvailable());

  html_source->AddBoolean(
      "enableKeyboardAndPointerLockPrompt",
      base::FeatureList::IsEnabled(
          permissions::features::kKeyboardAndPointerLockPrompt));

  html_source->AddBoolean(
      "enableLinkedServicesSetting",
      base::FeatureList::IsEnabled(features::kLinkedServicesSetting));

#if BUILDFLAG(ENABLE_COMPOSE)
  const bool compose_enabled = ComposeEnabling::IsEnabledForProfile(profile);
  const bool compose_visible = ComposeEnabling::IsSettingVisible(profile);
#else
  const bool compose_enabled = false;
  const bool compose_visible = false;
#endif  // BUILDFLAG(ENABLE_COMPOSE)
  html_source->AddBoolean(
      "enableComposeProactiveNudge",
      compose_enabled && base::FeatureList::IsEnabled(
                             compose::features::kEnableComposeProactiveNudge));

  html_source->AddBoolean(
      "downloadBubblePartialViewControlledByPref",
      download::IsDownloadBubbleEnabled() &&
          download::IsDownloadBubblePartialViewControlledByPref());

  html_source->AddBoolean(
      "extendedReportingRemovePrefDependency",
      base::FeatureList::IsEnabled(
          safe_browsing::kExtendedReportingRemovePrefDependency));

  html_source->AddBoolean(
      "hashPrefixRealTimeLookupsSamplePing",
      base::FeatureList::IsEnabled(
          safe_browsing::kHashPrefixRealTimeLookupsSamplePing));

  html_source->AddBoolean(
      "enablePasswordLeakToggleMove",
      base::FeatureList::IsEnabled(safe_browsing::kPasswordLeakToggleMove));

  AddSettingsPageUIHandler(std::make_unique<AboutHandler>(profile));
  AddSettingsPageUIHandler(std::make_unique<ResetSettingsHandler>(profile));

  // Add a handler to provide pluralized strings.
  auto plural_string_handler = std::make_unique<PluralStringHandler>();
  plural_string_handler->AddLocalizedString("securityKeysNewPIN",
                                            IDS_SETTINGS_SECURITY_KEYS_NEW_PIN);
  plural_string_handler->AddLocalizedString(
      "safetyCheckExtensionsReviewLabel",
      IDS_SETTINGS_SAFETY_CHECK_REVIEW_EXTENSIONS);
  plural_string_handler->AddLocalizedString(
      "safetyCheckNotificationPermissionReviewHeaderLabel",
      IDS_SETTINGS_SAFETY_CHECK_REVIEW_NOTIFICATION_PERMISSIONS_HEADER_LABEL);
  plural_string_handler->AddLocalizedString(
      "safetyCheckNotificationPermissionReviewBlockAllToastLabel",
      IDS_SETTINGS_SAFETY_CHECK_NOTIFICATION_PERMISSION_REVIEW_BLOCK_ALL_TOAST_LABEL);
  plural_string_handler->AddLocalizedString(
      "safetyCheckNotificationPermissionReviewPrimaryLabel",
      IDS_SETTINGS_SAFETY_CHECK_REVIEW_NOTIFICATION_PERMISSIONS_PRIMARY_LABEL);
  plural_string_handler->AddLocalizedString(
      "safetyCheckUnusedSitePermissionsHeaderLabel",
      IDS_SETTINGS_SAFETY_CHECK_UNUSED_SITE_PERMISSIONS_HEADER_LABEL);
  plural_string_handler->AddLocalizedString(
      "safetyCheckNotificationPermissionReviewSecondaryLabel",
      IDS_SETTINGS_SAFETY_CHECK_REVIEW_NOTIFICATION_PERMISSIONS_SECONDARY_LABEL);
  plural_string_handler->AddLocalizedString(
      "safetyCheckUnusedSitePermissionsPrimaryLabel",
      IDS_SETTINGS_SAFETY_CHECK_UNUSED_SITE_PERMISSIONS_PRIMARY_LABEL);
  plural_string_handler->AddLocalizedString(
      "safetyCheckUnusedSitePermissionsSecondaryLabel",
      IDS_SETTINGS_SAFETY_CHECK_UNUSED_SITE_PERMISSIONS_SECONDARY_LABEL);
  plural_string_handler->AddLocalizedString(
      "safetyHubRevokedPermissionsSecondaryLabel",
      IDS_SETTINGS_SAFETY_HUB_REVOKED_PERMISSIONS_SECONDARY_LABEL);
  plural_string_handler->AddLocalizedString(
      "safetyCheckUnusedSitePermissionsToastBulkLabel",
      IDS_SETTINGS_SAFETY_CHECK_UNUSED_SITE_PERMISSIONS_TOAST_BULK_LABEL);
  plural_string_handler->AddLocalizedString(
      "safetyHubNotificationPermissionsPrimaryLabel",
      IDS_SETTINGS_SAFETY_HUB_NOTIFICATION_PERMISSIONS_PRIMARY_LABEL);
  plural_string_handler->AddLocalizedString(
      "safetyHubNotificationPermissionsSecondaryLabel",
      IDS_SETTINGS_SAFETY_HUB_NOTIFICATION_PERMISSIONS_SECONDARY_LABEL);
  web_ui->AddMessageHandler(std::move(plural_string_handler));

  // Add the metrics handler to write uma stats.
  web_ui->AddMessageHandler(std::make_unique<MetricsHandler>());

  webui::SetupWebUIDataSource(html_source, kSettingsResources,
                              IDR_SETTINGS_SETTINGS_HTML);
  // Add chrome://webui-test for cr-lottie test.
  html_source->OverrideContentSecurityPolicy(
      network::mojom::CSPDirectiveName::ConnectSrc,
      "connect-src chrome://webui-test chrome://resources chrome://theme "
      "'self';");
  // Add TrustedTypes policy for cr-lottie.
  html_source->OverrideContentSecurityPolicy(
      network::mojom::CSPDirectiveName::TrustedTypes,
      base::StrCat({webui::kDefaultTrustedTypesPolicies,
                    " lottie-worker-script-loader;"}));

#if !BUILDFLAG(OPTIMIZE_WEBUI)
  html_source->AddResourcePaths(kSettingsSharedResources);
#endif

  AddLocalizedStrings(html_source, profile, web_ui->GetWebContents());

  ManagedUIHandler::Initialize(web_ui, html_source);

  content::URLDataSource::Add(
      profile, std::make_unique<FaviconSource>(
                   profile, chrome::FaviconUrlFormat::kFavicon2));

  // Privacy Sandbox
  PrivacySandboxService* privacy_sandbox_service =
      PrivacySandboxServiceFactory::GetForProfile(profile);
  bool is_privacy_sandbox_restricted =
      privacy_sandbox_service->IsPrivacySandboxRestricted();
  bool is_restricted_notice_enabled =
      privacy_sandbox_service->IsRestrictedNoticeEnabled();
  html_source->AddBoolean("isPrivacySandboxRestricted",
                          is_privacy_sandbox_restricted);
  html_source->AddBoolean("isPrivacySandboxRestrictedNoticeEnabled",
                          is_restricted_notice_enabled);

  html_source->AddBoolean(
      "safetyHubAbusiveNotificationRevocationEnabled",
      base::FeatureList::IsEnabled(
          safe_browsing::kSafetyHubAbusiveNotificationRevocation));

  html_source->AddBoolean("enableSafetyHub",
                          base::FeatureList::IsEnabled(features::kSafetyHub));

  // Mode B UX
  html_source->AddBoolean(
      "is3pcdCookieSettingsRedesignEnabled",
      TrackingProtectionSettingsFactory::GetForProfile(profile)
          ->IsTrackingProtection3pcdEnabled());

  html_source->AddBoolean(
      "isAlwaysBlock3pcsIncognitoEnabled",
      base::FeatureList::IsEnabled(privacy_sandbox::kAlwaysBlock3pcsIncognito));

  // ACT UX
  html_source->AddBoolean(
      "isIpProtectionUxEnabled",
      base::FeatureList::IsEnabled(privacy_sandbox::kIpProtectionUx));
  html_source->AddBoolean("isFingerprintingProtectionUxEnabled",
                          base::FeatureList::IsEnabled(
                              privacy_sandbox::kFingerprintingProtectionUx));

  // Performance
  AddSettingsPageUIHandler(std::make_unique<PerformanceHandler>());
  html_source->AddBoolean(
      "isPerformanceInterventionUiEnabled",
      base::FeatureList::IsEnabled(
          performance_manager::features::kPerformanceInterventionUI));
  html_source->AddBoolean(
      "isBatterySaverModeManagedByOS",
      performance_manager::user_tuning::IsBatterySaverModeManagedByOS());

  html_source->AddBoolean(
      "autoPictureInPictureEnabled",
      base::FeatureList::IsEnabled(
          blink::features::kMediaSessionEnterPictureInPicture));

  html_source->AddBoolean(
      "capturedSurfaceControlEnabled",
      base::FeatureList::IsEnabled(
          features::kCapturedSurfaceControlKillswitch) &&
          base::FeatureList::IsEnabled(
              features::kCapturedSurfaceControlStickyPermissions));

  html_source->AddBoolean("enableAutomaticFullscreenContentSetting",
                          base::FeatureList::IsEnabled(
                              features::kAutomaticFullscreenContentSetting));

#if BUILDFLAG(IS_CHROMEOS)
  html_source->AddBoolean(
      "enableSmartCardReadersContentSetting",
      base::FeatureList::IsEnabled(blink::features::kSmartCard));
#endif

#if BUILDFLAG(IS_WIN) && BUILDFLAG(GOOGLE_CHROME_BRANDING)
  // System
  html_source->AddBoolean(
      "showFeatureNotificationsSetting",
      base::FeatureList::IsEnabled(features::kRegisterOsUpdateHandlerWin));
#endif  // BUILDFLAG(IS_WIN) && BUILDFLAG(GOOGLE_CHROME_BRANDING)

  html_source->AddBoolean(
      "enableWebAppInstallation",
      base::FeatureList::IsEnabled(blink::features::kWebAppInstallation));

  html_source->AddBoolean("showGlicSettings",
                          base::FeatureList::IsEnabled(features::kGlic));

  // AI
  const bool ai_settings_refresh_enabled =
      optimization_guide::features::IsAiSettingsPageRefreshEnabled();

  if (ai_settings_refresh_enabled) {
    const bool show_ai_settings_for_testing =
        optimization_guide::features::kShowAiSettingsForTesting.Get();

    html_source->AddBoolean("showAiSettingsForTesting",
                            show_ai_settings_for_testing);

    const bool use_is_setting_visible = base::FeatureList::IsEnabled(
        optimization_guide::features::kAiSettingsPageEnterpriseDisabledUi);

    std::pair<const std::string_view, bool> optimization_guide_features[] = {
        {"showTabOrganizationControl",
         use_is_setting_visible
             ? TabOrganizationUtils::GetInstance()->IsSettingVisible(profile)
             : TabOrganizationUtils::GetInstance()->IsEnabled(profile)},
        {"showComposeControl",
         use_is_setting_visible ? compose_visible : compose_enabled},
        {"showWallpaperSearchControl",
         use_is_setting_visible
             ? customize_chrome::IsWallpaperSearchSettingVisibleForProfile(
                   profile)
             : customize_chrome::IsWallpaperSearchEnabledForProfile(profile)},
        {"showHistorySearchControl",
         history_embeddings::IsHistoryEmbeddingsSettingVisible(profile)},
        {"showCompareControl",
         use_is_setting_visible
             ? commerce::IsProductSpecificationsSettingVisible(
                   shopping_service->GetAccountChecker())
             : commerce::CanFetchProductSpecificationsData(
                   shopping_service->GetAccountChecker())},
    };

    bool show_ai_page = show_ai_settings_for_testing;
    for (auto [name, visible] : optimization_guide_features) {
      html_source->AddBoolean(name, visible || show_ai_settings_for_testing);
      show_ai_page |= visible;
    }

    // "showAdvancedFeaturesMainControl", despite the name, controls whether the
    // AI subpage is shown. We want to show the page if any of the AI features
    // are enabled.
    // TODO(crbug.com/363968675): Rename this to be clearer.
    html_source->AddBoolean("showAdvancedFeaturesMainControl", show_ai_page);
  } else {
    std::pair<UserVisibleFeatureKey, const std::string_view>
        optimization_guide_features[] = {
            {UserVisibleFeatureKey::kCompose, "showComposeControl"},
            {UserVisibleFeatureKey::kTabOrganization,
             "showTabOrganizationControl"},
            {UserVisibleFeatureKey::kWallpaperSearch,
             "showWallpaperSearchControl"},
            {UserVisibleFeatureKey::kHistorySearch, "showHistorySearchControl"},
        };
    bool is_any_ai_feature_enabled = false;

    auto* optimization_guide_service =
        OptimizationGuideKeyedServiceFactory::GetForProfile(profile);
    for (auto [key, name] : optimization_guide_features) {
      const bool visible = optimization_guide_service &&
                           optimization_guide_service->IsSettingVisible(key);
      html_source->AddBoolean(name, visible);

      // The main toggle is visible only if at least one of the sub toggles is
      // visible.
      is_any_ai_feature_enabled |= visible;
    }

    html_source->AddBoolean("showAdvancedFeaturesMainControl",
                            is_any_ai_feature_enabled);
    // Compare is only shown when Synpase ("AiSettingsPageRefresh") is enabled.
    html_source->AddBoolean("showCompareControl", false);
  }

  html_source->AddBoolean("enableAiSettingsPageRefresh",
                          ai_settings_refresh_enabled);
  html_source->AddBoolean(
      "enableAiSettingsInPrivacyGuide",
      optimization_guide::features::IsPrivacyGuideAiSettingsEnabled());

  TryShowHatsSurveyWithTimeout();
}

DEFINE_CLASS_ELEMENT_IDENTIFIER_VALUE(
    SettingsUI,
    kAutofillPredictionImprovementsHeaderElementId);

SettingsUI::~SettingsUI() = default;

#if BUILDFLAG(IS_CHROMEOS_ASH)
void SettingsUI::InitBrowserSettingsWebUIHandlers() {
  Profile* profile = Profile::FromWebUI(web_ui());

  // TODO(jamescook): Sort out how account management is split between Chrome OS
  // and browser settings.
  if (ash::IsAccountManagerAvailable(profile)) {
    auto* factory =
        g_browser_process->platform_part()->GetAccountManagerFactory();
    auto* account_manager =
        factory->GetAccountManager(profile->GetPath().value());
    DCHECK(account_manager);
    auto* account_manager_facade =
        ::GetAccountManagerFacade(profile->GetPath().value());
    DCHECK(account_manager_facade);

    web_ui()->AddMessageHandler(
        std::make_unique<ash::settings::AccountManagerUIHandler>(
            account_manager, account_manager_facade,
            IdentityManagerFactory::GetForProfile(profile),
            ash::AccountAppsAvailabilityFactory::GetForProfile(profile)));
  }

  if (!profile->IsGuestSession()) {
    ash::phonehub::PhoneHubManager* phone_hub_manager =
        ash::phonehub::PhoneHubManagerFactory::GetForProfile(profile);
    ash::eche_app::EcheAppManager* eche_app_manager =
        ash::eche_app::EcheAppManagerFactory::GetForProfile(profile);

    web_ui()->AddMessageHandler(std::make_unique<
                                ash::settings::MultideviceHandler>(
        profile->GetPrefs(),
        ash::multidevice_setup::MultiDeviceSetupClientFactory::GetForProfile(
            profile),
        phone_hub_manager
            ? phone_hub_manager->GetMultideviceFeatureAccessManager()
            : nullptr,
        eche_app_manager ? eche_app_manager->GetAppsAccessManager() : nullptr,
        phone_hub_manager ? phone_hub_manager->GetCameraRollManager() : nullptr,
        phone_hub_manager ? phone_hub_manager->GetBrowserTabsModelProvider()
                          : nullptr));
  }
}
#else   // BUILDFLAG(IS_CHROMEOS_ASH)
void SettingsUI::BindInterface(
    mojo::PendingReceiver<
        theme_color_picker::mojom::ThemeColorPickerHandlerFactory>
        pending_receiver) {
  if (theme_color_picker_handler_factory_receiver_.is_bound()) {
    theme_color_picker_handler_factory_receiver_.reset();
  }
  theme_color_picker_handler_factory_receiver_.Bind(
      std::move(pending_receiver));
}
#endif  // !BUILDFLAG(IS_CHROMEOS_ASH)

void SettingsUI::BindInterface(
    mojo::PendingReceiver<help_bubble::mojom::HelpBubbleHandlerFactory>
        pending_receiver) {
  if (help_bubble_handler_factory_receiver_.is_bound()) {
    help_bubble_handler_factory_receiver_.reset();
  }
  help_bubble_handler_factory_receiver_.Bind(std::move(pending_receiver));
}

void SettingsUI::AddSettingsPageUIHandler(
    std::unique_ptr<content::WebUIMessageHandler> handler) {
  DCHECK(handler);
  web_ui()->AddMessageHandler(std::move(handler));
}

void SettingsUI::TryShowHatsSurveyWithTimeout() {
  HatsService* hats_service =
      HatsServiceFactory::GetForProfile(Profile::FromWebUI(web_ui()),
                                        /* create_if_necessary = */ true);
  if (hats_service) {
    int timeout_ms =
        features::kHappinessTrackingSurveysForDesktopSettingsTime.Get()
            .InMilliseconds();
    hats_service->LaunchDelayedSurveyForWebContents(
        kHatsSurveyTriggerSettings, web_ui()->GetWebContents(), timeout_ms, {},
        {}, HatsService::NavigationBehaviour::REQUIRE_SAME_ORIGIN);
  }
}

#if !BUILDFLAG(IS_CHROMEOS_ASH)
void SettingsUI::CreateThemeColorPickerHandler(
    mojo::PendingReceiver<theme_color_picker::mojom::ThemeColorPickerHandler>
        handler,
    mojo::PendingRemote<theme_color_picker::mojom::ThemeColorPickerClient>
        client) {
  theme_color_picker_handler_ = std::make_unique<ThemeColorPickerHandler>(
      std::move(handler), std::move(client),
      NtpCustomBackgroundServiceFactory::GetForProfile(
          Profile::FromWebUI(web_ui())),
      web_ui()->GetWebContents());
}
#endif  // !BUILDFLAG(IS_CHROMEOS_ASH)

void SettingsUI::CreateHelpBubbleHandler(
    mojo::PendingRemote<help_bubble::mojom::HelpBubbleClient> client,
    mojo::PendingReceiver<help_bubble::mojom::HelpBubbleHandler> handler) {
  help_bubble_handler_ = std::make_unique<user_education::HelpBubbleHandler>(
      std::move(handler), std::move(client), this,
      std::vector<ui::ElementIdentifier>{
          kEnhancedProtectionSettingElementId,
          kAnonymizedUrlCollectionPersonalizationSettingId,
          kInactiveTabSettingElementId,
          kAutofillPredictionImprovementsHeaderElementId,
      });
}

void SettingsUI::CreateCustomizeColorSchemeModeHandler(
    mojo::PendingRemote<
        customize_color_scheme_mode::mojom::CustomizeColorSchemeModeClient>
        client,
    mojo::PendingReceiver<
        customize_color_scheme_mode::mojom::CustomizeColorSchemeModeHandler>
        handler) {
  customize_color_scheme_mode_handler_ =
      std::make_unique<CustomizeColorSchemeModeHandler>(
          std::move(client), std::move(handler), Profile::FromWebUI(web_ui()));
}

void SettingsUI::BindInterface(
    mojo::PendingReceiver<customize_color_scheme_mode::mojom::
                              CustomizeColorSchemeModeHandlerFactory>
        pending_receiver) {
  if (customize_color_scheme_mode_handler_factory_receiver_.is_bound()) {
    customize_color_scheme_mode_handler_factory_receiver_.reset();
  }
  customize_color_scheme_mode_handler_factory_receiver_.Bind(
      std::move(pending_receiver));
}

WEB_UI_CONTROLLER_TYPE_IMPL(SettingsUI)

}  // namespace settings
