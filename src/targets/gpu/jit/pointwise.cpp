#include <migraphx/gpu/compiler.hpp>
#include <migraphx/gpu/compile_hip_code_object.hpp>
#include <migraphx/gpu/compile_hip.hpp>
#include <migraphx/gpu/context.hpp>
#include <migraphx/cpp_generator.hpp>
#include <migraphx/ranges.hpp>
#include <migraphx/reduce_dims.hpp>
#include <migraphx/stringutils.hpp>
#include <migraphx/dead_code_elimination.hpp>
#include <migraphx/eliminate_common_subexpression.hpp>
#include <migraphx/module.hpp>
#include <migraphx/pass_manager.hpp>

namespace migraphx {
inline namespace MIGRAPHX_INLINE_NS {
namespace gpu {

static const char* const pointwise_kernel = R"__migraphx__(
#include <migraphx/kernels/index.hpp>
#include <migraphx/kernels/pointwise.hpp>
#include <args.hpp>

namespace migraphx {

${preamble}

extern "C" {
__global__ void kernel(${params}) 
{
    pointwise(${lambda}, ${args});
}
    
}

} // namespace migraphx

)__migraphx__";

struct pointwise_compiler : compiler<pointwise_compiler>
{
    std::string name() const { return "pointwise"; }

    operation compile_op(context&, const std::vector<shape>& inputs, const value& v) const
    {
        hip_compile_options options;
        options.global         = v.get("global", compute_global(inputs.front().elements()));
        options.local          = v.get("local", 1024);
        options.inputs         = inputs;
        options.output         = inputs.back();
        options.virtual_inputs = reduce_dims(inputs);
        options.params         = "-Wno-float-equal";
        auto src               = interpolate_string(pointwise_kernel,
                                    {{"params", enum_params(inputs.size(), "void * private_p")},
                                    {"args", enum_params(inputs.size(), "private_p")},
                                    {"lambda", v.at("lambda").to<std::string>()},
                                    {"preamble", v.get("preamble", std::string{})}});
        return compile_hip_code_object(src, options);
    }

    compiler_replace compile(context& ctx, instruction_ref ins, operation) const
    {
        assert(not ins->module_inputs().empty());
        auto* pm = ins->module_inputs().front();
        run_passes(*pm, {eliminate_common_subexpression{}, dead_code_elimination{}});
        cpp_generator g;
        g.fmap([](const std::string& fname) { return "migraphx::" + fname; });
        g.add_point_op("where", "${function:where}(${0}, ${1}, ${2})");
        g.add_point_op("prelu", "${function:where}(${0} < 0, ${0} * ${1}, ${0})");
        g.add_point_op("sign", "${function:where}(${0} > 0, 1, ${function:where}(${0} < 0, -1, 0))");
        g.add_point_op("equal", "migraphx::abs(${0} == ${1})");
        g.add_point_op("less", "migraphx::abs(${0} < ${1})");
        g.add_point_op("greater", "migraphx::abs(${0} > ${1})");
        g.add_point_op("not", "migraphx::abs(not ${0})");
        // Add explict conversions
        g.fresult(
            [](const shape& s) { return "migraphx::convert<" + shape::cpp_type(s.type()) + ">"; });
        auto name =
            g.create_function(g.generate_module(*pm).set_attributes({"__device__"}).set_generic_types(*pm));
        std::string lambda = "MIGRAPHX_LIFT(" + name + ")";
        return replace(compile_op(ctx, to_shapes(ins->inputs()), {{"lambda", lambda}, {"preamble", g.str()}}));
    }
};
} // namespace gpu
} // namespace MIGRAPHX_INLINE_NS
} // namespace migraphx
