#ifndef MIGRAPHX_GUARD_SCHEDULE_MODEL_HPP
#define MIGRAPHX_GUARD_SCHEDULE_MODEL_HPP

#include <cassert>
#include <string>
#include <functional>
#include <memory>
#include <type_traits>
#include <utility>

#include <migraphx/config.hpp>
#include <migraphx/instruction_ref.hpp>
#include <vector>

namespace migraphx {
inline namespace MIGRAPHX_INLINE_NS {

struct module;
struct operation;

#ifdef DOXYGEN

/// An interface for target-dependent model for the scheduler
struct schedule_model
{
    /// Get the number of concurrent instruction allowed
    std::size_t concurrency() const;
    /// Schedule a concurrent instruction
    void sched(module& m, instruction_ref ins, std::size_t n) const;
    // Insert necessary waits before an instruction
    void wait(module& m, instruction_ref ins, std::size_t wait_id) const;
    // Insert necessary records after an instruction
    void record(module& m, instruction_ref ins, std::size_t wait_id) const;
    /// Compute weights for an operation
    std::size_t weight(const operation& op) const;
};

#else

<%
interface('schedule_model',
    virtual('concurrency', returns='std::size_t', const=True),
    virtual('sched', m='module&', ins='instruction_ref', n='std::size_t', const=True),
    virtual('wait', m='module&', ins='instruction_ref', wait_id='std::size_t', const=True),
    virtual('record', m='module&', ins='instruction_ref', wait_id='std::size_t', const=True),
    virtual('weight', returns='std::size_t', op='const operation&', const=True)
)
%>

#endif

} // namespace MIGRAPHX_INLINE_NS
} // namespace migraphx

#endif
