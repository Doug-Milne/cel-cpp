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

#include <cstddef>
#include <string>
#include <utility>

#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "absl/strings/cord.h"
#include "absl/strings/string_view.h"
#include "common/any.h"
#include "common/casting.h"
#include "common/json.h"
#include "common/value.h"
#include "internal/serialize.h"
#include "internal/status_macros.h"

namespace cel::common_internal {

namespace {

std::string BoolDebugString(bool value) { return value ? "true" : "false"; }

}  // namespace

std::string BoolValueBase::DebugString() const {
  return BoolDebugString(NativeValue());
}

absl::StatusOr<Json> BoolValueBase::ConvertToJson(AnyToJsonConverter&) const {
  return NativeValue();
}

absl::StatusOr<size_t> BoolValueBase::GetSerializedSize(
    AnyToJsonConverter&) const {
  return internal::SerializedBoolValueSize(NativeValue());
}

absl::Status BoolValueBase::SerializeTo(AnyToJsonConverter&,
                                        absl::Cord& value) const {
  return internal::SerializeBoolValue(NativeValue(), value);
}

absl::StatusOr<absl::Cord> BoolValueBase::Serialize(
    AnyToJsonConverter& value_manager) const {
  absl::Cord value;
  CEL_RETURN_IF_ERROR(SerializeTo(value_manager, value));
  return value;
}

absl::StatusOr<std::string> BoolValueBase::GetTypeUrl(
    absl::string_view prefix) const {
  return MakeTypeUrlWithPrefix(prefix, "google.protobuf.BoolValue");
}

absl::StatusOr<Any> BoolValueBase::ConvertToAny(
    AnyToJsonConverter& value_manager, absl::string_view prefix) const {
  CEL_ASSIGN_OR_RETURN(auto value, Serialize(value_manager));
  CEL_ASSIGN_OR_RETURN(auto type_url, GetTypeUrl(prefix));
  return MakeAny(std::move(type_url), std::move(value));
}

absl::StatusOr<ValueView> BoolValueBase::Equal(ValueManager&, ValueView other,
                                               Value&) const {
  if (auto other_value = As<BoolValueView>(other); other_value.has_value()) {
    return BoolValueView{NativeValue() == other_value->NativeValue()};
  }
  return BoolValueView{false};
}

absl::StatusOr<Value> BoolValueBase::Equal(ValueManager& value_manager,
                                           ValueView other) const {
  Value scratch;
  CEL_ASSIGN_OR_RETURN(auto result, Equal(value_manager, other, scratch));
  return Value{result};
}

}  // namespace cel::common_internal
