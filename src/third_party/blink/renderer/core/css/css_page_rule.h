/*
 * (C) 1999-2003 Lars Knoll (knoll@kde.org)
 * (C) 2002-2003 Dirk Mueller (mueller@kde.org)
 * Copyright (C) 2002, 2006, 2008, 2012 Apple Inc. All rights reserved.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Library General Public
 * License as published by the Free Software Foundation; either
 * version 2 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Library General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public License
 * along with this library; see the file COPYING.LIB.  If not, write to
 * the Free Software Foundation, Inc., 51 Franklin Street, Fifth Floor,
 * Boston, MA 02110-1301, USA.
 */

#ifndef THIRD_PARTY_BLINK_RENDERER_CORE_CSS_CSS_PAGE_RULE_H_
#define THIRD_PARTY_BLINK_RENDERER_CORE_CSS_CSS_PAGE_RULE_H_

#include "third_party/blink/renderer/core/core_export.h"
#include "third_party/blink/renderer/core/css/css_grouping_rule.h"
#include "third_party/blink/renderer/core/css/css_rule.h"
#include "third_party/blink/renderer/platform/heap/garbage_collected.h"
#include "third_party/blink/renderer/platform/heap/member.h"
#include "third_party/blink/renderer/platform/wtf/casting.h"

namespace blink {

class CSSStyleDeclaration;
class CSSStyleSheet;
class ExecutionContext;
class StyleRulePage;
class StyleRuleCSSStyleDeclaration;

class CORE_EXPORT CSSPageRule final : public CSSGroupingRule {
  DEFINE_WRAPPERTYPEINFO();

 public:
  CSSPageRule(StyleRulePage*, CSSStyleSheet*);
  ~CSSPageRule() override;

  String cssText() const override;
  void Reattach(StyleRuleBase*) override;

  CSSStyleDeclaration* style() const;

  String selectorText() const;
  void setSelectorText(const ExecutionContext*, const String&);

  StyleRulePage* GetStyleRulePage() const { return page_rule_.Get(); }

  void Trace(Visitor*) const override;

 private:
  CSSRule::Type GetType() const override { return kPageRule; }

  Member<StyleRulePage> page_rule_;
  mutable Member<StyleRuleCSSStyleDeclaration> properties_cssom_wrapper_;
};

template <>
struct DowncastTraits<CSSPageRule> {
  static bool AllowFrom(const CSSRule& rule) {
    return rule.GetType() == CSSRule::kPageRule;
  }
};

}  // namespace blink

#endif  // THIRD_PARTY_BLINK_RENDERER_CORE_CSS_CSS_PAGE_RULE_H_
