// Copyright 2021 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "android_webview/nonembedded/component_updater/aw_component_update_service.h"

#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include "android_webview/common/aw_paths.h"
#include "android_webview/nonembedded/component_updater/aw_component_update_service.h"
#include "android_webview/nonembedded/component_updater/aw_component_updater_configurator.h"
#include "android_webview/nonembedded/component_updater/registration.h"
#include "android_webview/nonembedded/webview_apk_process.h"
#include "base/android/callback_android.h"
#include "base/android/jni_string.h"
#include "base/android/path_utils.h"
#include "base/android/scoped_java_ref.h"
#include "base/check.h"
#include "base/command_line.h"
#include "base/containers/contains.h"
#include "base/files/file_enumerator.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/files/important_file_writer.h"
#include "base/files/scoped_temp_dir.h"
#include "base/functional/callback_helpers.h"
#include "base/json/json_string_value_serializer.h"
#include "base/json/values_util.h"
#include "base/logging.h"
#include "base/no_destructor.h"
#include "base/notreached.h"
#include "base/path_service.h"
#include "base/strings/string_number_conversions.h"
#include "base/task/single_thread_task_runner.h"
#include "base/time/time.h"
#include "base/values.h"
#include "base/version.h"
#include "components/component_updater/android/component_loader_policy.h"
#include "components/component_updater/component_installer.h"
#include "components/component_updater/component_updater_service.h"
#include "components/component_updater/component_updater_utils.h"
#include "components/update_client/crx_update_item.h"
#include "components/update_client/update_client.h"
#include "components/update_client/utils.h"

// Must come after all headers that specialize FromJniType() / ToJniType().
#include "android_webview/nonembedded/nonembedded_jni_headers/AwComponentUpdateService_jni.h"
#include "android_webview/nonembedded/nonembedded_jni_headers/ComponentsProviderPathUtil_jni.h"

namespace android_webview {

// static
AwComponentUpdateService* AwComponentUpdateService::GetInstance() {
  static base::NoDestructor<AwComponentUpdateService> instance;
  return instance.get();
}

// static
void JNI_AwComponentUpdateService_StartComponentUpdateService(
    JNIEnv* env,
    const base::android::JavaParamRef<jobject>& j_finished_callback,
    jboolean j_on_demand_update) {
  AwComponentUpdateService::GetInstance()->StartComponentUpdateService(
      base::BindOnce(
          &base::android::RunIntCallbackAndroid,
          base::android::ScopedJavaGlobalRef<jobject>(j_finished_callback)),
      j_on_demand_update);
}

AwComponentUpdateService::AwComponentUpdateService()
    : AwComponentUpdateService(MakeAwComponentUpdaterConfigurator(
          base::CommandLine::ForCurrentProcess(),
          WebViewApkProcess::GetInstance()->GetPrefService())) {}

AwComponentUpdateService::AwComponentUpdateService(
    scoped_refptr<update_client::Configurator> configurator)
    : update_client_(update_client::UpdateClientFactory(configurator)),
      configurator_(configurator) {}

AwComponentUpdateService::~AwComponentUpdateService() = default;

// Start ComponentUpdateService once.
void AwComponentUpdateService::StartComponentUpdateService(
    UpdateCallback finished_callback,
    bool on_demand_update) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  RegisterComponents(
      base::BindRepeating(&AwComponentUpdateService::RegisterComponent,
                          base::Unretained(this)),
      base::BindOnce(
          &AwComponentUpdateService::ScheduleUpdatesOfRegisteredComponents,
          weak_ptr_factory_.GetWeakPtr(),
          base::BindOnce(&AwComponentUpdateService::UpdateMetadataFiles,
                         weak_ptr_factory_.GetWeakPtr(),
                         std::move(finished_callback)),
          on_demand_update));
}

bool AwComponentUpdateService::RegisterComponent(
    const component_updater::ComponentRegistration& component) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  // TODO(crbug.com/40750393): Add the histograms being logged in
  // CrxUpdateService once we support logging metrics from nonembedded WebView.

  if (component.app_id.empty() || !component.version.IsValid() ||
      !component.installer) {
    return false;
  }

  // Update the registration data if the component has been registered before.
  auto it = components_.find(component.app_id);
  if (it != components_.end()) {
    it->second = component;
    return true;
  }

  components_.insert(std::make_pair(component.app_id, component));
  components_order_.push_back(component.app_id);
  return true;
}

void AwComponentUpdateService::CheckForUpdates(UpdateCallback on_finished,
                                               bool on_demand_update) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);

  // TODO(crbug.com/40750393): Add the histograms being logged in
  // CrxUpdateService once we support logging metrics from nonembedded WebView.

  std::vector<std::string> secure_ids;    // Require HTTPS for update checks.
  std::vector<std::string> unsecure_ids;  // Can fallback to HTTP.
  for (const auto& id : components_order_) {
    DCHECK(base::Contains(components_, id));

    const auto component = component_updater::GetComponent(components_, id);
    if (!component || component->requires_network_encryption)
      secure_ids.push_back(id);
    else
      unsecure_ids.push_back(id);
  }

  if (unsecure_ids.empty() && secure_ids.empty()) {
    base::SingleThreadTaskRunner::GetCurrentDefault()->PostTask(
        FROM_HERE, base::BindOnce(std::move(on_finished), 0));
    return;
  }

  auto on_finished_callback =
      base::BindOnce(&AwComponentUpdateService::RecordComponentsUpdated,
                     weak_ptr_factory_.GetWeakPtr(), std::move(on_finished));

  // Reset updated components counter.
  components_updated_count_ = 0;

  if (!unsecure_ids.empty()) {
    update_client_->Update(
        unsecure_ids,
        base::BindOnce(&AwComponentUpdateService::GetCrxComponents,
                       base::Unretained(this)),
        {}, on_demand_update,
        base::BindOnce(&AwComponentUpdateService::OnUpdateComplete,
                       weak_ptr_factory_.GetWeakPtr(),
                       secure_ids.empty() ? std::move(on_finished_callback)
                                          : update_client::Callback(),
                       base::TimeTicks::Now()));
  }

  if (!secure_ids.empty()) {
    update_client_->Update(
        secure_ids,
        base::BindOnce(&AwComponentUpdateService::GetCrxComponents,
                       base::Unretained(this)),
        {}, on_demand_update,
        base::BindOnce(&AwComponentUpdateService::OnUpdateComplete,
                       weak_ptr_factory_.GetWeakPtr(),
                       std::move(on_finished_callback),
                       base::TimeTicks::Now()));
  }

  return;
}

void AwComponentUpdateService::OnUpdateComplete(
    update_client::Callback callback,
    const base::TimeTicks& start_time,
    update_client::Error error) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  // TODO(crbug.com/40750393): Add the histograms being logged in
  // CrxUpdateService once we support logging metrics from nonembedded WebView.

  if (!callback.is_null()) {
    base::SingleThreadTaskRunner::GetCurrentDefault()->PostTask(
        FROM_HERE, base::BindOnce(std::move(callback), error));
  }
}

update_client::CrxComponent AwComponentUpdateService::ToCrxComponent(
    const component_updater::ComponentRegistration& component) const {
  update_client::CrxComponent crx;
  crx.pk_hash = component.public_key_hash;
  crx.app_id = component.app_id;
  crx.installer = component.installer;
  crx.action_handler = component.action_handler;
  crx.version = component.version;
  crx.fingerprint = component.fingerprint;
  crx.name = component.name;
  crx.installer_attributes = component.installer_attributes;
  crx.requires_network_encryption = component.requires_network_encryption;

  crx.crx_format_requirement =
      crx_file::VerifierFormat::CRX3_WITH_PUBLISHER_PROOF;

  return crx;
}

std::optional<component_updater::ComponentRegistration>
AwComponentUpdateService::GetComponent(const std::string& id) const {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  return component_updater::GetComponent(components_, id);
}

void AwComponentUpdateService::GetCrxComponents(
    const std::vector<std::string>& ids,
    base::OnceCallback<
        void(const std::vector<std::optional<update_client::CrxComponent>>&)>
        callback) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  std::vector<std::optional<update_client::CrxComponent>> crxs;
  for (std::optional<component_updater::ComponentRegistration> item :
       component_updater::GetCrxComponents(components_, ids)) {
    crxs.push_back(
        item ? std::optional<update_client::CrxComponent>{ToCrxComponent(*item)}
             : std::nullopt);
  }
  std::move(callback).Run(crxs);
}

void AwComponentUpdateService::ScheduleUpdatesOfRegisteredComponents(
    UpdateCallback on_finished_updates,
    bool on_demand_update) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  CheckForUpdates(std::move(on_finished_updates), on_demand_update);
}

void AwComponentUpdateService::RegisterComponents(
    RegisterComponentsCallback register_callback,
    base::OnceClosure on_finished) {
  RegisterComponentsForUpdate(register_callback, std::move(on_finished));
}

void AwComponentUpdateService::IncrementComponentsUpdatedCount() {
  components_updated_count_++;
}

void AwComponentUpdateService::RecordComponentsUpdated(
    UpdateCallback on_finished,
    update_client::Error error) {
  std::move(on_finished).Run(components_updated_count_);
}

std::string AwComponentUpdateService::GetCohortId(
    const std::string& component_id) {
  return configurator_->GetPersistedData()->GetCohort(component_id);
}

void AwComponentUpdateService::UpdateMetadataFiles(
    UpdateCallback on_finished,
    int32_t components_updated_count) {
  DCHECK_CALLED_ON_VALID_SEQUENCE(sequence_checker_);
  for (const auto& component_entry : components_) {
    const std::string& component_id = component_entry.first;
    const auto& component = component_entry.second;

    base::FilePath cps_component_base_path =
        GetComponentsProviderServiceDirectory(component.public_key_hash);
    std::string highest_sequence_number_dir =
        GetHighestSequenceNumberDirectory(cps_component_base_path);

    // If no directory was found, continue to the next component.
    if (highest_sequence_number_dir.empty()) {
      LOG(ERROR) << "No directory found for component " << component_id;
      continue;
    }

    base::FilePath dest_path =
        cps_component_base_path.AppendASCII(highest_sequence_number_dir);

    base::Value::Dict metadata_file_contents;
    metadata_file_contents.Set(component_updater::kMetadataFileCohortIdKey,
                               GetCohortId(component_id));
    std::string metadata_file_contents_json;
    JSONStringValueSerializer serializer(&metadata_file_contents_json);
    if (!serializer.Serialize(metadata_file_contents)) {
      LOG(ERROR) << "Failed to serialize metadata for component "
                 << component_id;
      continue;
    }

    // Write the metadata JSON to the destination path.
    base::FilePath metadata_file_path =
        dest_path.Append("aw_extra_component_metadata.json");
    if (!base::ImportantFileWriter::WriteFileAtomically(
            metadata_file_path, metadata_file_contents_json)) {
      LOG(ERROR) << "Failed to write metadata file for component "
                 << component_id << " at " << metadata_file_path;
    }
  }

  std::move(on_finished).Run(components_updated_count);
}

std::string AwComponentUpdateService::GetHighestSequenceNumberDirectory(
    base::FilePath cps_component_base_path) {
  JNIEnv* env = jni_zero::AttachCurrentThread();
  return Java_ComponentsProviderPathUtil_getTheHighestSequenceNumberDirectory(
      env, cps_component_base_path.MaybeAsASCII());
}

int AwComponentUpdateService::GetHighestSequenceNumber(
    base::FilePath cps_component_base_path) {
  JNIEnv* env = jni_zero::AttachCurrentThread();
  return Java_ComponentsProviderPathUtil_getTheHighestSequenceNumber(
      env, cps_component_base_path.MaybeAsASCII());
}

base::FilePath AwComponentUpdateService::GetComponentsProviderServiceDirectory(
    const std::vector<uint8_t>& public_key_hash) {
  std::string component_id =
      update_client::GetCrxIdFromPublicKeyHash(public_key_hash);

  JNIEnv* env = jni_zero::AttachCurrentThread();
  return base::FilePath(

             Java_ComponentsProviderPathUtil_getComponentsServingDirectoryPath(
                 env))
      .AppendASCII(component_id);
}

}  // namespace android_webview
