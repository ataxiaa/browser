// Copyright 2019 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef IOS_CHROME_BROWSER_QR_SCANNER_UI_BUNDLED_QR_SCANNER_CAMERA_CONTROLLER_DELEGATE_H_
#define IOS_CHROME_BROWSER_QR_SCANNER_UI_BUNDLED_QR_SCANNER_CAMERA_CONTROLLER_DELEGATE_H_

#import "ios/chrome/browser/scanner/ui_bundled/camera_controller.h"

// Receives the QR scanner results.
@protocol QRScannerCameraControllerDelegate <CameraControllerDelegate>

// Called when the scanner detects a valid code. The camera controller stops
// recording when a result is scanned. A valid code is any non-empty string. If
// `load` is YES, the result should be loaded immediately without requiring
// additional user input. The value of `load` will only be YES for barcodes
// which can only encode digits.
- (void)receiveQRScannerResult:(NSString*)result loadImmediately:(BOOL)load;

@end

#endif  // IOS_CHROME_BROWSER_QR_SCANNER_UI_BUNDLED_QR_SCANNER_CAMERA_CONTROLLER_DELEGATE_H_
