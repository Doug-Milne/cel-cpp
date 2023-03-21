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

#ifndef THIRD_PARTY_CEL_CPP_EXTENSIONS_PROTOBUF_VALUE_H_
#define THIRD_PARTY_CEL_CPP_EXTENSIONS_PROTOBUF_VALUE_H_

#include <type_traits>
#include <utility>

#include "google/protobuf/duration.pb.h"
#include "google/protobuf/timestamp.pb.h"
#include "absl/base/attributes.h"
#include "absl/status/statusor.h"
#include "absl/time/time.h"
#include "base/value.h"
#include "base/value_factory.h"
#include "base/values/duration_value.h"
#include "base/values/timestamp_value.h"
#include "extensions/protobuf/enum_type.h"
#include "extensions/protobuf/struct_value.h"
#include "extensions/protobuf/type.h"
#include "internal/status_macros.h"
#include "google/protobuf/descriptor.h"
#include "google/protobuf/generated_enum_util.h"
#include "google/protobuf/message.h"

namespace cel::extensions {

// Utility class for creating and interacting with protocol buffer values.
class ProtoValue final {
 private:
  template <typename T>
  using DerivedMessage = std::conjunction<
      std::is_base_of<google::protobuf::Message, std::decay_t<T>>,
      std::negation<std::is_same<google::protobuf::Message, std::decay_t<T>>>>;

  template <typename T>
  using DurationMessage =
      std::is_same<google::protobuf::Duration, std::decay_t<T>>;

  template <typename T>
  using NotDurationMessage = std::negation<DurationMessage<T>>;

  template <typename T>
  using TimestampMessage =
      std::is_same<google::protobuf::Timestamp, std::decay_t<T>>;

  template <typename T>
  using NotTimestampMessage = std::negation<TimestampMessage<T>>;

  template <typename T>
  using DerivedEnum = google::protobuf::is_proto_enum<std::decay_t<T>>;

  template <typename T>
  using NullValueEnum =
      std::is_same<google::protobuf::NullValue, std::decay_t<T>>;

  template <typename T>
  using NotNullValueEnum = std::negation<NullValueEnum<T>>;

 public:
  // Create a new EnumValue from a generated protocol buffer enum.
  template <typename T>
  static std::enable_if_t<
      std::conjunction_v<DerivedEnum<T>, NotNullValueEnum<T>>,
      absl::StatusOr<Handle<EnumValue>>>
  Create(ValueFactory& value_factory, const T& value) {
    CEL_ASSIGN_OR_RETURN(auto type,
                         ProtoType::Resolve<T>(value_factory.type_manager()));
    return value_factory.CreateEnumValue(
        std::move(type), static_cast<std::underlying_type_t<T>>(value));
  }

  // Create NullValue.
  template <typename T>
  static std::enable_if_t<std::conjunction_v<DerivedEnum<T>, NullValueEnum<T>>,
                          absl::StatusOr<Handle<NullValue>>>
  Create(ValueFactory& value_factory, const T& value ABSL_ATTRIBUTE_UNUSED) {
    return value_factory.GetNullValue();
  }

  // Create a new StructValue from a generated protocol buffer message.
  template <typename T>
  static std::enable_if_t<
      std::conjunction_v<DerivedMessage<T>, NotDurationMessage<T>,
                         NotTimestampMessage<T>>,
      absl::StatusOr<Handle<ProtoStructValue>>>
  Create(ValueFactory& value_factory, T&& value) {
    return ProtoStructValue::Create(value_factory, std::forward<T>(value));
  }

  // Create a new DurationValue from google.protobuf.Duration.
  static absl::StatusOr<Handle<DurationValue>> Create(
      ValueFactory& value_factory, const google::protobuf::Duration& value) {
    return value_factory.CreateUncheckedDurationValue(
        absl::Seconds(value.seconds()) + absl::Nanoseconds(value.nanos()));
  }

  // Create a new TimestampValue from google.protobuf.Timestamp.
  static absl::StatusOr<Handle<TimestampValue>> Create(
      ValueFactory& value_factory, const google::protobuf::Timestamp& value) {
    return value_factory.CreateUncheckedTimestampValue(
        absl::UnixEpoch() + absl::Seconds(value.seconds()) +
        absl::Nanoseconds(value.nanos()));
  }

  // Create a new Value from a protocol buffer message.
  static absl::StatusOr<Handle<Value>> Create(ValueFactory& value_factory,
                                              const google::protobuf::Message& value);

  // Create a new Value from a protocol buffer message.
  static absl::StatusOr<Handle<Value>> Create(ValueFactory& value_factory,
                                              google::protobuf::Message&& value);

  // Create a new Value from a protocol buffer enum.
  static absl::StatusOr<Handle<Value>> Create(
      ValueFactory& value_factory, const google::protobuf::EnumDescriptor& descriptor,
      int value);

  // Create a new Value from a protocol buffer enum.
  static absl::StatusOr<Handle<Value>> Create(
      ValueFactory& value_factory, const google::protobuf::EnumValueDescriptor& value) {
    return Create(value_factory, *value.type(), value.number());
  }

 private:
  ProtoValue() = delete;
  ProtoValue(const ProtoValue&) = delete;
  ProtoValue(ProtoValue&&) = delete;
  ~ProtoValue() = delete;
  ProtoValue& operator=(const ProtoValue&) = delete;
  ProtoValue& operator=(ProtoValue&&) = delete;
};

}  // namespace cel::extensions

#endif  // THIRD_PARTY_CEL_CPP_EXTENSIONS_PROTOBUF_VALUE_H_
