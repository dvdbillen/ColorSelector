#ifndef PREFS_H
#define PREFS_H

#include <QColor>

/*
   S = a source value
   D = a destination value

   We have a conventional linear interpolation function:
      double interpolate(double v1, double v2, double amount) { return v1*(1-amount) + v2*amout; }

   We know the values of A and x and we know that A = interpolate(S, D, x)
   We know the values of B and y and we know that B = interpolate(A, D, y)

   With that knowledge, how can we derive a value for z such that interpolate(S, D, z) = B?



double interpolate(double v1, double v2, double amount) {
    return v1 * (1.0 - amount) + v2 * amount;
}

double find_z(double S, double D, double x, double A, double y, double B) {
    // Check if S equals D to avoid division by zero or undefined behavior
    if (S == D) {
        return 0.0; // In this case, A = B = S = D, so z can be any value; return 0 for simplicity
    }

    // Calculate z using the derived formula: z = x + y - x*y
    double z = x + y - x * y;
    return z;
}
*/

class demo_prefs {
    double corner_rounding_ = 0;
    bool custom_ = false;
    demo_prefs();
public:

    bool custom() const {
        return custom_;
    }

    double corner_rounding() const {
        return corner_rounding_;
    }

    static const demo_prefs& get() {
        static demo_prefs the_;
        return the_;
    }
};

#endif // PREFS_H
