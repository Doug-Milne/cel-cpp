// Copyright 2023 Google LLC
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     https://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "base/internal/function_adapter.h"

#include "absl/status/status.h"
#include "base/memory_manager.h"
#include "base/value_factory.h"
#include "base/values/double_value.h"
#include "base/values/int_value.h"
#include "base/values/uint_value.h"
#include "internal/testing.h"

namespace cel::internal {
namespace {

using cel::internal::StatusIs;

static_assert(AdaptedKind<int64_t>() == Kind::kInt, "int adapts to int64_t");
static_assert(AdaptedKind<uint64_t>() == Kind::kUint,
              "uint adapts to uint64_t");
static_assert(AdaptedKind<double>() == Kind::kDouble,
              "double adapts to double");
static_assert(AdaptedKind<Handle<Value>>() == Kind::kAny,
              "any adapts to Handle<Value>");

class ValueFactoryTestBase : public testing::Test {
 public:
  ValueFactoryTestBase()
      : type_factory_(MemoryManager::Global()),
        type_manager_(type_factory_, TypeProvider::Builtin()),
        value_factory_(type_manager_) {}

  ValueFactory& value_factory() { return value_factory_; }

 private:
  TypeFactory type_factory_;
  TypeManager type_manager_;
  ValueFactory value_factory_;
};

class HandleToAdaptedVisitorTest : public ValueFactoryTestBase {};

TEST_F(HandleToAdaptedVisitorTest, Int) {
  Handle<Value> v = value_factory().CreateIntValue(10);

  int64_t out;
  ASSERT_OK(HandleToAdaptedVisitor{v}(&out));

  EXPECT_EQ(out, 10);
}

TEST_F(HandleToAdaptedVisitorTest, IntWrongKind) {
  Handle<Value> v = value_factory().CreateUintValue(10);

  int64_t out;
  EXPECT_THAT(
      HandleToAdaptedVisitor{v}(&out),
      StatusIs(absl::StatusCode::kInvalidArgument, "expected int value"));
}

TEST_F(HandleToAdaptedVisitorTest, Uint) {
  Handle<Value> v = value_factory().CreateUintValue(11);

  uint64_t out;
  ASSERT_OK(HandleToAdaptedVisitor{v}(&out));

  EXPECT_EQ(out, 11);
}

TEST_F(HandleToAdaptedVisitorTest, UintWrongKind) {
  Handle<Value> v = value_factory().CreateIntValue(11);

  uint64_t out;
  EXPECT_THAT(
      HandleToAdaptedVisitor{v}(&out),
      StatusIs(absl::StatusCode::kInvalidArgument, "expected uint value"));
}

TEST_F(HandleToAdaptedVisitorTest, Double) {
  Handle<Value> v = value_factory().CreateDoubleValue(12.0);

  double out;
  ASSERT_OK(HandleToAdaptedVisitor{v}(&out));

  EXPECT_EQ(out, 12.0);
}

TEST_F(HandleToAdaptedVisitorTest, DoubleWrongKind) {
  Handle<Value> v = value_factory().CreateUintValue(10);

  double out;
  EXPECT_THAT(
      HandleToAdaptedVisitor{v}(&out),
      StatusIs(absl::StatusCode::kInvalidArgument, "expected double value"));
}

class AdaptedToHandleVisitorTest : public ValueFactoryTestBase {};

TEST_F(AdaptedToHandleVisitorTest, Int) {
  int64_t value = 10;

  ASSERT_OK_AND_ASSIGN(auto result,
                       AdaptedToHandleVisitor{value_factory()}(value));

  ASSERT_TRUE(result.Is<IntValue>());
  EXPECT_EQ(result.As<IntValue>()->value(), 10);
}

TEST_F(AdaptedToHandleVisitorTest, Double) {
  double value = 10;

  ASSERT_OK_AND_ASSIGN(auto result,
                       AdaptedToHandleVisitor{value_factory()}(value));

  ASSERT_TRUE(result.Is<DoubleValue>());
  EXPECT_EQ(result.As<DoubleValue>()->value(), 10.0);
}

TEST_F(AdaptedToHandleVisitorTest, Uint) {
  uint64_t value = 10;

  ASSERT_OK_AND_ASSIGN(auto result,
                       AdaptedToHandleVisitor{value_factory()}(value));

  ASSERT_TRUE(result.Is<UintValue>());
  EXPECT_EQ(result.As<UintValue>()->value(), 10);
}

TEST_F(AdaptedToHandleVisitorTest, StatusOrValue) {
  absl::StatusOr<int64_t> value = 10;

  ASSERT_OK_AND_ASSIGN(auto result,
                       AdaptedToHandleVisitor{value_factory()}(value));

  ASSERT_TRUE(result.Is<IntValue>());
  EXPECT_EQ(result.As<IntValue>()->value(), 10);
}

TEST_F(AdaptedToHandleVisitorTest, StatusOrError) {
  absl::StatusOr<int64_t> value = absl::InternalError("test_error");

  EXPECT_THAT(AdaptedToHandleVisitor{value_factory()}(value).status(),
              StatusIs(absl::StatusCode::kInternal, "test_error"));
}

TEST_F(AdaptedToHandleVisitorTest, Any) {
  auto handle =
      value_factory().CreateErrorValue(absl::InternalError("test_error"));

  ASSERT_OK_AND_ASSIGN(auto result,
                       AdaptedToHandleVisitor{value_factory()}(handle));

  ASSERT_TRUE(result.Is<ErrorValue>());
  EXPECT_THAT(result.As<ErrorValue>()->value(),
              StatusIs(absl::StatusCode::kInternal, "test_error"));
}

}  // namespace
}  // namespace cel::internal
