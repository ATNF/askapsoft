/*
 * LSQRSolver.cpp
 *
 * @author Vitaliy Ogarko <vogarko@gmail.com>
 */
#include <iostream>
#include <stdexcept>
#include <cmath>

#include <lsqr_solver/LSQRSolver.h>
#include <lsqr_solver/MathUtils.h>

// Use this flag to suppress output.
#define SUPPRESS_OUTPUT

namespace askap { namespace lsqr {

LSQRSolver::LSQRSolver(size_t nlines, size_t nelements) :
    nlines(nlines),
    nelements(nelements),
    u(nlines),
    Hv(nlines),
    Hv_loc(nlines),
    v0(nelements),
    v(nelements),
    w(nelements)
{
}

void LSQRSolver::Solve(size_t niter,
        double rmin,
        const SparseMatrix& matrix,
        const Vector& b,
        Vector& x,
        int myrank)
{
#ifndef SUPPRESS_OUTPUT
    if (myrank == 0)
    {
        std::cout << "Entered LSQRSolver::Solve" << std::endl;
    }
#endif

    // Sanity check.
    if (matrix.GetNumberElements() == 0)
    {
        std::cout << "WARNING: Zero elements in the matrix. Exiting the solver." << std::endl;
        return;
    }

    // Sanity check.
    if (MathUtils::GetNormSquared(b) == 0.0)
    {
        std::cout << "WARNING: |b| = 0. Exiting the solver." << std::endl;
        return;
    }

    // Sanity check.
    if (b.size() != nlines)
    {
        throw std::invalid_argument("Wrong dimension of b in LSQRSolver::Solve!");
    }

    // Sanity check.
    if (x.size() != nelements)
    {
        throw std::invalid_argument("Wrong dimension of x in LSQRSolver::Solve!");
    }

    // Initialization.
    u = b;
    double alpha, beta;

    // Normalize u and initialize beta.
    if (!MathUtils::Normalize(u, beta, false))
    {
        throw std::runtime_error("PROBLEM: Could not normalize initial u, zero denominator!");
    }

    // Set x0 = 0, as required by the algorithm.
    std::fill(x.begin(), x.end(), 0.0);

    // b1 is initial residual as x0 = 0 by the algorithm.
    double b1 = beta;

    // Compute v = Ht.u.
    matrix.TransMultVector(u, v);

    // Normalize v and initialize alpha.
    if (!MathUtils::Normalize(v, alpha, true))
    {
        throw std::runtime_error("PROBLEM: Could not normalize initial v, zero denominator!");
    }

    double rhobar = alpha;
    double phibar = beta;
    w = v;

    size_t iter = 1;
    double r = 1.0;

    // Main loop.
    while (iter <= niter && r > rmin)
    {
        // Scale u: u = - alpha * u
        MathUtils::Multiply(u, - alpha);

        // Compute u = u + H.v parallel.
        #ifdef PARALLEL
            matrix.MultVector(v, Hv_loc);

            // TODO: Convert to C++.
            //call mpi_allreduce(Hv_loc, Hv, N_lines, CUSTOM_MPI_TYPE, MPI_SUM, MPI_COMM_WORLD, ierr)
        #else
            matrix.MultVector(v, Hv);
        #endif

        // u = u + Hv
        MathUtils::Add(u, Hv);

        // Normalize u and update beta.
        if (!MathUtils::Normalize(u, beta, false))
        {
            // Found an exact solution.
            std::cout << "WARNING: |u| = 0. Possibly found an exact solution in the LSQR solver!" << std::endl;
        }

        // Scale v: v = - beta * v
        MathUtils::Multiply(v, - beta);

        // Compute v = v + Ht.u
        matrix.TransMultVector(u, v0);

        // v = v + v0
        MathUtils::Add(v, v0);

        // Normalize v and update alpha.
        if (!MathUtils::Normalize(v, alpha, true))
        {
            // Found an exact solution.
            std::cout << "WARNING: |v| = 0. Possibly found an exact solution in the LSQR solver!" << std::endl;
        }

        // Compute scalars for updating the solution.
        double rho = sqrt(rhobar * rhobar + beta * beta);

        // Sanity check (avoid zero division).
        if (rho == 0.0)
        {
            std::cout << "WARNING: rho = 0. Exiting the loop." << std::endl;
            break;
        }

        // Compute scalars for updating the solution.
        double c       = rhobar / rho;
        double s       = beta / rho;
        double theta   = s * alpha;
        rhobar         = - c * alpha;
        double phi     = c * phibar;
        phibar         = s * phibar;
        double t1      = phi / rho;
        double t2      = - theta / rho;

        // Update the current solution x (w is an auxiliary array in order to compute the solution).
        // x = t1 * w + x
        MathUtils::Transform(1.0, x, t1, w);

        // w = t2 * w + v
        MathUtils::Transform(t2, w, 1.0, v);

        // Norm of the relative residual (analytical formulation).
        r = phibar / b1;

#ifndef SUPPRESS_OUTPUT
        // Printing log.
        if (iter % 10 == 0)
        {
            // Calculate the gradient: 2A'(Ax - b).
            #ifdef PARALLEL
                matrix.MultVector(x, Hv_loc);

                // TODO: Convert to C++.
                //call mpi_allreduce(Hv_loc, Hv, N_lines, CUSTOM_MPI_TYPE, MPI_SUM, MPI_COMM_WORLD, ierr)
            #else
                matrix.MultVector(x, Hv);
            #endif

            // Hv = Hv - b
            MathUtils::Transform(1.0, Hv, - 1.0, b);

            matrix.TransMultVector(Hv, v0);

            // Norm of the gradient.
            double g = 2.0 * MathUtils::GetNormParallel(v0);

            if (myrank == 0)
            {
                std::cout <<  "it, r, g =" << iter << ", " << r << ", " << g << std::endl;
            }
        }
#endif

        // To avoid floating point exception of denormal value.
        // Basically this is another stopping criterion.
        if (fabs(rhobar) < 1.e-30)
        {
            #ifndef SUPPRESS_OUTPUT
                std::cout << "WARNING: Small rhobar! Possibly algorithm is converged. Exiting the loop." << std::endl;
            #endif
            break;
        }

        iter += 1;
    }

    if (myrank == 0)
    {
        std::cout << "End of LSQRSolver::Solve, r =" << r << " iter =" << iter - 1 << std::endl;
    }
}

}} // namespace askap.lsqr
