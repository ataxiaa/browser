// Copyright 2015 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "third_party/blink/renderer/core/dom/attr.h"

#include "testing/gmock/include/gmock/gmock.h"
#include "testing/gtest/include/gtest/gtest.h"
#include "third_party/blink/renderer/core/dom/document.h"
#include "third_party/blink/renderer/core/testing/null_execution_context.h"
#include "third_party/blink/renderer/platform/heap/garbage_collected.h"
#include "third_party/blink/renderer/platform/testing/task_environment.h"

namespace blink {

class AttrTest : public testing::Test {
 protected:
  void SetUp() override;

  Attr* CreateAttribute();
  const AtomicString& Value() const { return value_; }

 private:
  test::TaskEnvironment task_environment_;
  ScopedNullExecutionContext execution_context_;
  Persistent<Document> document_;
  AtomicString value_;
};

void AttrTest::SetUp() {
  document_ = Document::CreateForTest(execution_context_.GetExecutionContext());
  value_ = AtomicString("value");
}

Attr* AttrTest::CreateAttribute() {
  return document_->createAttribute(AtomicString("name"), ASSERT_NO_EXCEPTION);
}

TEST_F(AttrTest, InitialValueState) {
  Attr* attr = CreateAttribute();
  Node* node = attr;
  EXPECT_EQ(g_empty_atom, attr->value());
  EXPECT_EQ(g_empty_string, node->nodeValue());
  EXPECT_EQ(g_empty_string, attr->textContent());
}

TEST_F(AttrTest, SetValue) {
  Attr* attr = CreateAttribute();
  Node* node = attr;
  attr->setValue(Value(), ASSERT_NO_EXCEPTION);
  EXPECT_EQ(Value(), attr->value());
  EXPECT_EQ(Value(), node->nodeValue());
  EXPECT_EQ(Value(), attr->textContent());
}

TEST_F(AttrTest, SetNodeValue) {
  Attr* attr = CreateAttribute();
  Node* node = attr;
  node->setNodeValue(Value());
  EXPECT_EQ(Value(), attr->value());
  EXPECT_EQ(Value(), node->nodeValue());
  EXPECT_EQ(Value(), attr->textContent());
}

TEST_F(AttrTest, SetTextContent) {
  Attr* attr = CreateAttribute();
  Node* node = attr;
  attr->setTextContent(Value());
  EXPECT_EQ(Value(), attr->value());
  EXPECT_EQ(Value(), node->nodeValue());
  EXPECT_EQ(Value(), attr->textContent());
}

}  // namespace blink
