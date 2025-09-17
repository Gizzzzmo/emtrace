#include "test_utils.h"
#include <emtrace/emtrace.h>
#include <float.h>
#include <math.h>

EXPECT_OUTPUT(
    "Doubles: 3.14159 2.71828 nan inf -inf\n"
    // "3.141590 2.718280 nan inf -inf\n"
    "Extreme values: 1.7976931348623157e+308 2.2250738585072014e-308\n"
);

int main(void) {
    EMTRACE_INIT();

    // Test basic double values
    double pi = 3.14159;
    double e = 2.71828;
    double nan_val = NAN;
    double inf_val = INFINITY;
    double neg_inf = -INFINITY;

    EMTRACELN_F(
        "Doubles: {} {} {} {} {}", double, pi, double, e, double, nan_val, double, inf_val, double,
        neg_inf
    );

    // TODO: // Test C-style formatting
    // EMTRACELN_F(
    //     "%f %f %f %f %f", double, pi, double, e, double, nan_val, double, inf_val, double,
    //     neg_inf
    // );
    //
    // Test extreme values
    double max_val = DBL_MAX; // 1.7976931348623157e+308
    double min_val = DBL_MIN; // 2.2250738585072014e-308

    EMTRACELN_F("Extreme values: {} {}", double, max_val, double, min_val);

    return 0;
}
