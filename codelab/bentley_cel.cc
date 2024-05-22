
#include "bentley_cel.h"

#include <memory>
#include <string>

#include "google/api/expr/v1alpha1/syntax.pb.h"
#include "google/protobuf/arena.h"
#include "absl/status/status.h"
#include "absl/strings/str_cat.h"
#include "absl/strings/string_view.h"
#include "eval/public/activation.h"
#include "eval/public/builtin_func_registrar.h"
#include "eval/public/cel_expr_builder_factory.h"
#include "eval/public/cel_expression.h"
#include "eval/public/cel_options.h"
#include "eval/public/cel_value.h"
#include "internal/status_macros.h"
#include "parser/parser.h"
#include "common/json.h"
#include "codelab/bentley/bentley_user.pb.h"
#include "eval/public/structs/cel_proto_wrapper.h"

namespace bentley::pw::cel::codelab {
namespace {

using ::google::api::expr::v1alpha1::ParsedExpr;
using ::google::api::expr::parser::Parse;
using ::google::api::expr::runtime::Activation;
using ::google::api::expr::runtime::CelExpression;
using ::google::api::expr::runtime::CelExpressionBuilder;
using ::google::api::expr::runtime::CelValue;
using ::google::api::expr::runtime::CreateCelExpressionBuilder;
using ::google::api::expr::runtime::InterpreterOptions;
using ::google::api::expr::runtime::RegisterBuiltinFunctions;

// Convert the CelResult to a C++ string if it is string typed. Otherwise,
// return invalid argument error. This takes a copy to avoid lifecycle concerns
// (the evaluator may represent strings as stringviews backed by the input
// expression).
absl::StatusOr<std::string> ConvertResult(const CelValue& value) {
  if (CelValue::StringHolder inner_value; value.GetValue(&inner_value)) {
    return std::string(inner_value.value());
  } else {
    return absl::InvalidArgumentError(absl::StrCat(
        "expected string result got '", CelValue::TypeName(value.type()), "'"));
  }
}
}  // namespace

absl::StatusOr<std::string> ParseAndEvaluate(absl::string_view cel_expr) {
  // Setup a default environment for building expressions.
  InterpreterOptions options;
  std::unique_ptr<CelExpressionBuilder> builder =
      CreateCelExpressionBuilder(options);

  CEL_RETURN_IF_ERROR(
      RegisterBuiltinFunctions(builder->GetRegistry(), options));

  // Parse the expression. This is fine for codelabs, but this skips the type
  // checking phase. It won't check that functions and variables are available
  // in the environment, and it won't handle certain ambiguous identifier
  // expressions (e.g. container lookup vs namespaced name, packaged function
  // vs. receiver call style function).
  ParsedExpr parsed_expr;
  CEL_ASSIGN_OR_RETURN(parsed_expr, Parse(cel_expr));

  // The evaluator uses a proto Arena for incidental allocations during
  // evaluation.
  google::protobuf::Arena arena;
  // The activation provides variables and functions that are bound into the
  // expression environment. In this example, there's no context expected, so
  // we just provide an empty one to the evaluator.
  Activation activation;

  User currentUser;
  currentUser.set_username("sfalik");
  currentUser.set_desc("Shane Falik");
  currentUser.set_email("shane.falik@bentley.com");
  
  activation.InsertValue("currentUser", google::api::expr::runtime::CelProtoWrapper::CreateMessage(&currentUser, &arena));

  // Build the expression plan. This assumes that the source expression AST and
  // the expression builder outlive the CelExpression object.
  CEL_ASSIGN_OR_RETURN(std::unique_ptr<CelExpression> expression_plan,
                       builder->CreateExpression(&parsed_expr.expr(),
                                                 &parsed_expr.source_info()));

  // Actually run the expression plan. We don't support any environment
  // variables at the moment so just use an empty activation.
  CEL_ASSIGN_OR_RETURN(CelValue result,
                       expression_plan->Evaluate(activation, &arena));

  // Convert the result to a c++ string. CelValues may reference instances from
  // either the input expression, or objects allocated on the arena, so we need
  // to pass ownership (in this case by copying to a new instance and returning
  // that).
  return ConvertResult(result);
}

}  // namespace bentley::pw::cel::codelab
