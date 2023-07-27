// Copyright 2022 Google LLC
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

#include "base/values/null_value.h"

#include <string>

#include "absl/strings/cord.h"
#include "absl/strings/string_view.h"
#include "common/any.h"

namespace cel {

CEL_INTERNAL_VALUE_IMPL(NullValue);

std::string NullValue::DebugString() { return "null"; }

absl::StatusOr<Any> NullValue::ConvertToAny(ValueFactory&) const {
  return MakeAny(MakeTypeUrl("google.protobuf.Value"),
                 absl::Cord(absl::string_view("\x08\x00", 2)));
}

absl::StatusOr<Json> NullValue::ConvertToJson(ValueFactory&) const {
  return kJsonNull;
}

}  // namespace cel
