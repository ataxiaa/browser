// Copyright 2022 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/enterprise/identifiers/profile_id_delegate_impl.h"

#include "base/check.h"
#include "base/hash/sha1.h"
#include "base/uuid.h"
#include "build/buildflag.h"
#include "chrome/browser/profiles/profile.h"
#include "chrome/browser/profiles/profile_manager.h"
#include "components/enterprise/browser/identifiers/identifiers_prefs.h"
#include "components/prefs/pref_service.h"

#if !BUILDFLAG(IS_CHROMEOS)
#include "components/enterprise/browser/controller/browser_dm_token_storage.h"
#else
#include "components/policy/core/common/cloud/cloud_policy_util.h"
#endif  // !BUILDFLAG(IS_CHROMEOS)

#if BUILDFLAG(IS_WIN)
#include "base/strings/utf_string_conversions.h"
#include "base/win/wmi.h"
#endif  // BUILDFLAG(IS_WIN)

namespace enterprise {

namespace {

// Creates and persists the profile GUID if one does not already exist
void CreateProfileGUID(Profile* profile, const base::FilePath& profile_path) {
  auto* prefs = profile->GetPrefs();
  if (!prefs->GetString(kProfileGUIDPref).empty()) {
    return;
  }

  auto* preset_profile_management_data =
      PresetProfileManagmentData::Get(profile);
  std::string preset_profile_guid = preset_profile_management_data->GetGuid();

  std::string new_profile_guid =
      (preset_profile_guid.empty())
          ? base::Uuid::GenerateRandomV4().AsLowercaseString()
          : std::move(preset_profile_guid);

  prefs->SetString(kProfileGUIDPref, new_profile_guid);
  preset_profile_management_data->ClearGuid();
}

}  // namespace

PresetProfileManagmentData* PresetProfileManagmentData::Get(Profile* profile) {
  CHECK(profile);

  if (!profile->GetUserData(kPresetProfileManagementData)) {
    profile->SetUserData(
        kPresetProfileManagementData,
        std::make_unique<PresetProfileManagmentData>(std::string()));
  }

  return static_cast<PresetProfileManagmentData*>(
      profile->GetUserData(kPresetProfileManagementData));
}

void PresetProfileManagmentData::SetGuid(std::string guid) {
  CHECK(!guid.empty());
  CHECK(guid_.empty());

  guid_ = guid;
}

std::string PresetProfileManagmentData::GetGuid() {
  return guid_;
}

void PresetProfileManagmentData::ClearGuid() {
  guid_ = std::string();
}

PresetProfileManagmentData::PresetProfileManagmentData(std::string preset_guid)
    : guid_(preset_guid) {}

ProfileIdDelegateImpl::ProfileIdDelegateImpl(Profile* profile)
    : profile_(profile) {
  CHECK(profile_);
  CreateProfileGUID(profile_, profile->GetPath());
}

ProfileIdDelegateImpl::~ProfileIdDelegateImpl() = default;

std::string ProfileIdDelegateImpl::GetDeviceId() {
  return ProfileIdDelegateImpl::GetId();
}

#if !BUILDFLAG(IS_CHROMEOS)
// Gets the device ID from the BrowserDMTokenStorage.
std::string ProfileIdDelegateImpl::GetId() {
  std::string device_id =
      policy::BrowserDMTokenStorage::Get()->RetrieveClientId();

// On Windows, the combination of the client ID and device serial
// number are used to form the device ID.
#if BUILDFLAG(IS_WIN)
  // Serial number could be empty for various reasons. However, we should still
  // generate a profile ID with whatever we have. Devices without serial number
  // will have higher chance of twin issue but it is still better than no ID at
  // all.
  device_id += base::WideToUTF8(base::win::WmiComputerSystemInfo::Get().serial_number());
#endif  // BUILDFLAG(IS_WIN)

  return device_id;
}
#else
// Gets the device ID from cloud policy.
std::string ProfileIdDelegateImpl::GetId() {
  std::string device_id = policy::GetDeviceName();

  return device_id;
}
#endif  // !BUILDFLAG(IS_CHROMEOS)

}  // namespace enterprise
