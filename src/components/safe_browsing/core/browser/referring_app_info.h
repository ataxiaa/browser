// Copyright 2024 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_SAFE_BROWSING_CORE_BROWSER_REFERRING_APP_INFO_H_
#define COMPONENTS_SAFE_BROWSING_CORE_BROWSER_REFERRING_APP_INFO_H_

#include "components/safe_browsing/core/common/proto/csd.pb.h"
#include "url/gurl.h"

namespace safe_browsing {
namespace internal {
struct ReferringAppInfo {
  safe_browsing::ReferringAppInfo::ReferringAppSource referring_app_source;
  std::string referring_app_name;
  GURL target_url;
};
}  // namespace internal
}  // namespace safe_browsing

#endif  // COMPONENTS_SAFE_BROWSING_CORE_BROWSER_REFERRING_APP_INFO_H_
