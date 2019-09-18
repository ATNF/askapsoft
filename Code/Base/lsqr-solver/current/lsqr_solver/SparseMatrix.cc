/*
 * SparseMatrix.cpp
 *
 * @author Vitaliy Ogarko <vogarko@gmail.com>
 */

#include <stdexcept>
#include <cassert>

#include <lsqr_solver/SparseMatrix.h>

namespace askap { namespace lsqr {

SparseMatrix::SparseMatrix(size_t nl) :
    finalized(false),
    nel(0),
    nl(nl),
    nl_current(0),
    sa(),
    ija(),
    ijl(nl + 1)
#ifdef HAVE_MPI
    ,itsComm(MPI_COMM_NULL)
#endif
{
}
#ifdef HAVE_MPI
SparseMatrix::SparseMatrix(size_t nl, const MPI_Comm &comm) :
    finalized(false),
    nel(0),
    nl(nl),
    nl_current(0),
    sa(),
    ija(),
    ijl(nl + 1)
{
    assert(comm != MPI_COMM_NULL);
    MPI_Comm_dup(comm, &itsComm);
}
#endif

SparseMatrix::~SparseMatrix()
{
#ifdef HAVE_MPI
    if (itsComm != MPI_COMM_NULL) {
        MPI_Comm_free(&itsComm);
    }
#endif
}

bool SparseMatrix::Finalize(size_t ncolumns)
{
    // Sanity check.
    if (nl_current != nl) {
        throw std::runtime_error("Wrong total number of rows in SparseMatrix::Finalize!");
    }

    // Store index of the last element.
    ijl[nl] = nel;

    if (!ValidateIndexBoundaries(ncolumns)) {
        throw std::runtime_error("Sparse matrix validation failed!");
    }

    finalized = true;

    return true;
}

void SparseMatrix::Add(double value, size_t column)
{
    // Sanity check.
    if (finalized) {
        throw std::runtime_error("Matrix has already been finalized in SparseMatrix::Add!");
    }

    // Do not add zero values to a sparse matrix.
    if (value == 0.) return;

    // Sanity check for the number of lines.
    if (nl_current == 0) {
        throw std::runtime_error("Error in the number of lines in SparseMatrix::Add!");
    }

    nel += 1;
    sa.push_back(value);
    ija.push_back(column);
}

void SparseMatrix::NewRow()
{
    // Sanity check.
    if (finalized) {
        throw std::runtime_error("Matrix has already been finalized in SparseMatrix::NewRow!");
    }

    // Sanity check.
    if (nl_current >= nl) {
        throw std::runtime_error("Error in number of rows in SparseMatrix::NewRow!");
    }

    nl_current += 1;
    ijl[nl_current - 1] = nel;
}

double SparseMatrix::GetValue(size_t i, size_t j) const
{
    // Sanity check.
    if (!finalized) {
        throw std::runtime_error("Matrix has not been finalized yet in SparseMatrix::GetValue!");
    }

    double res = 0.0;
    for (size_t k = ijl[j]; k < ijl[j + 1]; k++) {
        if (ija[k] == i) {
        // Found a non-zero element at the column i.
            res = sa[k];
            break;
        }
    }
    return res;
}

void SparseMatrix::Reset()
{
    finalized = false;

    nl_current = 0;
    nel = 0;

    std::fill(sa.begin(), sa.end(), 0.);
    std::fill(ija.begin(), ija.end(), 0);
    std::fill(ijl.begin(), ijl.end(), 0);
}

void SparseMatrix::MultVector(const Vector& x, Vector& b) const
{
    // Sanity check.
    if (!finalized) {
        throw std::runtime_error("Matrix has not been finalized yet in SparseMatrix::MultVector!");
    }

    // Set all elements to zero.
    std::fill(b.begin(), b.end(), 0.);

    for (size_t i = 0; i < nl; i++) {
        for (size_t k = ijl[i]; k < ijl[i + 1]; k++) {
            b[i] += sa[k] * x[ija[k]];
        }
    }
}

void SparseMatrix::TransMultVector(const Vector& x, Vector& b) const
{
    // Sanity check.
    if (!finalized) {
        throw std::runtime_error("Matrix has not been finalized yet in SparseMatrix::TransMultVector!");
    }

    // Set all elements to zero.
    std::fill(b.begin(), b.end(), 0.);

    for (size_t i = 0; i < nl; i++) {
// Compiler directive to vectorize the following loop.
//#pragma ivdep       // For Intel compiler.
#pragma GCC ivdep   // For GCC compiler.

        for (size_t k = ijl[i]; k < ijl[i + 1]; k++) {
            size_t j = ija[k];
            b[j] += sa[k] * x[i];
        }
    }
}

void SparseMatrix::Extend(size_t extra_nl, size_t extra_nnz)
{
    // Sanity check.
    if (!finalized) {
        throw std::runtime_error("Matrix has not been finalized yet in SparseMatrix::Extend!");
    }

    finalized = false;

    // Reset the last element index.
    ijl[nl] = 0;

    nl += extra_nl;
    ijl.resize(nl + 1);
}

size_t SparseMatrix::GetNumberNonemptyRows() const
{
    // Sanity check.
    if (!finalized) {
        throw std::runtime_error("Matrix has not been finalized yet in SparseMatrix::GetNumberNonemptyRows!");
    }

    size_t number_empty_rows = 0;
    for (size_t i = 0; i < nl; i++) {
        if (ijl[i] == ijl[i + 1]) number_empty_rows++;
    }
    return (nl - number_empty_rows);
}

bool SparseMatrix::ValidateIndexBoundaries(size_t ncolumns)
{
    // Use the same loop as in A'x multiplication.
    for (size_t i = 0; i < nl; i++) {
        for (size_t k = ijl[i]; k < ijl[i + 1]; k++) {
            if (k > nel) {
                throw std::runtime_error("Sparse matrix validation failed for k-index!");
            }

            size_t j = ija[k];

            if (j > ncolumns - 1) {
                throw std::runtime_error("Sparse matrix validation failed for j-index!");
            }
        }
    }
    return true;
}

#ifdef HAVE_MPI
const MPI_Comm& SparseMatrix::GetComm() const
{
    return itsComm;
}
#endif

}} // namespace askap.lsqr

