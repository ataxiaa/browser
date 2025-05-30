// Copyright 2020 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.device;

import org.jni_zero.JNINamespace;

import org.chromium.build.annotations.NullMarked;

/**
 * Lists //services/device features that can be accessed through {@link DeviceFeatureMap}.
 *
 * <p>Note: Features must be added to the array |kFeaturesExposedToJava| in
 * //services/device/public/cpp/device_feature_map.cc.
 */
@JNINamespace("features")
@NullMarked
public abstract class DeviceFeatureList {
    public static final String GENERIC_SENSOR_EXTRA_CLASSES = "GenericSensorExtraClasses";
    public static final String WEBAUTHN_ANDROID_CRED_MAN = "WebAuthenticationAndroidCredMan";
    public static final String WEBAUTHN_ANDROID_USE_PASSKEY_CACHE =
            "WebAuthenticationAndroidUsePasskeyCache";
    public static final String BATTERY_STATUS_MANAGER_BROADCAST_RECEIVER_IN_BACKGROUND =
            "BatteryStatusManagerBroadcastReceiverInBackground";
    public static final String WEBAUTHN_ENABLE_PAASK_SETTING =
            "WebAuthenticationEnablePaaskFragment";
}
