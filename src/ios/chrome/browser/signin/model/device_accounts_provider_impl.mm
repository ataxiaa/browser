// Copyright 2016 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/signin/model/device_accounts_provider_impl.h"

#import "base/check.h"
#import "base/notreached.h"
#import "base/strings/sys_string_conversions.h"
#import "base/task/sequenced_task_runner.h"
#import "base/types/pass_key.h"
#import "components/signin/public/identity_manager/account_info.h"
#import "components/signin/public/identity_manager/signin_constants.h"
#import "ios/chrome/browser/shared/model/application_context/application_context.h"
#import "ios/chrome/browser/signin/model/chrome_account_manager_service.h"
#import "ios/chrome/browser/signin/model/constants.h"
#import "ios/chrome/browser/signin/model/signin_util.h"
#import "ios/chrome/browser/signin/model/system_identity.h"
#import "ios/chrome/browser/signin/model/system_identity_manager.h"
#import "ios/public/provider/chrome/browser/signin/signin_error_api.h"

using signin::constants::kNoHostedDomainFound;

namespace {

using AccessTokenResult = DeviceAccountsProvider::AccessTokenResult;
using AccountInfo = DeviceAccountsProvider::AccountInfo;

// Helper function converting `error` for `identity` to an
// AuthenticationErrorCategory.
AuthenticationErrorCategory AuthenticationErrorCategoryFromError(
    id<SystemIdentity> identity,
    NSError* error) {
  DCHECK(error);
  if ([error.domain isEqualToString:kSystemIdentityManagerErrorDomain]) {
    SystemIdentityManagerErrorCode error_code =
        static_cast<SystemIdentityManagerErrorCode>(error.code);
    switch (error_code) {
      case SystemIdentityManagerErrorCode::kNoAuthenticatedIdentity:
        return kAuthenticationErrorCategoryUnknownIdentityErrors;
      case SystemIdentityManagerErrorCode::kClientIDMismatch:
        return kAuthenticationErrorCategoryUnknownIdentityErrors;
      case SystemIdentityManagerErrorCode::kInvalidTokenIdentity:
        return kAuthenticationErrorCategoryAuthorizationErrors;
    }
  }

  SystemIdentityManager* system_identity_manager =
      GetApplicationContext()->GetSystemIdentityManager();
  if (system_identity_manager->IsMDMError(identity, error)) {
    return kAuthenticationErrorCategoryAuthorizationErrors;
  }

  switch (ios::provider::GetSigninErrorCategory(error)) {
    case ios::provider::SigninErrorCategory::kUnknownError:
      return kAuthenticationErrorCategoryUnknownErrors;
    case ios::provider::SigninErrorCategory::kAuthorizationError:
      return kAuthenticationErrorCategoryAuthorizationErrors;
    case ios::provider::SigninErrorCategory::kAuthorizationForbiddenError:
      return kAuthenticationErrorCategoryAuthorizationForbiddenErrors;
    case ios::provider::SigninErrorCategory::kNetworkError:
      return kAuthenticationErrorCategoryNetworkServerErrors;
    case ios::provider::SigninErrorCategory::kUserCancellationError:
      return kAuthenticationErrorCategoryUserCancellationErrors;
  }

  NOTREACHED() << "unexpected error: "
               << base::SysNSStringToUTF8([error description]);
}

// Helper function converting the result of fetching the access token from
// what SystemIdentityManager pass to the callback to what is expected for
// AccessTokenCallback.
AccessTokenResult AccessTokenResultFrom(
    id<SystemIdentity> identity,
    std::optional<SystemIdentityManager::AccessTokenInfo> access_token,
    NSError* error) {
  if (error) {
    return base::unexpected(
        AuthenticationErrorCategoryFromError(identity, error));
  } else {
    DCHECK(access_token.has_value());
    DeviceAccountsProvider::AccessTokenInfo info = {
        access_token->token,
        access_token->expiration_time,
    };
    return base::ok(std::move(info));
  }
}

std::vector<DeviceAccountsProvider::AccountInfo>
ConvertSystemIdentitiesToAccountInfos(NSArray<id<SystemIdentity>>* identities) {
  std::vector<AccountInfo> result;
  result.reserve(identities.count);

  SystemIdentityManager* system_identity_manager =
      GetApplicationContext()->GetSystemIdentityManager();

  for (id<SystemIdentity> identity : identities) {
    CHECK(identity);
    AccountInfo account_info;
    account_info.gaia = base::SysNSStringToUTF8(identity.gaiaID);
    account_info.email = base::SysNSStringToUTF8(identity.userEmail);

    // If hosted domain is nil, then it means the information has not been
    // fetched from gaia; in that case, set account_info.hosted_domain to
    // an empty string. Otherwise, set it to the value of the hostedDomain
    // or kNoHostedDomainFound if the string is empty.
    NSString* hosted_domain =
        system_identity_manager->GetCachedHostedDomainForIdentity(identity);
    if (hosted_domain) {
      account_info.hosted_domain = hosted_domain.length
                                       ? base::SysNSStringToUTF8(hosted_domain)
                                       : kNoHostedDomainFound;
    }
    result.push_back(std::move(account_info));
  }

  return result;
}

}  // anonymous namespace

DeviceAccountsProviderImpl::DeviceAccountsProviderImpl(
    ChromeAccountManagerService* account_manager_service)
    : account_manager_service_(account_manager_service) {
  DCHECK(account_manager_service_);
  chrome_account_manager_observation_.Observe(account_manager_service_);
}

DeviceAccountsProviderImpl::~DeviceAccountsProviderImpl() = default;

void DeviceAccountsProviderImpl::AddObserver(
    DeviceAccountsProvider::Observer* observer) {
  observer_list_.AddObserver(observer);
}

void DeviceAccountsProviderImpl::RemoveObserver(
    DeviceAccountsProvider::Observer* observer) {
  observer_list_.RemoveObserver(observer);
}

void DeviceAccountsProviderImpl::GetAccessToken(
    const std::string& gaia_id,
    const std::string& client_id,
    const std::set<std::string>& scopes,
    AccessTokenCallback callback) {
  DCHECK(!callback.is_null());
  id<SystemIdentity> identity =
      account_manager_service_->GetIdentityWithGaiaID(gaia_id);

  // If the identity is unknown, there is no need to try to fetch the access
  // token as it will fail immediately. Post the callback with a failure.
  if (!identity) {
    base::SequencedTaskRunner::GetCurrentDefault()->PostTask(
        FROM_HERE,
        base::BindOnce(std::move(callback),
                       base::unexpected(
                           kAuthenticationErrorCategoryUnknownIdentityErrors)));
    return;
  }

  GetApplicationContext()->GetSystemIdentityManager()->GetAccessToken(
      identity, client_id, scopes,
      base::BindOnce(&AccessTokenResultFrom, identity)
          .Then(std::move(callback)));
}

std::vector<DeviceAccountsProvider::AccountInfo>
DeviceAccountsProviderImpl::GetAccountsForProfile() const {
  NSArray<id<SystemIdentity>>* identities =
      account_manager_service_->GetAllIdentities();
  return ConvertSystemIdentitiesToAccountInfos(identities);
}

std::vector<DeviceAccountsProvider::AccountInfo>
DeviceAccountsProviderImpl::GetAccountsOnDevice() const {
  NSArray<id<SystemIdentity>>* identities =
      account_manager_service_->GetAllIdentitiesOnDevice(
          base::PassKey<DeviceAccountsProviderImpl>());
  return ConvertSystemIdentitiesToAccountInfos(identities);
}

void DeviceAccountsProviderImpl::OnIdentitiesOnDeviceChanged() {
  for (auto& observer : observer_list_) {
    observer.OnAccountsOnDeviceChanged();
  }
}
