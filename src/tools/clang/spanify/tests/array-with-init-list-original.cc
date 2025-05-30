// Copyright 2024 The Chromium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <string>
#include <vector>

namespace {

struct Aggregate {
  int a;
  int b;
  int c;
};

Aggregate Build(int a, int b, int c) {
  return Aggregate{a, b, c};
}

}  // namespace

void test_with_structs() {
  const int index = 0;

  // Expected rewrite:
  // auto buf0 = std::to_array<Aggregate>({{13, 1, 7}, {14, 2, 5}, {15, 2, 4}});
  Aggregate buf0[] = {{13, 1, 7}, {14, 2, 5}, {15, 2, 4}};
  buf0[index].a = 0;

  // Expected rewrite:
  // std::array<Aggregate, 2> buf1 = {
  //     Build(1, 2, 3),
  //     Build(4, 5, 6),
  // };
  Aggregate buf1[2] = {
      Build(1, 2, 3),
      Build(4, 5, 6),
  };
  buf1[index].a = 0;

  // Expected rewrite:
  // std::array<Aggregate, 3> buf2 = {{
  //     Build(1, 2, 3),
  //     {1, 2, 3},
  //     Build(4, 5, 6),
  // }};
  Aggregate buf2[3] = {
      Build(1, 2, 3),
      {1, 2, 3},
      Build(4, 5, 6),
  };
  buf2[index].a = 0;
}

void test_with_arrays() {
  // Expected rewrite:
  // std::array<std::array<int, 3>, 3> buf0 = {{
  //     {0, 1, 2},
  //     {3, 4, 5},
  //     {6, 7, 8},
  // }};
  int buf0[3][3] = {
      {0, 1, 2},
      {3, 4, 5},
      {6, 7, 8},
  };
  buf0[0][0] = 0;

  // Since function returning array is not allowed, we don't need to
  // test the following:
  //   int buf1[3][3] = {
  //      BuildArray(0, 1, 2)
  //      BuildArray(3, 4, 5)
  //      BuildArray(6, 7, 8)
  //   };
}

void test_with_strings() {
  const int index = 0;
  // Expected rewrite:
  // auto buf0 = std::to_array<std::string>({"1", "2", "3"});
  std::string buf0[] = {"1", "2", "3"};
  buf0[index] = "4";
}

void test_with_const() {
  // Expected rewrite:
  // const auto data = std::to_array<bool>({false, true});
  const bool data[] = {false, true};
  (void)data[0];
}

void test_with_static() {
  // Expected rewrite:
  // static auto data = std::to_array<int>({1, 2, 3});
  static int data[] = {1, 2, 3};
  (void)data[0];
}

void test_with_constexpr() {
  // Expected rewrite:
  // constexpr const auto data = std::to_array<int>({1, 2, 3});
  constexpr const int data[] = {1, 2, 3};
  (void)data[0];
}

void test_with_volatile() {
  // Expected rewrite:
  // auto data = std::to_array<volatile int>({1, 2, 3});
  volatile int data[] = {1, 2, 3};
  (void)data[0];
}

void test_with_all_qualifiers() {
  // Expected rewrite:
  // static const auto data = std::to_array<volatile int>({1, 2, 3});
  static const volatile int data[] = {1, 2, 3};
  (void)data[0];
}

void test_with_const_char() {
  // Expected rewrite:
  // static auto data = std::to_array<const char*>({" B", " kB", " MB"});
  static const char* data[] = {" B", " kB", " MB"};
  (void)data[0];
}

void test_with_constant_const_char() {
  // Expected rewrite:
  // static const auto data = std::to_array<const char*>({" B", " kB", " MB"});
  static const char* const data[] = {" B", " kB", " MB"};
  (void)data[0];
}

void test_with_computed_size() {
  // Expected rewrite:
  // std::array<int, 2 + 2> data = {1, 2, 3, 4};
  int data[2 + 2] = {1, 2, 3, 4};
  std::ignore = data[0];
}

void test_with_constexpr_size() {
  constexpr int size = 2 + 2;
  // Expected rewrite:
  // std::array<int, size> data = {1, 2, 3, 4};
  int data[size] = {1, 2, 3, 4};
  std::ignore = data[0];
}

void test_with_const_size() {
  const int size = 2 + 2;
  // Expected rewrite:
  // std::array<int, size> data = {1, 2, 3, 4};
  int data[size] = {1, 2, 3, 4};
  std::ignore = data[0];
}

void test_brace_elision_computed_size() {
  constexpr int size = 2;
  struct Aggregate {
    int a;
    int b;
  };
  // Expected rewrite:
  // std::array<Aggregate, size> buffer = {{{1, 2}, {3, 4}}};
  Aggregate buffer[size] = {{1, 2}, {3, 4}};
  std::ignore = buffer[0];
}

void test_with_nested_vector() {
  // Expected rewrite:
  // auto data = std::to_array<std::vector<int>>({
  //     {1, 2, 3},
  //     {4, 5, 6},
  // });
  std::vector<int> data[] = {
      {1, 2, 3},
      {4, 5, 6},
  };
  std::ignore = data[0];
}

void test_with_nested_vector_const_size() {
  // Expected rewrite:
  // std::array<std::vector<int>, 2> data = {{
  //    {1, 2, 3},
  //    {4, 5, 6},
  // }};
  std::vector<int> data[2] = {
      {1, 2, 3},
      {4, 5, 6},
  };
  std::ignore = data[0];
}
