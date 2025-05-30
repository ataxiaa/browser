// Copyright 2024 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/segmentation_platform/embedder/home_modules/send_tab_notification_promo.h"

#include "base/metrics/field_trial_params.h"
#include "build/build_config.h"
#include "components/segmentation_platform/embedder/home_modules/constants.h"
#include "components/segmentation_platform/embedder/home_modules/ephemeral_module_utils.h"
#include "components/segmentation_platform/embedder/home_modules/home_modules_card_registry.h"
#include "components/segmentation_platform/internal/metadata/feature_query.h"
#include "components/segmentation_platform/internal/metadata/metadata_writer.h"
#include "components/segmentation_platform/public/features.h"
#include "components/segmentation_platform/public/proto/model_metadata.pb.h"
#include "components/send_tab_to_self/features.h"

namespace {

// The maximum number of times a card can be visible to the user.
const int kMaxSendTabNotificationCardImpressions = 1;

}  // namespace

namespace segmentation_platform {

namespace home_modules {

const char kSendTabInfobarReceivedInLastSessionSignalKey[] =
    "send_tab_infobar_received_in_last_session";

SendTabNotificationPromo::SendTabNotificationPromo(int send_tab_promo_count)
    : CardSelectionInfo(kSendTabNotificationPromo) {}

std::map<SignalKey, FeatureQuery> SendTabNotificationPromo::GetInputs() {
  std::map<SignalKey, FeatureQuery> map = {
      {kSendTabInfobarReceivedInLastSessionSignalKey,
       FeatureQuery::FromCustomInput(MetadataWriter::CustomInput{
           .tensor_length = 1,
           .fill_policy = proto::CustomInput::FILL_FROM_INPUT_CONTEXT,
           .name = kSendTabInfobarReceivedInLastSession})}};
  return map;
}

CardSelectionInfo::ShowResult SendTabNotificationPromo::ComputeCardResult(
    const CardSelectionSignals& signals) const {
  // Check for a forced `ShowResult`.
  std::optional<CardSelectionInfo::ShowResult> forced_result =
      GetForcedEphemeralModuleShowResult();

  if (forced_result.has_value() &&
      forced_result.value().result_label.has_value() &&
      kSendTabNotificationPromo == forced_result.value().result_label.value()) {
    return forced_result.value();
  }

  CardSelectionInfo::ShowResult result;
  result.result_label = kSendTabNotificationPromo;

  if (*signals.GetSignal(kSendTabInfobarReceivedInLastSessionSignalKey)) {
    result.position = EphemeralHomeModuleRank::kTop;
    return result;
  }

  result.position = EphemeralHomeModuleRank::kNotShown;
  return result;
}

bool SendTabNotificationPromo::IsEnabled(int impression_count) {
#if BUILDFLAG(IS_IOS)
  if (!send_tab_to_self::
          IsSendTabIOSPushNotificationsEnabledWithMagicStackCard()) {
    return false;
  }
#endif  // BUILDFLAG(IS_IOS)

  std::optional<CardSelectionInfo::ShowResult> forced_result =
      GetForcedEphemeralModuleShowResult();

  // If forced to show/hide and the module label matches the current module,
  // return true/false accordingly.
  if (forced_result.has_value() &&
      forced_result.value().result_label.has_value() &&
      kSendTabNotificationPromo == forced_result.value().result_label.value()) {
    return forced_result.value().position == EphemeralHomeModuleRank::kTop;
  }

  if (impression_count < kMaxSendTabNotificationCardImpressions) {
    return true;
  }

  return false;
}

}  // namespace home_modules
}  // namespace segmentation_platform
