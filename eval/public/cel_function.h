#ifndef THIRD_PARTY_CEL_CPP_EVAL_PUBLIC_CEL_FUNCTION_H_
#define THIRD_PARTY_CEL_CPP_EVAL_PUBLIC_CEL_FUNCTION_H_

#include <memory>
#include <string>
#include <utility>
#include <vector>

#include "absl/status/status.h"
#include "absl/strings/string_view.h"
#include "absl/types/span.h"
#include "eval/public/cel_value.h"

namespace google::api::expr::runtime {

// Type that describes CelFunction.
// This complex structure is needed for overloads support.
class CelFunctionDescriptor {
 public:
  CelFunctionDescriptor(absl::string_view name, bool receiver_style,
                        std::vector<CelValue::Type> types,
                        bool is_strict = true)
      : impl_(std::make_shared<Impl>(name, receiver_style, std::move(types),
                                     is_strict)) {}

  // Function name.
  const std::string& name() const { return impl_->name; }

  // Whether function is receiver style i.e. true means arg0.name(args[1:]...).
  bool receiver_style() const { return impl_->receiver_style; }

  // The argmument types the function accepts.
  const std::vector<CelValue::Type>& types() const { return impl_->types; }

  // if true (strict, default), error or unknown arguments are propagated
  // instead of calling the function. if false (non-strict), the function may
  // receive error or unknown values as arguments.
  bool is_strict() const { return impl_->is_strict; }

  // Helper for matching a descriptor. This tests that the shape is the same --
  // |other| accepts the same number and types of arguments and is the same call
  // style).
  bool ShapeMatches(const CelFunctionDescriptor& other) const {
    return ShapeMatches(other.receiver_style(), other.types());
  }
  bool ShapeMatches(bool receiver_style,
                    const std::vector<CelValue::Type>& types) const;

  bool operator==(const CelFunctionDescriptor& other) const;

  bool operator<(const CelFunctionDescriptor& other) const;

 private:
  struct Impl final {
    Impl(absl::string_view name, bool receiver_style,
         std::vector<CelValue::Type> types, bool is_strict)
        : name(name),
          receiver_style(receiver_style),
          types(std::move(types)),
          is_strict(is_strict) {}

    std::string name;
    bool receiver_style;
    std::vector<CelValue::Type> types;
    bool is_strict;
  };

  std::shared_ptr<const Impl> impl_;
};

// CelFunction is a handler that represents single
// CEL function.
// CelFunction provides Evaluate() method, that performs
// evaluation of the function. CelFunction instances provide
// descriptors that contain function information:
// - name
// - is function receiver style (e.f(g) vs f(e,g))
// - amount of arguments and their types.
// Function overloads are resolved based on their arguments and
// receiver style.
class CelFunction {
 public:
  // Build CelFunction from descriptor
  explicit CelFunction(CelFunctionDescriptor descriptor)
      : descriptor_(std::move(descriptor)) {}

  // Non-copyable
  CelFunction(const CelFunction& other) = delete;
  CelFunction& operator=(const CelFunction& other) = delete;

  virtual ~CelFunction() {}

  // Evaluates CelValue based on arguments supplied.
  // If result content is to be allocated (e.g. string concatenation),
  // arena parameter must be used as allocation manager.
  // Provides resulting value in *result, returns evaluation success/failure.
  // Methods should discriminate between internal evaluator errors, that
  // makes further evaluation impossible or unreasonable (example - argument
  // type or number mismatch) and business logic errors (example division by
  // zero). When former happens, error Status is returned and *result is
  // not changed. In case of business logic error, returned Status is Ok, and
  // error is provided as CelValue - wrapped CelError in *result.
  virtual absl::Status Evaluate(absl::Span<const CelValue> arguments,
                                CelValue* result,
                                google::protobuf::Arena* arena) const = 0;

  // Determines whether instance of CelFunction is applicable to
  // arguments supplied.
  // Method is called during runtime.
  bool MatchArguments(absl::Span<const CelValue> arguments) const;

  // CelFunction descriptor
  const CelFunctionDescriptor& descriptor() const { return descriptor_; }

 private:
  CelFunctionDescriptor descriptor_;
};

}  // namespace google::api::expr::runtime

#endif  // THIRD_PARTY_CEL_CPP_EVAL_PUBLIC_CEL_FUNCTION_H_
