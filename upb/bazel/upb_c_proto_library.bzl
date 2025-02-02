"""upb_c_proto_library() exposes upb's generated C API for protobuf (foo.upb.h)"""

load("//upb/bazel:upb_minitable_proto_library.bzl", "UpbMinitableCcInfo", "upb_minitable_proto_library_aspect")
load("//upb/bazel:upb_proto_library_internal/aspect.bzl", "upb_proto_aspect_impl")
load("//upb/bazel:upb_proto_library_internal/cc_library_func.bzl", "upb_use_cpp_toolchain")
load("//upb/bazel:upb_proto_library_internal/rule.bzl", "upb_proto_rule_impl")

UpbWrappedCcInfo = provider(
    "Provider for cc_info for protos",
    fields = ["cc_info", "cc_info_with_thunks"],
)

_UpbWrappedGeneratedSrcsInfo = provider(
    "Provider for generated sources",
    fields = ["srcs"],
)

def _upb_c_proto_library_aspect_impl(target, ctx):
    return upb_proto_aspect_impl(
        target = target,
        ctx = ctx,
        generator = "upb",
        cc_provider = UpbWrappedCcInfo,
        dep_cc_provider = UpbMinitableCcInfo,
        file_provider = _UpbWrappedGeneratedSrcsInfo,
        provide_cc_shared_library_hints = False,
    )

upb_c_proto_library_aspect = aspect(
    attrs = {
        "_copts": attr.label(
            default = "//upb:upb_proto_library_copts__for_generated_code_only_do_not_use",
        ),
        "_gen_upb": attr.label(
            executable = True,
            cfg = "exec",
            default = "//upb/upbc:protoc-gen-upb_stage1",
        ),
        "_protoc": attr.label(
            executable = True,
            cfg = "exec",
            default = "//:protoc",
        ),
        "_cc_toolchain": attr.label(
            default = "@bazel_tools//tools/cpp:current_cc_toolchain",
        ),
        "_upb": attr.label_list(default = [
            "//upb:generated_code_support__only_for_generated_code_do_not_use__i_give_permission_to_break_me",
        ]),
    },
    implementation = _upb_c_proto_library_aspect_impl,
    requires = [upb_minitable_proto_library_aspect],
    required_aspect_providers = [UpbMinitableCcInfo],
    provides = [
        UpbWrappedCcInfo,
        _UpbWrappedGeneratedSrcsInfo,
    ],
    attr_aspects = ["deps"],
    fragments = ["cpp"],
    toolchains = upb_use_cpp_toolchain(),
    incompatible_use_toolchain_transition = True,
)

def _upb_c_proto_library_rule_impl(ctx):
    return upb_proto_rule_impl(ctx, UpbWrappedCcInfo, _UpbWrappedGeneratedSrcsInfo)

upb_c_proto_library = rule(
    output_to_genfiles = True,
    implementation = _upb_c_proto_library_rule_impl,
    attrs = {
        "deps": attr.label_list(
            aspects = [upb_c_proto_library_aspect],
            allow_rules = ["proto_library"],
            providers = [ProtoInfo],
        ),
    },
    provides = [CcInfo],
)
