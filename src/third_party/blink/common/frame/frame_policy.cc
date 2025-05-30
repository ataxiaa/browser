// Copyright 2017 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/public/common/frame/frame_policy.h"
#include "services/network/public/mojom/web_sandbox_flags.mojom-shared.h"

namespace blink {

FramePolicy::FramePolicy()
    : sandbox_flags(network::mojom::WebSandboxFlags::kNone),
      container_policy({}),
      required_document_policy({}),
      deferred_fetch_policy(DeferredFetchPolicy::kDisabled) {}

FramePolicy::FramePolicy(
    network::mojom::WebSandboxFlags sandbox_flags,
    const ParsedPermissionsPolicy& container_policy,
    const DocumentPolicyFeatureState& required_document_policy,
    DeferredFetchPolicy deferred_fetch_policy)
    : sandbox_flags(sandbox_flags),
      container_policy(container_policy),
      required_document_policy(required_document_policy),
      deferred_fetch_policy(deferred_fetch_policy) {}

FramePolicy::FramePolicy(const FramePolicy& lhs) = default;

FramePolicy::~FramePolicy() = default;

bool operator==(const FramePolicy& lhs, const FramePolicy& rhs) = default;

bool operator!=(const FramePolicy& lhs, const FramePolicy& rhs) {
  return !(lhs == rhs);
}

}  // namespace blink
