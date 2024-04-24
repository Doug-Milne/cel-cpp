#include "eval/eval/create_list_step.h"

#include <cstddef>
#include <cstdint>
#include <memory>
#include <utility>
#include <vector>

#include "absl/container/flat_hash_set.h"
#include "absl/status/status.h"
#include "absl/status/statusor.h"
#include "absl/types/optional.h"
#include "base/ast_internal/expr.h"
#include "common/casting.h"
#include "common/value.h"
#include "eval/eval/attribute_trail.h"
#include "eval/eval/attribute_utility.h"
#include "eval/eval/direct_expression_step.h"
#include "eval/eval/evaluator_core.h"
#include "eval/eval/expression_step_base.h"
#include "internal/status_macros.h"
#include "runtime/internal/mutable_list_impl.h"

namespace google::api::expr::runtime {

namespace {

using ::cel::Cast;
using ::cel::ErrorValue;
using ::cel::InstanceOf;
using ::cel::ListValueBuilderInterface;
using ::cel::UnknownValue;
using ::cel::Value;
using ::cel::runtime_internal::MutableListValue;

class CreateListStep : public ExpressionStepBase {
 public:
  CreateListStep(int64_t expr_id, int list_size, bool immutable,
                 absl::flat_hash_set<int> optional_indices)
      : ExpressionStepBase(expr_id),
        list_size_(list_size),
        immutable_(immutable),
        optional_indices_(std::move(optional_indices)) {}

  absl::Status Evaluate(ExecutionFrame* frame) const override;

 private:
  int list_size_;
  bool immutable_;
  absl::flat_hash_set<int32_t> optional_indices_;
};

absl::Status CreateListStep::Evaluate(ExecutionFrame* frame) const {
  if (list_size_ < 0) {
    return absl::Status(absl::StatusCode::kInternal,
                        "CreateListStep: list size is <0");
  }

  if (!frame->value_stack().HasEnough(list_size_)) {
    return absl::Status(absl::StatusCode::kInternal,
                        "CreateListStep: stack underflow");
  }

  auto args = frame->value_stack().GetSpan(list_size_);

  cel::Value result;
  for (const auto& arg : args) {
    if (arg->Is<cel::ErrorValue>()) {
      result = arg;
      frame->value_stack().Pop(list_size_);
      frame->value_stack().Push(std::move(result));
      return absl::OkStatus();
    }
  }

  if (frame->enable_unknowns()) {
    absl::optional<UnknownValue> unknown_set =
        frame->attribute_utility().IdentifyAndMergeUnknowns(
            args, frame->value_stack().GetAttributeSpan(list_size_),
            /*use_partial=*/true);
    if (unknown_set.has_value()) {
      frame->value_stack().Pop(list_size_);
      frame->value_stack().Push(std::move(unknown_set).value());
      return absl::OkStatus();
    }
  }

  CEL_ASSIGN_OR_RETURN(auto builder,
                       frame->value_manager().NewListValueBuilder(
                           frame->value_manager().GetDynListType()));

  builder->Reserve(args.size());
  for (size_t i = 0; i < args.size(); ++i) {
    auto& arg = args[i];
    if (optional_indices_.contains(static_cast<int32_t>(i))) {
      if (auto optional_arg = cel::As<cel::OptionalValue>(arg); optional_arg) {
        if (!optional_arg->HasValue()) {
          continue;
        }
        CEL_RETURN_IF_ERROR(builder->Add(optional_arg->Value()));
      } else {
        return cel::TypeConversionError(arg.GetTypeName(), "optional_type")
            .NativeValue();
      }
    } else {
      CEL_RETURN_IF_ERROR(builder->Add(std::move(arg)));
    }
  }

  if (immutable_) {
    result = std::move(*builder).Build();
  } else {
    result = cel::OpaqueValue{
        frame->value_manager().GetMemoryManager().MakeShared<MutableListValue>(
            std::move(builder))};
  }
  frame->value_stack().PopAndPush(list_size_, std::move(result));
  return absl::OkStatus();
}

absl::flat_hash_set<int32_t> MakeOptionalIndicesSet(
    const cel::ast_internal::CreateList& create_list_expr) {
  absl::flat_hash_set<int32_t> optional_indices;
  for (const auto& optional_index : create_list_expr.optional_indices()) {
    optional_indices.insert(optional_index);
  }
  return optional_indices;
}

class CreateListDirectStep : public DirectExpressionStep {
 public:
  CreateListDirectStep(
      std::vector<std::unique_ptr<DirectExpressionStep>> elements,
      absl::flat_hash_set<int32_t> optional_indices, int64_t expr_id)
      : DirectExpressionStep(expr_id),
        elements_(std::move(elements)),
        optional_indices_(std::move(optional_indices)) {}

  absl::Status Evaluate(ExecutionFrameBase& frame, Value& result,
                        AttributeTrail& attribute_trail) const override {
    CEL_ASSIGN_OR_RETURN(auto builder,
                         frame.value_manager().NewListValueBuilder(
                             frame.value_manager().GetDynListType()));

    builder->Reserve(elements_.size());
    AttributeUtility::Accumulator unknowns =
        frame.attribute_utility().CreateAccumulator();
    AttributeTrail tmp_attr;

    for (size_t i = 0; i < elements_.size(); ++i) {
      const auto& element = elements_[i];
      CEL_RETURN_IF_ERROR(element->Evaluate(frame, result, tmp_attr));

      if (cel::InstanceOf<ErrorValue>(result)) return absl::OkStatus();

      if (frame.attribute_tracking_enabled()) {
        if (frame.missing_attribute_errors_enabled()) {
          if (frame.attribute_utility().CheckForMissingAttribute(tmp_attr)) {
            CEL_ASSIGN_OR_RETURN(
                result, frame.attribute_utility().CreateMissingAttributeError(
                            tmp_attr.attribute()));
            return absl::OkStatus();
          }
        }
        if (frame.unknown_processing_enabled()) {
          if (InstanceOf<UnknownValue>(result)) {
            unknowns.Add(Cast<UnknownValue>(result));
          }
          if (frame.attribute_utility().CheckForUnknown(tmp_attr,
                                                        /*use_partial=*/true)) {
            unknowns.Add(tmp_attr);
          }
        }
      }

      // Conditionally add if optional.
      if (optional_indices_.contains(static_cast<int32_t>(i))) {
        if (auto optional_arg =
                cel::As<cel::OptionalValue>(static_cast<const Value&>(result));
            optional_arg) {
          if (!optional_arg->HasValue()) {
            continue;
          }
          CEL_RETURN_IF_ERROR(builder->Add(optional_arg->Value()));
          continue;
        }
        return cel::TypeConversionError(result.GetTypeName(), "optional_type")
            .NativeValue();
      }

      // Otherwise just add.
      CEL_RETURN_IF_ERROR(builder->Add(std::move(result)));
    }

    if (!unknowns.IsEmpty()) {
      result = std::move(unknowns).Build();
      return absl::OkStatus();
    }
    result = std::move(*builder).Build();

    return absl::OkStatus();
  }

 private:
  std::vector<std::unique_ptr<DirectExpressionStep>> elements_;
  absl::flat_hash_set<int32_t> optional_indices_;
};

}  // namespace

// TODO(uncreated-issue/67): optional support.
std::unique_ptr<DirectExpressionStep> CreateDirectListStep(
    std::vector<std::unique_ptr<DirectExpressionStep>> deps,
    absl::flat_hash_set<int32_t> optional_indices, int64_t expr_id) {
  return std::make_unique<CreateListDirectStep>(
      std::move(deps), std::move(optional_indices), expr_id);
}

absl::StatusOr<std::unique_ptr<ExpressionStep>> CreateCreateListStep(
    const cel::ast_internal::CreateList& create_list_expr, int64_t expr_id) {
  return std::make_unique<CreateListStep>(
      expr_id, create_list_expr.elements().size(), /*immutable=*/true,
      MakeOptionalIndicesSet(create_list_expr));
}

absl::StatusOr<std::unique_ptr<ExpressionStep>> CreateCreateMutableListStep(
    const cel::ast_internal::CreateList& create_list_expr, int64_t expr_id) {
  return std::make_unique<CreateListStep>(
      expr_id, create_list_expr.elements().size(), /*immutable=*/false,
      MakeOptionalIndicesSet(create_list_expr));
}

}  // namespace google::api::expr::runtime
