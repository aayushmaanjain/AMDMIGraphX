#include <migraphx/register_target.hpp>
#include <migraphx/ref/target.hpp>
#include <migraphx/target.hpp>
#include "test.hpp"

TEST_CASE(make_target)
{
    for(const auto& name : migraphx::get_targets())
    {
        auto t = migraphx::make_target(name);
        CHECK(t.name() == name);
    }
}

TEST_CASE(make_invalid_target)
{
    EXPECT(test::throws([&] { migraphx::make_target("mi100"); }));
}

TEST_CASE(targets)
{
    auto ts = migraphx::get_targets();
    EXPECT(ts.size() > 0);
}

int main(int argc, const char* argv[]) { test::run(argc, argv); }
