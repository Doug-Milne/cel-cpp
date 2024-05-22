

package(default_visibility = ["//visibility:public"])

licenses(["notice"])

cc_binary(
    name = "bentley_try",
    srcs = ["bentley_try.cc"],
    deps = [
        "bentley_cel"
    ],
)

proto_library(
    name = "bentley_protos",
    srcs = [
        "bentley_user.proto"
    ],
    # strip_import_prefix = "/src",
    # visibility = [
    #     "//java/kotlin:__pkg__",
    #     "//java/kotlin-lite:__pkg__",
    # ],
)

cc_proto_library(
    name = "bentley_cc_protos",
    deps = [":bentley_protos"],
)


cc_library(
    name = "bentley_cel",
    srcs = ["bentley_cel.cc"],
    hdrs = ["bentley_cel.h"],
    deps = [
        ":bentley_cc_protos",
        "//eval/public:activation",
        "//eval/public:builtin_func_registrar",
        "//eval/public:cel_expr_builder_factory",
        "//eval/public:cel_expression",
        "//eval/public:cel_options",
        "//eval/public:cel_value",
        "//internal:status_macros",
        "//eval/public/structs:cel_proto_wrapper",
        "//parser",
        "@com_google_absl//absl/status",
        "@com_google_absl//absl/status:statusor",
        "@com_google_absl//absl/strings",
        "@com_google_googleapis//google/api/expr/v1alpha1:syntax_cc_proto",
        "@com_google_protobuf//:protobuf",
    ],
)

