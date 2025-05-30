// Copyright 2024 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/autofill/core/browser/metrics/refill_metrics.h"

#include "base/metrics/histogram_functions.h"

namespace autofill::autofill_metrics {

void LogRefillTriggerReason(RefillTriggerReason refill_trigger_reason) {
  base::UmaHistogramEnumeration("Autofill.RefillTriggerReason",
                                refill_trigger_reason);
}

}  // namespace autofill::autofill_metrics
