// Copyright 2020 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/no_state_prefetch/renderer/no_state_prefetch_render_frame_observer.h"

#include "components/no_state_prefetch/renderer/no_state_prefetch_helper.h"
#include "components/no_state_prefetch/renderer/no_state_prefetch_observer_list.h"
#include "content/public/renderer/render_frame.h"
#include "third_party/blink/public/common/associated_interfaces/associated_interface_registry.h"

namespace prerender {

NoStatePrefetchRenderFrameObserver::NoStatePrefetchRenderFrameObserver(
    content::RenderFrame* render_frame)
    : content::RenderFrameObserver(render_frame) {
  render_frame->GetAssociatedInterfaceRegistry()
      ->AddInterface<prerender::mojom::NoStatePrefetchMessages>(
          base::BindRepeating(
              &NoStatePrefetchRenderFrameObserver::OnRenderFrameObserverRequest,
              base::Unretained(this)));
}

NoStatePrefetchRenderFrameObserver::~NoStatePrefetchRenderFrameObserver() =
    default;

void NoStatePrefetchRenderFrameObserver::OnRenderFrameObserverRequest(
    mojo::PendingAssociatedReceiver<prerender::mojom::NoStatePrefetchMessages>
        receiver) {
  receivers_.Add(this, std::move(receiver));
}

void NoStatePrefetchRenderFrameObserver::OnDestruct() {
  delete this;
}

void NoStatePrefetchRenderFrameObserver::SetIsNoStatePrefetching(
    const std::string& histogram_prefix) {
  // If the NoStatePrefetchHelper for this frame already exists, don't create
  // it. It can already be created for subframes during handling of
  // RenderFrameCreated, if the parent frame was prefetching at time of
  // subframe creation.
  if (!prerender::NoStatePrefetchHelper::Get(render_frame())) {
    // The NoStatePrefetchHelper will destroy itself either after recording
    // histograms or on destruction of the RenderView.
    new prerender::NoStatePrefetchHelper(render_frame(), histogram_prefix);
  }

  prerender::NoStatePrefetchObserverList::SetIsNoStatePrefetchingForFrame(
      render_frame(), /*is_no_state_prefetching=*/true);
}

}  // namespace prerender
