// Copyright 2024 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_AUTOFILL_CORE_BROWSER_FILLING_FIELD_FILLING_SKIP_REASON_H_
#define COMPONENTS_AUTOFILL_CORE_BROWSER_FILLING_FIELD_FILLING_SKIP_REASON_H_

#include <cstdint>
#include <string_view>

namespace autofill {

// Whether and why filling for a field was skipped during autofill.
enum class FieldFillingSkipReason : uint8_t {
  // Values are recorded as metrics and must not change or be reused.
  kUnknown = 0,
  kNotSkipped = 1,
  kNotInFilledSection = 2,
  kNotFocused = 3,
  kFormChanged = 4,
  kInvisibleField = 5,
  kValuePrefilled = 6,
  kUserFilledFields = 7,
  kAlreadyAutofilled = 8,
  // kNoFillableGroup = 9, // DEPRECATED
  kRefillNotInInitialFill = 10,
  // kExpiredCards = 11, // DEPRECATED
  kFillingLimitReachedType = 12,
  kUnrecognizedAutocompleteAttribute = 13,
  // kFieldDoesNotMatchTargetFieldsSet = 14, // DEPRECATED
  kFieldTypeUnrelated = 15,
  kNoValueToFill = 16,
  kAutofilledValueDidNotChange = 17,
  kMaxValue = kAutofilledValueDidNotChange
};

std::string_view GetSkipFieldFillLogMessage(FieldFillingSkipReason skip_reason);

}  // namespace autofill

#endif  // COMPONENTS_AUTOFILL_CORE_BROWSER_FILLING_FIELD_FILLING_SKIP_REASON_H_
