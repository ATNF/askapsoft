/*
 * MathUtils.h
 *
 * @author Vitaliy Ogarko <vogarko@gmail.com>
 */

#ifndef SRC_UTILS_MATHUTILS_H_
#define SRC_UTILS_MATHUTILS_H_

#include <vector>

/*
 * Contains functions for some math operations.
 */
namespace askap { namespace lsqr { namespace MathUtils {

typedef std::vector<double> Vector;

/*
 * Calculates l2-norm of a vector.
 */
double GetNorm(const Vector& x);

/*
 * Calculates l2-norm squared of a vector.
 */
double GetNormSquared(const Vector& x);

#ifdef HAVE_MPI
/*
 * Returns l2-norm of a vector x that is split between CPUs.
 */
double GetNormParallel(const Vector& x, int nbproc, const MPI_Comm &mpi_comm);
#endif

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
 * Also returns the vector norm.
 */
bool Normalize(Vector& x, double& norm);
#ifdef HAVE_MPI
// For the case when x is split between CPUs.
bool NormalizeParallel(Vector& x, double& norm, int nbproc, const MPI_Comm &mpi_comm);
#endif

}}} // namespace askap.lsqr.MathUtils

#endif /* SRC_UTILS_MATHUTILS_H_ */
