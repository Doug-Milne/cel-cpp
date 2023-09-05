#ifndef THIRD_PARTY_CEL_CPP_EVAL_EVAL_COMPREHENSION_STEP_H_
#define THIRD_PARTY_CEL_CPP_EVAL_EVAL_COMPREHENSION_STEP_H_

#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>

#include "eval/eval/evaluator_core.h"
#include "eval/eval/expression_step_base.h"

namespace google::api::expr::runtime {

class ComprehensionNextStep : public ExpressionStepBase {
 public:
  ComprehensionNextStep(size_t slot_offset, int64_t expr_id);

  void set_jump_offset(int offset);
  void set_error_jump_offset(int offset);

  absl::Status Evaluate(ExecutionFrame* frame) const override;

 private:
  size_t iter_slot_;
  size_t accu_slot_;
  int jump_offset_;
  int error_jump_offset_;
};

class ComprehensionCondStep : public ExpressionStepBase {
 public:
  ComprehensionCondStep(size_t slot_offset, bool shortcircuiting,
                        int64_t expr_id);

  void set_jump_offset(int offset);
  void set_error_jump_offset(int offset);

  absl::Status Evaluate(ExecutionFrame* frame) const override;

 private:
  size_t iter_slot_;
  size_t accu_slot_;
  int jump_offset_;
  int error_jump_offset_;
  bool shortcircuiting_;
};

// Creates a cleanup step for the comprehension.
// Removes the comprehension context then pushes the 'result' sub expression to
// the top of the stack.
std::unique_ptr<ExpressionStep> CreateComprehensionFinishStep(
    size_t slot_offset, int64_t expr_id);

// Creates a step that checks that the input is iterable and setups the loop
// context for the comprehension.
std::unique_ptr<ExpressionStep> CreateComprehensionInitStep(int64_t expr_id);

}  // namespace google::api::expr::runtime

#endif  // THIRD_PARTY_CEL_CPP_EVAL_EVAL_COMPREHENSION_STEP_H_
