// Copyright 2023 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_PERFORMANCE_MANAGER_EXECUTION_CONTEXT_PRIORITY_FRAME_CAPTURING_MEDIA_STREAM_VOTER_H_
#define COMPONENTS_PERFORMANCE_MANAGER_EXECUTION_CONTEXT_PRIORITY_FRAME_CAPTURING_MEDIA_STREAM_VOTER_H_

#include "components/performance_manager/public/execution_context_priority/execution_context_priority.h"
#include "components/performance_manager/public/execution_context_priority/priority_voting_system.h"
#include "components/performance_manager/public/graph/frame_node.h"

namespace performance_manager::execution_context_priority {

// This voter casts a TaskPriority::USER_BLOCKING vote to all frames that are
// capturing a media stream (audio or video), and a TaskPriority::LOWEST vote
// otherwise.
// Note: This FrameNodeObserver can affect the initial priority of a frame and
// thus uses `OnBeforeFrameNodeAdded`.
class FrameCapturingMediaStreamVoter : public PriorityVoter,
                                       public FrameNodeObserver {
 public:
  static const char kFrameCapturingMediaStreamReason[];

  FrameCapturingMediaStreamVoter();
  ~FrameCapturingMediaStreamVoter() override;

  FrameCapturingMediaStreamVoter(const FrameCapturingMediaStreamVoter&) =
      delete;
  FrameCapturingMediaStreamVoter& operator=(
      const FrameCapturingMediaStreamVoter&) = delete;

  // PriorityVoter:
  void InitializeOnGraph(Graph* graph, VotingChannel voting_channel) override;
  void TearDownOnGraph(Graph* graph) override;

  // FrameNodeObserver:
  void OnBeforeFrameNodeAdded(
      const FrameNode* frame_node,
      const FrameNode* pending_parent_frame_node,
      const PageNode* pending_page_node,
      const ProcessNode* pending_process_node,
      const FrameNode* pending_parent_or_outer_document_or_embedder) override;
  void OnBeforeFrameNodeRemoved(const FrameNode* frame_node) override;
  void OnIsCapturingMediaStreamChanged(const FrameNode* frame_node) override;

  VoterId voter_id() const { return voting_channel_.voter_id(); }

 private:
  VotingChannel voting_channel_;
};

}  // namespace performance_manager::execution_context_priority

#endif  // COMPONENTS_PERFORMANCE_MANAGER_EXECUTION_CONTEXT_PRIORITY_FRAME_CAPTURING_MEDIA_STREAM_VOTER_H_
