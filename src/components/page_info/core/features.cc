// Copyright 2020 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/page_info/core/features.h"

#include <string_view>

#include "base/containers/contains.h"
#include "base/containers/fixed_flat_set.h"
#include "base/feature_list.h"
#include "base/metrics/field_trial_params.h"
#include "base/strings/string_util.h"
#include "build/build_config.h"
#include "ui/base/l10n/l10n_util.h"

namespace page_info {
constexpr auto kDefaultLangs = base::MakeFixedFlatSet<std::string_view>({
    "ar", "bg", "ca", "cs", "da", "de", "el", "en", "es", "et",
    "fi", "fr", "he", "hi", "hr", "hu", "id", "it", "ja", "ko",
    "lt", "lv", "nb", "nl", "pl", "pt", "ro", "ru", "sk", "sl",
    "sr", "sv", "sw", "th", "tr", "uk", "vi", "zh",
});

extern bool IsAboutThisSiteFeatureEnabled(const std::string& locale) {
  std::string lang = l10n_util::GetLanguage(locale);
  if (base::Contains(kDefaultLangs, lang)) {
    return base::FeatureList::IsEnabled(kPageInfoAboutThisSite);
  }
  return base::FeatureList::IsEnabled(kPageInfoAboutThisSiteMoreLangs);
}

BASE_FEATURE(kPageInfoAboutThisSite,
             "PageInfoAboutThisSite",
             base::FEATURE_ENABLED_BY_DEFAULT);
BASE_FEATURE(kPageInfoAboutThisSiteMoreLangs,
             "PageInfoAboutThisSiteMoreLangs",
             base::FEATURE_DISABLED_BY_DEFAULT);

const base::FeatureParam<bool> kShowSampleContent{&kPageInfoAboutThisSite,
                                                  "ShowSampleContent", false};

#if !BUILDFLAG(IS_ANDROID)
BASE_FEATURE(kPageInfoHistoryDesktop,
             "PageInfoHistoryDesktop",
             base::FEATURE_DISABLED_BY_DEFAULT);

BASE_FEATURE(kPageInfoHideSiteSettings,
             "PageInfoHideSiteSettings",
             base::FEATURE_DISABLED_BY_DEFAULT);

#endif  // !BUILDFLAG(IS_ANDROID)

BASE_FEATURE(kMerchantTrust,
             "MerchantTrust",
             base::FEATURE_DISABLED_BY_DEFAULT);

const char kMerchantTrustEnabledWithSampleDataName[] =
    "enabled-with-sample-data";
const base::FeatureParam<bool> kMerchantTrustEnabledWithSampleData{
    &kMerchantTrust, kMerchantTrustEnabledWithSampleDataName, false};

const char kMerchantTrustEnabledForCountry[] = "us";
const char kMerchantTrustEnabledForLocale[] = "en-us";

const char kMerchantTrustForceShowUIForTestingName[] =
    "force-show-ui-for-testing";
const base::FeatureParam<bool> kMerchantTrustForceShowUIForTesting{
    &kMerchantTrust, kMerchantTrustForceShowUIForTestingName, false};

extern bool IsMerchantTrustFeatureEnabled(const std::string& country_code,
                                          const std::string& locale) {
  if (kMerchantTrustForceShowUIForTesting.Get()) {
    return true;
  }

  return base::FeatureList::IsEnabled(kMerchantTrust) &&
         base::ToLowerASCII(country_code) == kMerchantTrustEnabledForCountry &&
         base::ToLowerASCII(locale) == kMerchantTrustEnabledForLocale;
}

}  // namespace page_info
