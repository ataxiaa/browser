// Copyright 2023 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/authentication/ui_bundled/signin/instant_signin/instant_signin_coordinator.h"

#import "base/notreached.h"
#import "base/strings/sys_string_conversions.h"
#import "components/signin/public/base/signin_metrics.h"
#import "components/signin/public/identity_manager/identity_manager.h"
#import "ios/chrome/browser/authentication/ui_bundled/authentication_flow/authentication_flow.h"
#import "ios/chrome/browser/authentication/ui_bundled/authentication_ui_util.h"
#import "ios/chrome/browser/authentication/ui_bundled/identity_chooser/identity_chooser_coordinator.h"
#import "ios/chrome/browser/authentication/ui_bundled/identity_chooser/identity_chooser_coordinator_delegate.h"
#import "ios/chrome/browser/authentication/ui_bundled/signin/instant_signin/instant_signin_mediator.h"
#import "ios/chrome/browser/authentication/ui_bundled/signin/interruptible_chrome_coordinator.h"
#import "ios/chrome/browser/authentication/ui_bundled/signin/signin_constants.h"
#import "ios/chrome/browser/authentication/ui_bundled/signin/signin_coordinator+protected.h"
#import "ios/chrome/browser/shared/coordinator/alert/alert_coordinator.h"
#import "ios/chrome/browser/shared/model/browser/browser.h"
#import "ios/chrome/browser/shared/public/features/features.h"
#import "ios/chrome/browser/shared/ui/elements/activity_overlay_coordinator.h"
#import "ios/chrome/browser/signin/model/chrome_account_manager_service.h"
#import "ios/chrome/browser/signin/model/chrome_account_manager_service_factory.h"
#import "ios/chrome/browser/signin/model/identity_manager_factory.h"

@interface InstantSigninCoordinator () <AuthenticationFlowDelegate,
                                        IdentityChooserCoordinatorDelegate,
                                        InstantSigninMediatorDelegate>

@end

@implementation InstantSigninCoordinator {
  // The identity to which the user should be signed-in.
  id<SystemIdentity> _identity;
  // Promo action that triggered the sign-in coordinator.
  signin_metrics::PromoAction _promoAction;
  // Instant sign-in mediator.
  InstantSigninMediator* _mediator;
  // Coordinator for the user to select an account.
  IdentityChooserCoordinator* _identityChooserCoordinator;
  // Coordinator to add an account.
  SigninCoordinator* _addAccountSigninCoordinator;
  // Overlay to block the current window while the sign-in is in progress.
  ActivityOverlayCoordinator* _activityOverlayCoordinator;
  // Action recorded if sign-in succeeded.
  signin_metrics::AccountConsistencyPromoAction _actionToRecordOnSuccess;
}

#pragma mark - Public

- (instancetype)
    initWithBaseViewController:(UIViewController*)viewController
                       browser:(Browser*)browser
                      identity:(id<SystemIdentity>)identity
                   accessPoint:(signin_metrics::AccessPoint)accessPoint
                   promoAction:(signin_metrics::PromoAction)promoAction {
  self = [super initWithBaseViewController:viewController
                                   browser:browser
                               accessPoint:accessPoint];
  if (self) {
    _identity = identity;
    _promoAction = promoAction;
  }
  return self;
}

- (void)dealloc {
  // TODO(crbug.com/40067451): Switch back to DCHECK if the number of reports is
  // low.
  DUMP_WILL_BE_CHECK(!_mediator) << base::SysNSStringToUTF8([self description]);
}

#pragma mark - ChromeCoordinator

- (void)start {
  [super start];
  signin_metrics::LogSignInStarted(self.accessPoint);
  ProfileIOS* profile = self.browser->GetProfile();
  _mediator =
      [[InstantSigninMediator alloc] initWithAccessPoint:self.accessPoint];
  _mediator.delegate = self;

  if (_identity) {
    // If an identity was selected, sign-in can start now.
    // No need to record the event of sign-in started here because the success
    // rate should be high. We're only interested in computing CTRs.
    // TODO(crbug.com/40071752): This is always the default identity today,
    // but nothing prevents a call with a different identity in the future.
    // Check and log accordingly. Logging SIGNED_IN_WITH_NON_DEFAULT_ACCOUNT
    // now would mix the data with the recording further below.
    _actionToRecordOnSuccess = signin_metrics::AccountConsistencyPromoAction::
        SIGNED_IN_WITH_DEFAULT_ACCOUNT;
    [self startSignInOnlyFlow];
    return;
  }

  bool hasAccountOnDevice = false;
  if (AreSeparateProfilesForManagedAccountsEnabled()) {
    signin::IdentityManager* identityManager =
        IdentityManagerFactory::GetForProfile(profile);
    hasAccountOnDevice = !identityManager->GetAccountsOnDevice().empty();
  } else {
    ChromeAccountManagerService* accountManagerService =
        ChromeAccountManagerServiceFactory::GetForProfile(profile);
    hasAccountOnDevice = accountManagerService->HasIdentities();
  }
  if (!hasAccountOnDevice) {
    signin_metrics::RecordConsistencyPromoUserAction(
        signin_metrics::AccountConsistencyPromoAction::
            ADD_ACCOUNT_STARTED_WITH_NO_DEVICE_ACCOUNT,
        self.accessPoint);
    _actionToRecordOnSuccess = signin_metrics::AccountConsistencyPromoAction::
        SIGNED_IN_WITH_NO_DEVICE_ACCOUNT;
    [self startAddAccountForSignInOnly];
    return;
  }
  // Otherwise, the user needs to choose an identity.
  signin_metrics::RecordConsistencyPromoUserAction(
      signin_metrics::AccountConsistencyPromoAction::SHOWN, self.accessPoint);
  // TODO(crbug.com/40071752): Stop hardcoding "non-default identity" here. The
  // user might still choose the default one, or a new one, those map to
  // different actions. Instead, plumb the correct value to didSigninWithResult.
  _actionToRecordOnSuccess = signin_metrics::AccountConsistencyPromoAction::
      SIGNED_IN_WITH_NON_DEFAULT_ACCOUNT;
  _identityChooserCoordinator = [[IdentityChooserCoordinator alloc]
      initWithBaseViewController:self.baseViewController
                         browser:self.browser];
  _identityChooserCoordinator.delegate = self;
  [_identityChooserCoordinator start];
}

- (void)stop {
  CHECK(!_addAccountSigninCoordinator);
  CHECK(!_activityOverlayCoordinator);
  CHECK(!_identityChooserCoordinator);
  [_mediator disconnect];
  _mediator.delegate = nil;
  _mediator = nil;
  [super stop];
}

#pragma mark - SigninCoordinator

- (void)interruptWithAction:(SigninCoordinatorInterrupt)action
                 completion:(ProceduralBlock)completion {
  if (_addAccountSigninCoordinator) {
    CHECK(!_identityChooserCoordinator);
    CHECK(!_activityOverlayCoordinator);
    [_addAccountSigninCoordinator interruptWithAction:action
                                           completion:completion];
  } else if (_identityChooserCoordinator) {
    CHECK(!_activityOverlayCoordinator);
    [self stopIdentityChooserCoordinator];
    [self runCompletionWithSigninResult:SigninCoordinatorResultInterrupted
                     completionIdentity:nil];
    if (completion) {
      completion();
    }
  } else if (action == SigninCoordinatorInterrupt::UIShutdownNoDismiss) {
    CHECK(!IsInterruptibleCoordinatorAlwaysDismissedEnabled(),
          base::NotFatalUntil::M136);
    // In case of `UIShutdownNoDismiss`, everything should be done
    // synchronously. So we should not wait for the mediator interruption to be
    // done. The coordinator needs to finish itself, and then call the interrupt
    // completion.
    _mediator.delegate = nil;
    [_mediator interruptWithAction:action completion:nil];
    // Drop the activity overlay if it exists.
    [self stopActivityOverlay];
    [self runCompletionWithSigninResult:SigninCoordinatorResultInterrupted
                     completionIdentity:nil];
    if (completion) {
      completion();
    }
  } else {
    if (IsInterruptibleCoordinatorStoppedSynchronouslyEnabled()) {
      [_mediator interruptWithAction:action completion:nil];
      if (completion) {
        completion();
      }
    } else {
      [_mediator interruptWithAction:action completion:completion];
    }
  }
}

#pragma mark - AuthenticationFlowDelegate

- (void)didPresentDialog {
  [self removeActivityOverlay];
}

- (void)didDismissDialog {
  [self showActivityOverlay];
}

#pragma mark - IdentityChooserCoordinatorDelegate

- (void)identityChooserCoordinatorDidClose:
    (IdentityChooserCoordinator*)coordinator {
  // `_identityChooserCoordinator.delegate` was set to nil before calling this
  // method since `identityChooserCoordinatorDidTapOnAddAccount:` or
  // `identityChooserCoordinator:didSelectIdentity:` have been called before.
  NOTREACHED() << base::SysNSStringToUTF8([self description]);
}

- (void)identityChooserCoordinatorDidTapOnAddAccount:
    (IdentityChooserCoordinator*)coordinator {
  CHECK_EQ(coordinator, _identityChooserCoordinator)
      << base::SysNSStringToUTF8([self description]);
  _identityChooserCoordinator.delegate = nil;
  [self stopIdentityChooserCoordinator];
  [self startAddAccountForSignInOnly];
}

- (void)identityChooserCoordinator:(IdentityChooserCoordinator*)coordinator
                 didSelectIdentity:(id<SystemIdentity>)identity {
  CHECK_EQ(coordinator, _identityChooserCoordinator)
      << base::SysNSStringToUTF8([self description]);
  _identityChooserCoordinator.delegate = nil;
  [self stopIdentityChooserCoordinator];
  if (!identity) {
    // If no identity was selected, the coordinator can be closed.
    [self runCompletionWithSigninResult:SigninCoordinatorResultCanceledByUser
                     completionIdentity:nil];
    return;
  }
  _identity = identity;
  // The identity is now selected, the sign-in flow can be started.
  [self startSignInOnlyFlow];
}

#pragma mark - InstantSigninMediatorDelegate

- (void)instantSigninMediator:(InstantSigninMediator*)mediator
          didSigninWithResult:(SigninCoordinatorResult)result {
  [self removeActivityOverlay];
  switch (result) {
    case SigninCoordinatorResultSuccess: {
      signin_metrics::RecordConsistencyPromoUserAction(_actionToRecordOnSuccess,
                                                       self.accessPoint);
      [self runCompletionWithSigninResult:SigninCoordinatorResultSuccess
                       completionIdentity:_identity];
      break;
    }
    case SigninCoordinatorResultDisabled:
    case SigninCoordinatorResultInterrupted:
    case SigninCoordinatorResultCanceledByUser:
      [self runCompletionWithSigninResult:result completionIdentity:nil];
      break;
    case SigninCoordinatorUINotAvailable:
      NOTREACHED();
  }
}

#pragma mark - Private

// Starts the sign-in flow.
- (void)startSignInOnlyFlow {
  // TODO(crbug.com/375605482): Handle the case where the chosen identity is
  // assigned to a different profile.
  [self showActivityOverlay];
  signin_metrics::RecordSigninUserActionForAccessPoint(self.accessPoint);
  // If this was triggered by the user tapping the default button in the sign-in
  // promo, give the user a chance to see the full email, by showing a snackbar.
  auto postSigninActions =
      _promoAction == signin_metrics::PromoAction::PROMO_ACTION_WITH_DEFAULT
          ? PostSignInActionSet({PostSignInAction::kShowSnackbar})
          : PostSignInActionSet({PostSignInAction::kNone});
  if (self.accessPoint ==
      signin_metrics::AccessPoint::ACCESS_POINT_BOOKMARK_MANAGER) {
    postSigninActions.Put(PostSignInAction::kEnableUserSelectableTypeBookmarks);
  } else if (self.accessPoint ==
             signin_metrics::AccessPoint::ACCESS_POINT_READING_LIST) {
    postSigninActions.Put(
        PostSignInAction::kEnableUserSelectableTypeReadingList);
  }
  AuthenticationFlow* authenticationFlow =
      [[AuthenticationFlow alloc] initWithBrowser:self.browser
                                         identity:_identity
                                      accessPoint:self.accessPoint
                                postSignInActions:postSigninActions
                         presentingViewController:self.baseViewController];
  authenticationFlow.delegate = self;
  authenticationFlow.precedingHistorySync = YES;
  [_mediator startSignInOnlyFlowWithAuthenticationFlow:authenticationFlow];
}

// Starts the add account coordinator.
- (void)startAddAccountForSignInOnly {
  _addAccountSigninCoordinator = [SigninCoordinator
      addAccountCoordinatorWithBaseViewController:self.baseViewController
                                          browser:self.browser
                                      accessPoint:self.accessPoint];
  __weak __typeof(self) weakSelf = self;
  _addAccountSigninCoordinator.signinCompletion = ^(
      SigninCoordinatorResult result, id<SystemIdentity> resultIdentity) {
    [weakSelf addAccountDoneWithResult:result resultIdentity:resultIdentity];
  };
  [_addAccountSigninCoordinator start];
}

// Starts the sign-in flow if the identity has been selected, otherwise, it
// ends this coordinator.
- (void)addAccountDoneWithResult:(SigninCoordinatorResult)result
                  resultIdentity:(id<SystemIdentity>)resultIdentity {
  CHECK(_addAccountSigninCoordinator)
      << base::SysNSStringToUTF8([self description]);
  [self stopAddAccountSigninCoordinator];
  switch (result) {
    case SigninCoordinatorResultSuccess:
      _identity = resultIdentity;
      [self startSignInOnlyFlow];
      break;
    case SigninCoordinatorResultDisabled:
    case SigninCoordinatorResultInterrupted:
    case SigninCoordinatorResultCanceledByUser:
      [self runCompletionWithSigninResult:result completionIdentity:nil];
      break;
    case SigninCoordinatorUINotAvailable:
      // InstantSigninCoordinator presents its child coordinators directly and
      // does not use `ShowSigninCommand`.
      NOTREACHED();
  }
}

// Adds an activity overlay to block the UI.
- (void)showActivityOverlay {
  // TODO(crbug.com/40067451): Switch back to DCHECK if the number of reports is
  // low.
  DUMP_WILL_BE_CHECK(!_activityOverlayCoordinator);
  _activityOverlayCoordinator = [[ActivityOverlayCoordinator alloc]
      initWithBaseViewController:self.baseViewController
                         browser:self.browser];
  [_activityOverlayCoordinator start];
}

// Removes an activity overlay to block the UI. `-[HistorySyncCoordinator
// showActivityOverlay]` must have been called before. The activity must exists.
- (void)removeActivityOverlay {
  CHECK(_activityOverlayCoordinator);
  [self stopActivityOverlay];
}

- (void)stopActivityOverlay {
  [_activityOverlayCoordinator stop];
  _activityOverlayCoordinator = nil;
}

- (void)stopIdentityChooserCoordinator {
  [_identityChooserCoordinator stop];
  _identityChooserCoordinator = nil;
}

- (void)stopAddAccountSigninCoordinator {
  [_addAccountSigninCoordinator stop];
  _addAccountSigninCoordinator = nil;
}

#pragma mark - NSObject

- (NSString*)description {
  return [NSString
      stringWithFormat:
          @"<%@: %p, identity: %p, accessPoint: %d, mediator %p, "
          @"identityChooserCoordinator %p, addAccountSigninCoordinator %p, "
          @"baseViewController: %@, window: %p>",
          self.class.description, self, _identity,
          static_cast<int>(self.accessPoint), _mediator,
          _identityChooserCoordinator, _addAccountSigninCoordinator,
          NSStringFromClass([self.baseViewController class]),
          self.baseViewController.view.window];
}

@end
