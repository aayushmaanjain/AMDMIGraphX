#include <migraphx/memory_coloring.hpp>
#include "memory_coloring_impl.hpp"

namespace migraphx {
inline namespace MIGRAPHX_INLINE_NS {

void memory_coloring::apply(module& m) const
{
    if(!enabled(MIGRAPHX_DISABLE_MEMORY_COLORING{}))
    {
        memory_coloring_impl opt(&m, allocation_op, verify);
        opt.run();
    }
}

} // namespace MIGRAPHX_INLINE_NS
} // namespace migraphx
