// Minimal regression test for circle geometry helpers.
// Verifies the diameter, circumference, and area formulas used by the CAD math layer.
#include <cmath>
#include <cstdlib>
#include <numbers>

#include "../src/cad/Geometry.h"

int main()
{
    using namespace GForceCAD;

    const double radius = 5.0;
    const double expectedDiameter = 10.0;
    const double expectedCircumference = 2.0 * std::numbers::pi * radius;
    const double expectedArea = std::numbers::pi * radius * radius;

    if (std::abs(circleDiameter(radius) - expectedDiameter) > 1e-9) return 1;
    if (std::abs(circleCircumference(radius) - expectedCircumference) > 1e-9) return 2;
    if (std::abs(circleArea(radius) - expectedArea) > 1e-9) return 3;

    return 0;
}
