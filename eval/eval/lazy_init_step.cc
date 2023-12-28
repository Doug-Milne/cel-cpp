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

#include "eval/eval/lazy_init_step.h"

#include <cstddef>
#include <cstdint>
#include <memory>

#include "absl/status/status.h"
#include "eval/eval/evaluator_core.h"
#include "eval/eval/expression_step_base.h"

namespace google::api::expr::runtime {

namespace {

class CheckLazyInitStep : public ExpressionStepBase {
 public:
  CheckLazyInitStep(size_t slot_index, size_t subexpression_index,
                    int64_t expr_id)
      : ExpressionStepBase(expr_id),
        slot_index_(slot_index),
        subexpression_index_(subexpression_index) {}

  absl::Status Evaluate(ExecutionFrame* frame) const override {
    auto* slot = frame->comprehension_slots().Get(slot_index_);
    if (slot != nullptr) {
      frame->value_stack().Push(slot->value, slot->attribute);
      // skip next step (assign to slot)
      return frame->JumpTo(1);
    }

    // return to next step (assign to slot)
    frame->Call(0, subexpression_index_);
    return absl::OkStatus();
  }

 private:
  size_t slot_index_;
  size_t subexpression_index_;
};

class AssignSlotStep : public ExpressionStepBase {
 public:
  explicit AssignSlotStep(size_t slot_index, bool should_pop)
      : ExpressionStepBase(/*expr_id=*/-1, /*comes_from_ast=*/false),
        slot_index_(slot_index),
        should_pop_(should_pop) {}

  absl::Status Evaluate(ExecutionFrame* frame) const override {
    if (!frame->value_stack().HasEnough(1)) {
      return absl::InternalError("Stack underflow assigning lazy value");
    }

    frame->comprehension_slots().Set(slot_index_, frame->value_stack().Peek(),
                                     frame->value_stack().PeekAttribute());

    if (should_pop_) {
      frame->value_stack().Pop(1);
    }

    return absl::OkStatus();
  }

 private:
  size_t slot_index_;
  bool should_pop_;
};

class ClearSlotStep : public ExpressionStepBase {
 public:
  explicit ClearSlotStep(size_t slot_index, int64_t expr_id)
      : ExpressionStepBase(expr_id), slot_index_(slot_index) {}

  absl::Status Evaluate(ExecutionFrame* frame) const override {
    frame->comprehension_slots().ClearSlot(slot_index_);
    return absl::OkStatus();
  }

 private:
  size_t slot_index_;
};

}  // namespace

std::unique_ptr<ExpressionStep> CreateCheckLazyInitStep(
    size_t slot_index, size_t subexpression_index, int64_t expr_id) {
  return std::make_unique<CheckLazyInitStep>(slot_index, subexpression_index,
                                             expr_id);
}

std::unique_ptr<ExpressionStep> CreateAssignSlotStep(size_t slot_index) {
  return std::make_unique<AssignSlotStep>(slot_index, /*should_pop=*/false);
}

std::unique_ptr<ExpressionStep> CreateAssignSlotAndPopStep(size_t slot_index) {
  return std::make_unique<AssignSlotStep>(slot_index, /*should_pop=*/true);
}

std::unique_ptr<ExpressionStep> CreateClearSlotStep(size_t slot_index,
                                                    int64_t expr_id) {
  return std::make_unique<ClearSlotStep>(slot_index, expr_id);
}

}  // namespace google::api::expr::runtime