// Copyright 2023 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "services/network/shared_storage/shared_storage_test_utils.h"

#include <optional>
#include <string>
#include <utility>
#include <vector>

#include "base/strings/strcat.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "net/test/embedded_test_server/embedded_test_server.h"
#include "services/network/shared_storage/shared_storage_header_utils.h"
#include "url/url_util.h"

namespace network {

// static
size_t SharedStorageRequestCount::Get() {
  return count_;
}

// static
size_t SharedStorageRequestCount::Increment() {
  return count_++;
}

// static
void SharedStorageRequestCount::Reset() {
  count_ = 0;
}

// static
size_t SharedStorageRequestCount::count_ = 0;

std::string MakeSharedStorageTestPath() {
  return base::StrCat({kSharedStoragePathPrefix, kSharedStorageTestPath,
                       kSharedStorageWritePathSuffix});
}

std::string MakeSharedStorageBypassPath() {
  return base::StrCat({kSharedStoragePathPrefix, kSharedStorageBypassPath,
                       kSharedStorageWritePathSuffix});
}

std::string MakeSharedStorageRedirectPrefix() {
  return base::StrCat({kSharedStoragePathPrefix, kSharedStorageRedirectPath});
}

mojom::SharedStorageModifierMethodWithOptionsPtr MojomSetMethod(
    const std::u16string& key,
    const std::u16string& value,
    bool ignore_if_present,
    std::optional<std::string> with_lock) {
  auto method = mojom::SharedStorageModifierMethod::NewSetMethod(
      mojom::SharedStorageSetMethod::New(key, value, ignore_if_present));

  return mojom::SharedStorageModifierMethodWithOptions::New(
      std::move(method), std::move(with_lock));
}

mojom::SharedStorageModifierMethodWithOptionsPtr MojomAppendMethod(
    const std::u16string& key,
    const std::u16string& value,
    std::optional<std::string> with_lock) {
  auto method = mojom::SharedStorageModifierMethod::NewAppendMethod(
      mojom::SharedStorageAppendMethod::New(key, value));

  return mojom::SharedStorageModifierMethodWithOptions::New(
      std::move(method), std::move(with_lock));
}

mojom::SharedStorageModifierMethodWithOptionsPtr MojomDeleteMethod(
    const std::u16string& key,
    std::optional<std::string> with_lock) {
  auto method = mojom::SharedStorageModifierMethod::NewDeleteMethod(
      mojom::SharedStorageDeleteMethod::New(key));

  return mojom::SharedStorageModifierMethodWithOptions::New(
      std::move(method), std::move(with_lock));
}

mojom::SharedStorageModifierMethodWithOptionsPtr MojomClearMethod(
    std::optional<std::string> with_lock) {
  auto method = mojom::SharedStorageModifierMethod::NewClearMethod(
      mojom::SharedStorageClearMethod::New());

  return mojom::SharedStorageModifierMethodWithOptions::New(
      std::move(method), std::move(with_lock));
}

SharedStorageMethodWrapper::SharedStorageMethodWrapper(
    mojom::SharedStorageModifierMethodWithOptionsPtr method_with_options)
    : method_with_options(std::move(method_with_options)) {}

SharedStorageMethodWrapper::SharedStorageMethodWrapper(
    const SharedStorageMethodWrapper& other)
    : method_with_options(other.method_with_options.Clone()) {}

SharedStorageMethodWrapper& SharedStorageMethodWrapper::operator=(
    const SharedStorageMethodWrapper& other) {
  if (this != &other) {
    method_with_options = other.method_with_options.Clone();
  }
  return *this;
}

SharedStorageMethodWrapper::~SharedStorageMethodWrapper() = default;

std::ostream& operator<<(std::ostream& os,
                         const SharedStorageMethodWrapper& wrapper) {
  switch (wrapper.method_with_options->method->which()) {
    case mojom::SharedStorageModifierMethod::Tag::kSetMethod: {
      mojom::SharedStorageSetMethodPtr& set_method =
          wrapper.method_with_options->method->get_set_method();
      os << "Method: Set(" << set_method->key << "," << set_method->value << ","
         << (set_method->ignore_if_present ? "true" : "false") << ")";
      break;
    }
    case mojom::SharedStorageModifierMethod::Tag::kAppendMethod: {
      mojom::SharedStorageAppendMethodPtr& append_method =
          wrapper.method_with_options->method->get_append_method();
      os << "Method: Append(" << append_method->key << ","
         << append_method->value << ")";
      break;
    }
    case mojom::SharedStorageModifierMethod::Tag::kDeleteMethod: {
      mojom::SharedStorageDeleteMethodPtr& delete_method =
          wrapper.method_with_options->method->get_delete_method();
      os << "Method: Delete(" << delete_method->key << ")";
      break;
    }
    case mojom::SharedStorageModifierMethod::Tag::kClearMethod: {
      os << "Method: Clear()";
      break;
    }
  }

  const std::optional<std::string>& with_lock =
      wrapper.method_with_options->with_lock;
  if (with_lock) {
    os << "; WithLock: " << with_lock.value();
  }

  return os;
}

SharedStorageResponse::SharedStorageResponse(std::string shared_storage_write)
    : shared_storage_write_(std::move(shared_storage_write)) {}

SharedStorageResponse::SharedStorageResponse(net::HttpStatusCode code,
                                             std::string new_location)
    : code_(code), new_location_(std::move(new_location)) {}

SharedStorageResponse::SharedStorageResponse(std::string shared_storage_write,
                                             net::HttpStatusCode code,
                                             std::string new_location)
    : shared_storage_write_(std::move(shared_storage_write)),
      code_(code),
      new_location_(std::move(new_location)) {}

SharedStorageResponse::~SharedStorageResponse() = default;

void SharedStorageResponse::SendResponse(
    base::WeakPtr<net::test_server::HttpResponseDelegate> delegate) {
  if (shared_storage_write_.has_value()) {
    AddCustomHeader(kSharedStorageWriteHeader, shared_storage_write_.value());
  }
  if (new_location_.has_value()) {
    AddCustomHeader("Location", base::JoinString({kSharedStoragePathPrefix,
                                                  new_location_.value()},
                                                 "/"));
  }
  set_code(code_);
  set_content_type("text/plain");
  set_content(kSharedStorageResponseData);
  delegate->SendResponseHeaders(code(), GetHttpReasonPhrase(code()),
                                BuildHeaders());
  delegate->SendContents(content(), base::DoNothing());
}

std::unique_ptr<net::test_server::HttpResponse>
HandleSharedStorageRequestSimple(std::string shared_storage_write,
                                 const net::test_server::HttpRequest& request) {
  const auto& path = request.GetURL().path();
  if (!base::StartsWith(path, kSharedStoragePathPrefix)) {
    return nullptr;
  }

  auto it = request.headers.find(kSecSharedStorageWritableHeader);
  if (path == MakeSharedStorageBypassPath() ||
      (it != request.headers.end() &&
       it->second == kSecSharedStorageWritableValue)) {
    return std::make_unique<SharedStorageResponse>(
        std::move(shared_storage_write));
  }
  return std::make_unique<net::test_server::BasicHttpResponse>();
}

std::unique_ptr<net::test_server::HttpResponse>
HandleSharedStorageRequestMultiple(
    std::vector<std::string> shared_storage_write_headers,
    const net::test_server::HttpRequest& request) {
  const auto& path = request.GetURL().path();
  if (!base::StartsWith(path, kSharedStoragePathPrefix)) {
    return nullptr;
  }

  std::optional<std::string> write_header;
  auto it = request.headers.find(kSecSharedStorageWritableHeader);
  if ((base::EndsWith(path, kSharedStorageWritePathSuffix) &&
       it != request.headers.end() &&
       it->second == kSecSharedStorageWritableValue) &&
      SharedStorageRequestCount::Increment() <=
          shared_storage_write_headers.size()) {
    write_header = std::move(
        shared_storage_write_headers[SharedStorageRequestCount::Get() - 1]);
  }
  std::optional<std::string> location;
  const std::string& query = request.GetURL().query();
  if (base::StartsWith(path, MakeSharedStorageRedirectPrefix()) &&
      !query.empty()) {
    url::RawCanonOutputT<char16_t> decode_output;
    url::DecodeURLEscapeSequences(query, url::DecodeURLMode::kUTF8,
                                  &decode_output);
    location = base::UTF16ToUTF8(decode_output.view());
  }
  if (write_header.has_value()) {
    return location.has_value()
               ? std::make_unique<SharedStorageResponse>(
                     std::move(write_header.value()),
                     net::HTTP_TEMPORARY_REDIRECT, std::move(location.value()))
               : std::make_unique<SharedStorageResponse>(
                     std::move(write_header.value()));
  }
  return location.has_value()
             ? std::make_unique<SharedStorageResponse>(
                   net::HTTP_TEMPORARY_REDIRECT, std::move(location.value()))
             : std::make_unique<net::test_server::BasicHttpResponse>();
}

}  // namespace network
