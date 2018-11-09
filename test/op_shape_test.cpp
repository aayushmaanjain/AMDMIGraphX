#include <migraph/program.hpp>
#include <migraph/iterator_for.hpp>
#include <migraph/instruction.hpp>
#include <migraph/operators.hpp>
#include <sstream>
#include "test.hpp"

template <class... Ts>
void expect_shape(const migraph::shape& expected, const migraph::operation& op, Ts... xs)
{
    migraph::program p;
    std::vector<migraph::shape> shapes{xs...};
    std::vector<migraph::instruction_ref> args(shapes.size());
    std::transform(
        shapes.begin(), shapes.end(), args.begin(), [&](auto&& s) { return p.add_outline(s); });
    p.add_instruction(op, args);
    if(p.get_shape() != expected)
    {
        std::cout << "FAILED: Incorrect shape for " << op.name() << ": ";
        std::cout << expected << " != " << p.get_shape() << std::endl;
        for(auto&& s : shapes)
            std::cout << "    " << s << std::endl;
    }
}

template <class... Ts>
void throws_shape(const migraph::operation& op, Ts... xs)
{
    migraph::program p;
    std::vector<migraph::shape> shapes{xs...};
    std::vector<migraph::instruction_ref> args(shapes.size());
    std::transform(
        shapes.begin(), shapes.end(), args.begin(), [&](auto&& s) { return p.add_outline(s); });
    bool thrown = test::throws([&] { p.add_instruction(op, args); });
    if(not thrown)
    {
        std::cout << "FAILED: No error found for " << op.name() << ": ";
        for(auto&& s : shapes)
            std::cout << "    " << s << std::endl;
    }
}

template <class...>
struct always_false : std::false_type
{
};

template <class... Ts>
void throws_shape(const migraph::shape&, Ts...)
{
    static_assert(always_false<Ts...>{},
                  "An expected shape should not be passed to throws_shape function");
}

void batch_norm_inference_shape()
{
    const size_t channels = 3;
    migraph::shape s{migraph::shape::float_type, {4, channels, 3, 3}};
    migraph::shape vars{migraph::shape::float_type, {channels}};
    expect_shape(s, migraph::op::batch_norm_inference{}, s, vars, vars, vars, vars);
    throws_shape(migraph::op::batch_norm_inference{}, s);
    throws_shape(migraph::op::batch_norm_inference{}, s, vars, vars, vars, vars, vars);
}

void convolution_shape()
{
    migraph::shape output{migraph::shape::float_type, {4, 4, 1, 1}};
    migraph::shape input{migraph::shape::float_type, {4, 3, 3, 3}};
    migraph::shape weights{migraph::shape::float_type, {4, 3, 3, 3}};
    expect_shape(output, migraph::op::convolution{}, input, weights);
    throws_shape(migraph::op::convolution{}, input);

    migraph::shape input2{migraph::shape::float_type, {3, 3}};
    migraph::shape weights2{migraph::shape::float_type, {3, 3}};
    throws_shape(migraph::op::convolution{}, input2, weights2);
    throws_shape(migraph::op::convolution{}, input2, weights);
}

void transpose_shape()
{
    migraph::shape input{migraph::shape::float_type, {2, 2}};
    migraph::shape output{migraph::shape::float_type, {2, 2}, {1, 2}};
    expect_shape(input, migraph::op::transpose{{0, 1}}, input);
    expect_shape(output, migraph::op::transpose{{1, 0}}, input);
    throws_shape(migraph::op::transpose{{1, 2}}, input);
}

void contiguous_shape()
{
    migraph::shape output{migraph::shape::float_type, {2, 2}};
    migraph::shape input{migraph::shape::float_type, {2, 2}, {1, 2}};
    expect_shape(output, migraph::op::contiguous{}, input);
    throws_shape(migraph::op::contiguous{}, input, input);

    migraph::shape single{migraph::shape::float_type, {2}};
    expect_shape(single, migraph::op::contiguous{}, single);
}

void reshape_shape()
{
    migraph::shape input{migraph::shape::float_type, {24, 1, 1, 1}};
    for(auto&& new_shape :
        std::vector<std::vector<int64_t>>{{8, 3, 1, 1}, {1, 3, 4, 2}, {1, 3, 4, 2}})
    {
        std::vector<std::size_t> lens(new_shape.size());
        std::copy(new_shape.begin(), new_shape.end(), lens.begin());
        migraph::shape output{migraph::shape::float_type, lens};
        expect_shape(output, migraph::op::reshape{new_shape}, input);
    }

    for(auto&& new_shape : std::vector<std::vector<int64_t>>{{8, 3, 2, 2}, {1, 3, -1, -1}})
    {
        throws_shape(migraph::op::reshape{new_shape}, input);
    }
}

void flatten_shape()
{
    migraph::shape input{migraph::shape::float_type, {2, 4, 6, 8}};
    expect_shape(migraph::shape{migraph::shape::float_type, {1, 2 * 4 * 6 * 8}},
                 migraph::op::flatten{0},
                 input);
    expect_shape(
        migraph::shape{migraph::shape::float_type, {2, 4 * 6 * 8}}, migraph::op::flatten{1}, input);
    expect_shape(
        migraph::shape{migraph::shape::float_type, {2 * 4, 6 * 8}}, migraph::op::flatten{2}, input);
    expect_shape(
        migraph::shape{migraph::shape::float_type, {2 * 4 * 6, 8}}, migraph::op::flatten{3}, input);
    expect_shape(migraph::shape{migraph::shape::float_type, {2 * 4 * 6 * 8, 1}},
                 migraph::op::flatten{4},
                 input);
    throws_shape(migraph::op::flatten{5}, input);
}

void slice_shape()
{
    migraph::shape input{migraph::shape::int32_type, {2, 2, 3}};
    expect_shape(migraph::shape{migraph::shape::int32_type, {2, 2, 2}, {6, 3, 1}},
                 migraph::op::slice{{2}, {1}, {3}},
                 input);
    expect_shape(migraph::shape{migraph::shape::int32_type, {2, 2, 2}, {6, 3, 1}},
                 migraph::op::slice{{0, 1, 2}, {0, 0, 1}, {2, 2, 3}},
                 input);
    expect_shape(migraph::shape{migraph::shape::int32_type, {2, 2, 1}, {6, 3, 1}},
                 migraph::op::slice{{2}, {2}, {10}},
                 input);
}

void multibroadcast_shape()
{
    {
        std::vector<std::size_t> lens{4, 2, 5, 3};
        migraph::shape input{migraph::shape::float_type, {2, 1, 3}};
        expect_shape(migraph::shape{migraph::shape::float_type, lens, {0, 3, 0, 1}},
                     migraph::op::multibroadcast{lens},
                     input);
    }
    {
        std::vector<std::size_t> lens{4, 2, 5, 3};
        migraph::shape input{migraph::shape::float_type, {2, 1, 1}};
        expect_shape(migraph::shape{migraph::shape::float_type, lens, {0, 1, 0, 0}},
                     migraph::op::multibroadcast{lens},
                     input);
    }
    {
        std::vector<std::size_t> lens{4, 2, 5, 3};
        migraph::shape input{migraph::shape::float_type, {5, 1}};
        expect_shape(migraph::shape{migraph::shape::float_type, lens, {0, 0, 1, 0}},
                     migraph::op::multibroadcast{lens},
                     input);
    }
    {
        std::vector<std::size_t> lens{4, 2, 5, 3};
        migraph::shape input{migraph::shape::float_type, {4, 1, 1, 1}};
        expect_shape(migraph::shape{migraph::shape::float_type, lens, {1, 0, 0, 0}},
                     migraph::op::multibroadcast{lens},
                     input);
    }
    {
        std::vector<std::size_t> lens{4, 2, 5, 3};
        migraph::shape input{migraph::shape::float_type, {3}};
        expect_shape(migraph::shape{migraph::shape::float_type, lens, {0, 0, 0, 1}},
                     migraph::op::multibroadcast{lens},
                     input);
    }
    {
        std::vector<std::size_t> lens{4, 4, 1, 3};
        migraph::shape input{migraph::shape::float_type, {4, 1, 3}};
        expect_shape(migraph::shape{migraph::shape::float_type, lens, {0, 3, 3, 1}},
                     migraph::op::multibroadcast{lens},
                     input);
    }
    {
        std::vector<std::size_t> lens{4, 1, 1, 3};
        migraph::shape input{migraph::shape::float_type, {4, 1, 1, 1}};
        expect_shape(migraph::shape{migraph::shape::float_type, lens, {1, 1, 1, 0}},
                     migraph::op::multibroadcast{lens},
                     input);
    }
    {
        std::vector<std::size_t> lens{4, 1, 3};
        migraph::shape input{migraph::shape::float_type, {4, 1, 1, 1}};
        throws_shape(migraph::op::multibroadcast{lens}, input);
    }
    {
        std::vector<std::size_t> lens{4, 1, 3};
        migraph::shape input{migraph::shape::float_type, {}};
        throws_shape(migraph::op::multibroadcast{lens}, input);
    }
}

int main()
{
    multibroadcast_shape();
    batch_norm_inference_shape();
    convolution_shape();
    transpose_shape();
    contiguous_shape();
    reshape_shape();
    flatten_shape();
    slice_shape();
}
