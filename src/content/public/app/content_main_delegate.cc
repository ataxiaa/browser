// Copyright 2012 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/public/app/content_main_delegate.h"

#include "base/check.h"
#include "base/notreached.h"
#include "build/build_config.h"
#include "content/public/browser/content_browser_client.h"
#include "content/public/common/content_client.h"
#include "content/public/gpu/content_gpu_client.h"
#include "content/public/renderer/content_renderer_client.h"
#include "content/public/utility/content_utility_client.h"

namespace content {

std::optional<int> ContentMainDelegate::BasicStartupComplete() {
  return std::nullopt;
}

absl::variant<int, MainFunctionParams> ContentMainDelegate::RunProcess(
    const std::string& process_type,
    MainFunctionParams main_function_params) {
  return std::move(main_function_params);
}

#if BUILDFLAG(IS_LINUX) || BUILDFLAG(IS_CHROMEOS)

void ContentMainDelegate::ZygoteStarting(
    std::vector<std::unique_ptr<ZygoteForkDelegate>>* delegates) {}

#endif  // BUILDFLAG(IS_LINUX) || BUILDFLAG(IS_CHROMEOS)

int ContentMainDelegate::TerminateForFatalInitializationError() {
  NOTREACHED();
}

#if BUILDFLAG(IS_WIN)
bool ContentMainDelegate::ShouldHandleConsoleControlEvents() {
  return false;
}
#endif

bool ContentMainDelegate::ShouldLockSchemeRegistry() {
  return true;
}

std::optional<int> ContentMainDelegate::PreBrowserMain() {
  return std::nullopt;
}

bool ContentMainDelegate::ShouldCreateFeatureList(InvokedIn invoked_in) {
  return true;
}

bool ContentMainDelegate::ShouldInitializeMojo(InvokedIn invoked_in) {
  return true;
}

variations::VariationsIdsProvider*
ContentMainDelegate::CreateVariationsIdsProvider() {
  return nullptr;
}

void ContentMainDelegate::CreateThreadPool(std::string_view name) {
  base::ThreadPoolInstance::Create(name);
}

std::optional<int> ContentMainDelegate::PostEarlyInitialization(
    InvokedIn invoked_in) {
  return std::nullopt;
}

ContentClient* ContentMainDelegate::CreateContentClient() {
  return new ContentClient();
}

ContentBrowserClient* ContentMainDelegate::CreateContentBrowserClient() {
  return new ContentBrowserClient();
}

ContentGpuClient* ContentMainDelegate::CreateContentGpuClient() {
  return new ContentGpuClient();
}

ContentRendererClient* ContentMainDelegate::CreateContentRendererClient() {
  return new ContentRendererClient();
}

ContentUtilityClient* ContentMainDelegate::CreateContentUtilityClient() {
  return new ContentUtilityClient();
}

}  // namespace content
