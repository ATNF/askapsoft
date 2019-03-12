/*
 * MathUtils.cc
 *
 * @author Vitaliy Ogarko <vogarko@gmail.com>
 */

// MPI-specific includes
#ifdef HAVE_MPI
#include <mpi.h>
#endif

#include <cmath>
#include <cassert>
#include <stdexcept>

#include <lsqr_solver/MathUtils.h>

namespace askap { namespace lsqr { namespace MathUtils {

double GetNorm(const Vector& x)
{
    double accum = 0.;
    for (size_t i = 0; i < x.size(); ++i)
    {
        accum += x[i] * x[i];
    }
    return sqrt(accum);
}

double GetNormSquared(const Vector& x)
{
    double accum = 0.;
    for (size_t i = 0; i < x.size(); ++i)
    {
        accum += x[i] * x[i];
    }
    return accum;
}

double GetNormParallel(const Vector& x, int nbproc)
{
#ifdef HAVE_MPI
    if (nbproc > 1)
    {
        double s_loc = GetNormSquared(x);
        double s_glob = 0.;

        MPI_Allreduce(&s_loc, &s_glob, 1, MPI_DOUBLE, MPI_SUM, MPI_COMM_WORLD);

        double norm = sqrt(s_glob);
        return norm;
    }
    else
    {
        return GetNorm(x);
    }
#else
    assert(nbproc == 1);
    return GetNorm(x);
#endif
}

void Multiply(Vector& x, double s)
{
    for (size_t i = 0; i < x.size(); ++i)
    {
        x[i] *= s;
    }
}

void Add(Vector& x, const Vector& y)
{
    // Sanity check.
    if (x.size() != y.size())
    {
        throw std::runtime_error("Dimensions of vectors do not match in MathUtils::Add!");
    }

    for (size_t i = 0; i < x.size(); ++i)
    {
        x[i] += y[i];
    }
}

void Transform(double a, Vector& x, double b, const Vector& y)
{
    // Sanity check.
    if (x.size() != y.size())
    {
        throw std::runtime_error("Dimensions of vectors do not match in MathUtils::Transform!");
    }

    for (size_t i = 0; i < x.size(); ++i)
    {
        x[i] = a * x[i] + b * y[i];
    }
}

bool Normalize(Vector& x, double& norm, bool inParallel, int nbproc)
{
    if (inParallel)
    {
        norm = GetNormParallel(x, nbproc);
    }
    else
    {
        norm = GetNorm(x);
    }

    double inverseNorm;

    if (norm != 0.0)
    {
        inverseNorm = 1.0 / norm;
    }
    else
    {
        return false;
    }

    Multiply(x, inverseNorm);

    return true;
}

}}} // namespace askap.lsqr.MathUtils

