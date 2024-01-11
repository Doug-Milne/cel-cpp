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

#ifndef THIRD_PARTY_CEL_CPP_COMMON_VALUE_PROVIDER_H_
#define THIRD_PARTY_CEL_CPP_COMMON_VALUE_PROVIDER_H_

#include "absl/base/attributes.h"
#include "absl/status/statusor.h"
#include "absl/strings/cord.h"
#include "absl/strings/string_view.h"
#include "absl/types/optional.h"
#include "common/memory.h"
#include "common/type.h"
#include "common/type_provider.h"
#include "common/value.h"
#include "common/value_factory.h"

namespace cel {

// `ValueProvider` is an interface for constructing new instances of types are
// runtime. It handles type reflection.
class ValueProvider : public virtual TypeProvider {
 public:
  // `NewListValueBuilder` returns a new `ListValueBuilderInterface` for the
  // corresponding `ListType` `type`.
  absl::StatusOr<Unique<ListValueBuilder>> NewListValueBuilder(
      ValueFactory& value_factory, ListTypeView type);

  // `NewMapValueBuilder` returns a new `MapValueBuilderInterface` for the
  // corresponding `MapType` `type`.
  absl::StatusOr<Unique<MapValueBuilder>> NewMapValueBuilder(
      ValueFactory& value_factory, MapTypeView type);

  // `NewStructValueBuilder` returns a new `StructValueBuilder` for the
  // corresponding `StructType` `type`.
  virtual absl::StatusOr<absl::optional<Unique<StructValueBuilder>>>
  NewStructValueBuilder(ValueFactory& value_factory, StructTypeView type) = 0;

  // `NewValueBuilder` returns a new `ValueBuilder` for the corresponding type
  // `name`.  It is primarily used to handle wrapper types which sometimes show
  // up literally in expressions.
  absl::StatusOr<absl::optional<Unique<ValueBuilder>>> NewValueBuilder(
      ValueFactory& value_factory, absl::string_view name);

  // `FindValue` returns a new `Value` for the corresponding name `name`. This
  // can be used to translate enum names to numeric values.
  virtual absl::StatusOr<absl::optional<ValueView>> FindValue(
      ValueFactory& value_factory, absl::string_view name,
      Value& scratch ABSL_ATTRIBUTE_LIFETIME_BOUND) = 0;

  // `DeserializeValue` deserializes the bytes of `value` according to
  // `type_url`. Returns `NOT_FOUND` if `type_url` is unrecognized.
  absl::StatusOr<absl::optional<Value>> DeserializeValue(
      ValueFactory& value_factory, absl::string_view type_url,
      const absl::Cord& value);

 protected:
  virtual absl::StatusOr<absl::optional<Value>> DeserializeValueImpl(
      ValueFactory& value_factory, absl::string_view type_url,
      const absl::Cord& value);
};

Shared<ValueProvider> NewThreadCompatibleValueProvider(
    MemoryManagerRef memory_manager);

Shared<ValueProvider> NewThreadSafeValueProvider(
    MemoryManagerRef memory_manager);

}  // namespace cel

#endif  // THIRD_PARTY_CEL_CPP_COMMON_VALUE_PROVIDER_H_
