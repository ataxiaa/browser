// Copyright 2022 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_AUTOFILL_CORE_BROWSER_FORM_PARSING_IBAN_FIELD_PARSER_H_
#define COMPONENTS_AUTOFILL_CORE_BROWSER_FORM_PARSING_IBAN_FIELD_PARSER_H_

#include <memory>

#include "base/memory/raw_ptr.h"
#include "components/autofill/core/browser/country_type.h"
#include "components/autofill/core/browser/form_parsing/form_field_parser.h"
#include "components/autofill/core/common/language_code.h"

namespace autofill {

class AutofillScanner;

// A form field that accepts International Bank Account Number (IBAN).
class IbanFieldParser : public FormFieldParser {
 public:
  static std::unique_ptr<FormFieldParser> Parse(ParsingContext& context,
                                                AutofillScanner* scanner);

  explicit IbanFieldParser(FieldAndMatchInfo match);

  IbanFieldParser(const IbanFieldParser&) = delete;
  IbanFieldParser& operator=(const IbanFieldParser&) = delete;

 protected:
  void AddClassifications(FieldCandidatesMap& field_candidates) const override;

 private:
  FieldAndMatchInfo match_;
};

}  // namespace autofill

#endif  // COMPONENTS_AUTOFILL_CORE_BROWSER_FORM_PARSING_IBAN_FIELD_PARSER_H_
