// Copyright 2024 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "chrome/browser/ai/ai_writer.h"

#include "base/functional/bind.h"
#include "base/strings/strcat.h"
#include "chrome/browser/ai/ai_context_bound_object.h"
#include "chrome/browser/ai/ai_utils.h"
#include "components/optimization_guide/core/optimization_guide_util.h"
#include "components/optimization_guide/proto/common_types.pb.h"
#include "components/optimization_guide/proto/features/compose.pb.h"
#include "third_party/blink/public/mojom/ai/model_streaming_responder.mojom.h"

AIWriter::AIWriter(
    AIContextBoundObjectSet& context_bound_object_set,
    std::unique_ptr<optimization_guide::OptimizationGuideModelExecutor::Session>
        session,
    blink::mojom::AIWriterCreateOptionsPtr options,
    mojo::PendingReceiver<blink::mojom::AIWriter> receiver)
    : AIContextBoundObject(context_bound_object_set),
      session_(std::move(session)),
      options_(std::move(options)),
      receiver_(this, std::move(receiver)) {
  receiver_.set_disconnect_handler(base::BindOnce(
      &AIContextBoundObject::RemoveFromSet, base::Unretained(this)));
}

AIWriter::~AIWriter() {
  for (auto& responder : responder_set_) {
    responder->OnError(
        blink::mojom::ModelStreamingResponseStatus::kErrorSessionDestroyed);
  }
}

void AIWriter::Write(const std::string& input,
                     const std::optional<std::string>& context,
                     mojo::PendingRemote<blink::mojom::ModelStreamingResponder>
                         pending_responder) {
  optimization_guide::proto::ComposePageMetadata page_metadata;
  std::string context_string = base::JoinString(
      {options_->shared_context.value_or(""), context.value_or("")}, "\n");
  base::TrimString(context_string, "\n", &context_string);
  page_metadata.set_trimmed_page_inner_text(
      context_string.substr(0, AIUtils::kTrimmedInnerTextMaxChars));
  page_metadata.set_page_inner_text(context_string);

  optimization_guide::proto::ComposeRequest context_request;
  *context_request.mutable_page_metadata() = std::move(page_metadata);

  session_->AddContext(context_request);

  optimization_guide::proto::ComposeRequest execute_request;
  execute_request.mutable_generate_params()->set_user_input(input);

  session_->ExecuteModel(
      execute_request,
      base::BindRepeating(&AIWriter::ModelExecutionCallback,
                          weak_ptr_factory_.GetWeakPtr(),
                          responder_set_.Add(std::move(pending_responder))));
}

void AIWriter::ModelExecutionCallback(
    mojo::RemoteSetElementId responder_id,
    optimization_guide::OptimizationGuideModelStreamingExecutionResult result) {
  blink::mojom::ModelStreamingResponder* responder =
      responder_set_.Get(responder_id);
  if (!responder) {
    return;
  }
  if (!result.response.has_value()) {
    responder->OnError(
        AIUtils::ConvertModelExecutionError(result.response.error().error()));
    return;
  }

  auto compose_response = optimization_guide::ParsedAnyMetadata<
      optimization_guide::proto::ComposeResponse>(result.response->response);
  if (compose_response) {
    responder->OnStreaming(compose_response->output());
  }
  if (result.response->is_complete) {
    responder->OnCompletion(/*context_info=*/nullptr);
    responder_set_.Remove(responder_id);
  }
}
