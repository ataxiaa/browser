// Copyright 2022 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/desks_storage/core/desk_model_wrapper.h"

#include "ash/public/cpp/desk_template.h"
#include "base/memory/raw_ptr.h"
#include "base/uuid.h"
#include "components/account_id/account_id.h"
#include "components/desks_storage/core/desk_model.h"
#include "components/desks_storage/core/local_desk_data_manager.h"

namespace desks_storage {

DeskModelWrapper::DeskModelWrapper(
    desks_storage::DeskModel* saved_desks_and_groups_model)
    : saved_desks_and_groups_model_(saved_desks_and_groups_model) {}

DeskModelWrapper::~DeskModelWrapper() = default;

DeskModel::GetAllEntriesResult DeskModelWrapper::GetAllEntries() {
  DeskModel::GetAllEntriesResult templates_result =
      desk_template_model_->GetAllEntries();

  if (templates_result.status != DeskModel::GetAllEntriesStatus::kOk) {
    return templates_result;
  }

  DeskModel::GetAllEntriesResult saved_desk_or_group_result =
      saved_desks_and_groups_model_->GetAllEntries();

  if (saved_desk_or_group_result.status !=
      DeskModel::GetAllEntriesStatus::kOk) {
    return saved_desk_or_group_result;
  }

  std::vector<raw_ptr<const ash::DeskTemplate, VectorExperimental>>&
      all_entries = templates_result.entries;

  for (const ash::DeskTemplate* const entry :
       saved_desk_or_group_result.entries) {
    all_entries.push_back(entry);
  }

  for (const auto& it : policy_entries_)
    all_entries.push_back(it.get());

  return DeskModel::GetAllEntriesResult(DeskModel::GetAllEntriesStatus::kOk,
                                        std::move(all_entries));
}

DeskModel::GetEntryByUuidResult DeskModelWrapper::GetEntryByUUID(
    const base::Uuid& uuid) {
  // Check if this is an admin template uuid first.
  std::unique_ptr<ash::DeskTemplate> policy_entry =
      GetAdminDeskTemplateByUUID(uuid);

  if (policy_entry) {
    return DeskModel::GetEntryByUuidResult(GetEntryByUuidStatus::kOk,
                                           std::move(policy_entry));
  }

  if (GetDeskTemplateModel()->HasUuid(uuid)) {
    return GetDeskTemplateModel()->GetEntryByUUID(uuid);
  } else {
    return saved_desks_and_groups_model_->GetEntryByUUID(uuid);
  }
}

void DeskModelWrapper::AddOrUpdateEntry(
    std::unique_ptr<ash::DeskTemplate> new_entry,
    DeskModel::AddOrUpdateEntryCallback callback) {
  switch (new_entry->type()) {
    case ash::DeskTemplateType::kTemplate:
    case ash::DeskTemplateType::kFloatingWorkspace:
      GetDeskTemplateModel()->AddOrUpdateEntry(std::move(new_entry),
                                               std::move(callback));
      return;
    case ash::DeskTemplateType::kSaveAndRecall:
    case ash::DeskTemplateType::kCoral:
      saved_desks_and_groups_model_->AddOrUpdateEntry(std::move(new_entry),
                                                      std::move(callback));
      return;
    // Return kInvalidArgument on an unknown desk type.
    case ash::DeskTemplateType::kUnknown:
      std::move(callback).Run(AddOrUpdateEntryStatus::kInvalidArgument,
                              std::move(new_entry));
      return;
  }
}

void DeskModelWrapper::DeleteEntry(const base::Uuid& uuid,
                                   DeskModel::DeleteEntryCallback callback) {
  auto status = std::make_unique<DeskModel::DeleteEntryStatus>();
  if (GetDeskTemplateModel()->HasUuid(uuid)) {
    GetDeskTemplateModel()->DeleteEntry(uuid, std::move(callback));
  } else {
    saved_desks_and_groups_model_->DeleteEntry(uuid, std::move(callback));
  }
}

void DeskModelWrapper::DeleteAllEntries(
    DeskModel::DeleteEntryCallback callback) {
  DeskModel::DeleteEntryStatus desk_template_delete_status =
      GetDeskTemplateModel()->DeleteAllEntriesSync();
  if (desk_template_delete_status != DeskModel::DeleteEntryStatus::kOk) {
    std::move(callback).Run(desk_template_delete_status);
    return;
  }
  saved_desks_and_groups_model_->DeleteAllEntries(
      base::BindOnce(&DeskModelWrapper::OnDeleteAllEntries,
                     weak_ptr_factory_.GetWeakPtr(), std::move(callback)));
}

// TODO(crbug.com/1320805): Remove this function once both desk models support
// desk type counts.
size_t DeskModelWrapper::GetEntryCount() const {
  return GetCoralEntryCount() + GetSaveAndRecallDeskEntryCount() +
         GetDeskTemplateEntryCount();
}

size_t DeskModelWrapper::GetSaveAndRecallDeskEntryCount() const {
  return saved_desks_and_groups_model_->GetSaveAndRecallDeskEntryCount();
}

size_t DeskModelWrapper::GetDeskTemplateEntryCount() const {
  return GetDeskTemplateModel()->GetDeskTemplateEntryCount() +
         policy_entries_.size();
}

size_t DeskModelWrapper::GetCoralEntryCount() const {
  return saved_desks_and_groups_model_->GetCoralEntryCount();
}

size_t DeskModelWrapper::GetMaxSaveAndRecallDeskEntryCount() const {
  return saved_desks_and_groups_model_->GetMaxSaveAndRecallDeskEntryCount();
}

size_t DeskModelWrapper::GetMaxDeskTemplateEntryCount() const {
  return GetDeskTemplateModel()->GetMaxDeskTemplateEntryCount() +
         policy_entries_.size();
}

size_t DeskModelWrapper::GetMaxCoralEntryCount() const {
  return saved_desks_and_groups_model_->GetMaxCoralEntryCount();
}

std::set<base::Uuid> DeskModelWrapper::GetAllEntryUuids() const {
  std::set<base::Uuid> keys;

  for (const auto& it : policy_entries_)
    keys.emplace(it.get()->uuid());

  for (const auto& saved_desk_or_group_uuid :
       saved_desks_and_groups_model_->GetAllEntryUuids()) {
    keys.emplace(saved_desk_or_group_uuid);
  }

  for (const auto& desk_template_uuid :
       GetDeskTemplateModel()->GetAllEntryUuids()) {
    keys.emplace(desk_template_uuid);
  }
  return keys;
}

bool DeskModelWrapper::IsReady() const {
  return saved_desks_and_groups_model_->IsReady() &&
         GetDeskTemplateModel()->IsReady();
}

bool DeskModelWrapper::IsSyncing() const {
  return GetDeskTemplateModel()->IsSyncing();
}

ash::DeskTemplate* DeskModelWrapper::FindOtherEntryWithName(
    const std::u16string& name,
    ash::DeskTemplateType type,
    const base::Uuid& uuid) const {
  switch (type) {
    case ash::DeskTemplateType::kTemplate:
    case ash::DeskTemplateType::kFloatingWorkspace:
      return GetDeskTemplateModel()->FindOtherEntryWithName(name, type, uuid);
    case ash::DeskTemplateType::kSaveAndRecall:
    case ash::DeskTemplateType::kCoral:
      return saved_desks_and_groups_model_->FindOtherEntryWithName(name, type,
                                                                   uuid);
    case ash::DeskTemplateType::kUnknown:
      return nullptr;
  }
}

std::string DeskModelWrapper::GetCacheGuid() {
  return GetDeskTemplateModel()->GetCacheGuid();
}
desks_storage::DeskSyncBridge* DeskModelWrapper::GetDeskTemplateModel() const {
  DCHECK(desk_template_model_);
  return desk_template_model_;
}

void DeskModelWrapper::OnDeleteAllEntries(
    DeskModel::DeleteEntryCallback callback,
    desks_storage::DeskModel::DeleteEntryStatus status) {
  std::move(callback).Run(status);
}

}  // namespace desks_storage
