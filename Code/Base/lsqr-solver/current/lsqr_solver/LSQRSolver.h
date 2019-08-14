/*
 * LSQRSolver.h
 *
 * @author Vitaliy Ogarko <vogarko@gmail.com>
 */

#ifndef LSQRSOLVER_H_
#define LSQRSOLVER_H_

#include <lsqr_solver/GlobalTypedefs.h>
#include <lsqr_solver/SparseMatrix.h>

namespace askap { namespace lsqr {

/*
 * Least Square (LSQR) solver parallelized by the model parameters.
 * Solves min||Ax - b|| in L2 norm, where A is stored in sparse format.
 */
class LSQRSolver {
public:
    /*
     * Constructor.
     * nlines - the number of matrix lines (rows).
     * nelements - local (at current CPU) number of model parameters (the number of matrix columns).
     */
    LSQRSolver(size_t nlines, size_t nelements);

    /*
     * Solves min||Ax - b|| in L2 norm.
     * Niter - maximum number of iterations.
     * rmin - stopping criterion (relative residual).
     * suppress_output - flag for printing solver logs.
     */
    void Solve(size_t niter,
            double rmin,
            const SparseMatrix& matrix,
            const Vector& b,
            Vector& x,
            bool suppress_output = true);

    virtual ~LSQRSolver() {};

private:
    // The number of matrix lines (rows).
    size_t nlines;
    // Local (at current CPU) number of model parameters (the number of matrix columns).
    size_t nelements;

    // Auxiliary data arrays needed for calculations.
    Vector u;
    Vector Hv;
    Vector Hv_loc;
    Vector v0;
    Vector v;
    Vector w;
};

}} // namespace askap.lsqr

#endif /* LSQRSOLVER_H_ */
