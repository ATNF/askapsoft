/*
 * Point.h
 *
 * @author Vitaliy Ogarko <vogarko@gmail.com>
 */

#ifndef SRC_UTILS_POINT_H_
#define SRC_UTILS_POINT_H_

#include <iostream>

namespace askap { namespace lsqr {

// Type for storing a 3D point.
struct Point {

    // Position coordinates.
    double X, Y, Z;

    Point() :
        X(0.0),
        Y(0.0),
        Z(0.0)
    {
    }

    Point(double X, double Y, double Z) :
        X(X),
        Y(Y),
        Z(Z)
    {
    }

    inline bool operator==(const Point& rhs)
    {
        return (this->X == rhs.X && this->Y == rhs.Y && this->Z == rhs.Z);
    }

    friend std::ostream& operator<<(std::ostream& os, const Point& point)
    {
        os << point.X << ", " << point.Y << ", " << point.Z;
        return os;
    }
};

}} // namespace askap.lsqr

#endif /* SRC_UTILS_POINT_H_ */
