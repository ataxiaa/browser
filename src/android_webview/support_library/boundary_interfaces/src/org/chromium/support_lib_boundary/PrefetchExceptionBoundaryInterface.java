// Copyright 2024 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

package org.chromium.support_lib_boundary;

public interface PrefetchExceptionBoundaryInterface {

    String getMessage();

    Throwable getCause();
}
