// Copyright 2014 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.url;

import org.jni_zero.CalledByNative;
import org.jni_zero.JNINamespace;

import org.chromium.build.annotations.NullMarked;
import org.chromium.build.annotations.Nullable;

import java.net.IDN;

/** This class is used to convert unicode IDN domain names to ASCII, when not building with ICU. */
@JNINamespace("url::android")
@NullMarked
public class IDNStringUtil {
    /**
     * Attempts to convert a Unicode string to an ASCII string using IDN rules. As of May 2014, the
     * underlying Java function IDNA2003.
     *
     * @param src String to convert.
     * @return String containing only ASCII characters on success, null on failure.
     */
    @CalledByNative
    private static @Nullable String idnToASCII(String src) {
        try {
            return IDN.toASCII(src, IDN.USE_STD3_ASCII_RULES);
        } catch (Exception e) {
            return null;
        }
    }
}
