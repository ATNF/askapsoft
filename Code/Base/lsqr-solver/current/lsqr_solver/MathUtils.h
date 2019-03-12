/*
 * MathUtils.h
 *
 * @author Vitaliy Ogarko <vogarko@gmail.com>
 */

#ifndef SRC_UTILS_MATHUTILS_H_
#define SRC_UTILS_MATHUTILS_H_

#include <lsqr_solver/GlobalTypedefs.h>

/*
 * Contains functions for some math operations.
 */
namespace askap { namespace lsqr { namespace MathUtils {

/*
 * Calculates l2-norm of a vector.
 */
double GetNorm(const Vector& x);

/*
 * Calculates l2-norm squared of a vector.
 */
double GetNormSquared(const Vector& x);

/*
 * Returns l2-norm of a vector x that is split between CPUs.
 */
double GetNormParallel(const Vector& x, int nbproc);

/*
 * Multiplies vector by a scalar, and stores the result in the same vector.
 */
void Multiply(Vector& x, double s);

/*
 * Sums two vectors and stores the result in the first vector.
 */
void Add(Vector& x, const Vector& y);

/*
 * Calculates a * x + b * y, and stores result in x,
 */
void Transform(double a, Vector& x, double b, const Vector& y);

/*
 * Normalizes vector x by its l2-norm.
 * inParallel - flag that defines if x is split between CPUs.
 * Also returns the vector norm.
 */
bool Normalize(Vector& x, double& norm, bool inParallel, int nbproc);

}}} // namespace askap.lsqr.MathUtils

#endif /* SRC_UTILS_MATHUTILS_H_ */
