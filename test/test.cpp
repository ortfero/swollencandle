#include <swollencandle/swollencandle.hpp>

#define DOCTEST_CONFIG_IMPLEMENT_WITH_MAIN
#include "doctest.h"


TEST_SUITE("swollencandle") {

    TEST_CASE("parse_upscale_period") {
        auto const maybe_hour = swollencandle::parse_upscale_period("hour");
        REQUIRE(maybe_hour);
        REQUIRE_EQ(*maybe_hour, swollencandle::upscale_period::hour);
        auto const maybe_day = swollencandle::parse_upscale_period("day");
        REQUIRE(maybe_day);
        REQUIRE_EQ(*maybe_day, swollencandle::upscale_period::day);
        auto const maybe_month = swollencandle::parse_upscale_period("month");
        REQUIRE(maybe_month);
        REQUIRE_EQ(*maybe_month, swollencandle::upscale_period::month);
        auto const maybe_year = swollencandle::parse_upscale_period("year");
        REQUIRE(maybe_year);
        REQUIRE_EQ(*maybe_year, swollencandle::upscale_period::year);
        auto const empty = swollencandle::parse_upscale_period("");
        REQUIRE(!empty);
        auto const unknown = swollencandle::parse_upscale_period("unknown");
        REQUIRE(!unknown);
    }

}

