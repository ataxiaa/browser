// Copyright 2013 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ui/chrome_web_modal_dialog_manager_delegate.h"

#include "chrome/browser/platform_util.h"
#include "content/public/browser/web_contents.h"

ChromeWebModalDialogManagerDelegate::ChromeWebModalDialogManagerDelegate() =
    default;

ChromeWebModalDialogManagerDelegate::~ChromeWebModalDialogManagerDelegate() =
    default;

bool ChromeWebModalDialogManagerDelegate::IsWebContentsVisible(
    content::WebContents* web_contents) {
  return platform_util::IsVisible(web_contents->GetNativeView());
}
