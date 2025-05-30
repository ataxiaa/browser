// Copyright 2023 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#import "ios/chrome/browser/authentication/ui_bundled/signin/uno_signin_screen_provider.h"

#import "ios/chrome/browser/screen/ui_bundled/screen_provider+protected.h"
#import "ios/chrome/browser/screen/ui_bundled/screen_type.h"

@implementation UnoSigninScreenProvider

- (instancetype)init {
  NSArray<NSNumber*>* screens =
      @[ @(kSignIn), @(kHistorySync), @(kStepsCompleted) ];
  return [super initWithScreens:screens];
}

@end
