
#ifndef THIRD_PARTY_CEL_CPP_CODELAB_BENTLEY_CEL_H_
#define THIRD_PARTY_CEL_CPP_CODELAB_BENTLEY_CEL_H_

#include <string>

#include "absl/status/statusor.h"
#include "absl/strings/string_view.h"

namespace bentley::pw::cel::codelab {

// Parse a cel expression and evaluate it. This assumes no special setup for
// the evaluation environment, and that the expression results in a string
// value.
absl::StatusOr<std::string> ParseAndEvaluate(absl::string_view cel_expr);

}  // namespace bentley::pw::cel::codelab

#endif  // THIRD_PARTY_CEL_CPP_CODELAB_BENTLEY_CEL_H_
