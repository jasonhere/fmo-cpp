#include "../catch/catch.hpp"
#include "test-data.hpp"
#include "test-tools.hpp"

SCENARIO("performing per-pixel operations", "[image][processing]") {
    GIVEN("an empty destination image") {
        fmo::Image dst{ };
        WHEN("source image is BGR") {
            fmo::Image src{fmo::Format::BGR, IM_4x2_DIMS, IM_4x2_BGR.data()};
            THEN("calling less_than() throws") { REQUIRE_THROWS(fmo::less_than(src, dst, 0x95)); }
            THEN("calling greater_than() throws") {
                REQUIRE_THROWS(fmo::greater_than(src, dst, 0x95));
            }
        }
        GIVEN("a GRAY source image (4x2)") {
            fmo::Image src{fmo::Format::GRAY, IM_4x2_DIMS, IM_4x2_GRAY.data()};
            WHEN("less_than() is called") {
                fmo::less_than(src, dst, 0x95);
                THEN("result is as expected") {
                    REQUIRE(dst.dims() == src.dims());
                    REQUIRE(dst.format() == fmo::Format::GRAY);
                    REQUIRE(exact_match(dst, IM_4x2_LESS_THAN));
                }
            }
            WHEN("greater_than() is called") {
                fmo::greater_than(src, dst, 0x95);
                THEN("result is as expected") {
                    REQUIRE(dst.dims() == src.dims());
                    REQUIRE(dst.format() == fmo::Format::GRAY);
                    REQUIRE(exact_match(dst, IM_4x2_GREATER_THAN));
                }
            }
            GIVEN("a second GRAY source image (4x2_LESS_THAN)") {
                fmo::Image src2{fmo::Format::GRAY, IM_4x2_DIMS, IM_4x2_LESS_THAN.data()};
                WHEN("absdiff() is called") {
                    fmo::absdiff(src, src2, dst);
                    THEN("result is as expected") {
                        REQUIRE(dst.dims() == src.dims());
                        REQUIRE(dst.format() == src.format());
                        REQUIRE(exact_match(dst, IM_4x2_ABSDIFF));
                    }
                }
            }
        }
    }
}

SCENARIO("performing complex operations", "[image][processing]") {
    GIVEN("an empty destination image") {
        fmo::Image dst{ };
        GIVEN("a GRAY source image") {
            fmo::Image src{fmo::Format::GRAY, IM_4x2_DIMS, IM_4x2_GRAY.data()};
            WHEN("decimate() is called") {
                fmo::decimate(src, dst);
                THEN("result is as expected") {
                    REQUIRE(dst.format() == src.format());
                    REQUIRE((dst.dims() == fmo::Dims{2, 1}));
                    std::array<uint8_t, 2> expected = {{0x7F, 0x7F}};
                    REQUIRE(exact_match(dst, expected));
                }
            }
        }
    }
}
