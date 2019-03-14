/*
 * ModelDamping.cc
 *
 * @author Vitaliy Ogarko <vogarko@gmail.com>
 */

#include <stdexcept>
#include <cmath>

#include <lsqr_solver/ModelDamping.h>
#include <lsqr_solver/ParallelTools.h>

namespace askap { namespace lsqr {

ModelDamping::ModelDamping(size_t nelements) :
    nelements(nelements)
{
}

void ModelDamping::Add(double alpha,
                       double normPower,
                       SparseMatrix& matrix,
                       Vector& b,
                       const Vector* model,
                       const Vector* modelRef,
                       const Vector* dampingWeight,
                       int myrank,
                       int nbproc)
{
    // Sanity check.
    if ((model != NULL && model->size() != nelements)
        || (dampingWeight != NULL && dampingWeight->size() != nelements)
        || (modelRef != NULL && modelRef->size() != nelements))
    {
        throw std::invalid_argument("Wrong vector size in ModelDamping::Add!");
    }

    // Sanity check.
    if (!matrix.Finalized())
    {
        throw std::runtime_error("Matrix has not been finalized yet in ModelDamping::Add!");
    }

    // Sanity check.
    if (nbproc > 1 && matrix.GetComm() == NULL)
    {
        throw std::invalid_argument("MPI communicator not defined in ModelDamping::Add!");
    }

    size_t nelementsTotal = ParallelTools::get_total_number_elements(nelements, nbproc, matrix.GetComm());
    size_t nsmaller = ParallelTools::get_nsmaller(nelements, myrank, nbproc, matrix.GetComm());

    // Extend matrix and right-hand size for adding damping.
    matrix.Extend(nelementsTotal, nelements);
    b.resize(b.size() + nelementsTotal);

    // Gather here local parts of the right-hand-side from all CPUs.
    Vector b_loc(nelementsTotal);

    // Adding damping to the system.
    for (size_t i = 0; i < nelementsTotal; ++i)
    {
        if (i >= nsmaller && i < nsmaller + nelements)
        {
            // Matrix column in local matrix (on current CPU).
            size_t column = i - nsmaller;

            // Default values for when pointers are null.
            double dampingWeightValue = 1.0;
            double modelValue = 0.0;
            double modelRefValue = 0.0;

            // Extract values.
            if (dampingWeight != NULL) dampingWeightValue = dampingWeight->at(column);
            if (model != NULL) modelValue = model->at(column);
            if (modelRef != NULL) modelRefValue = modelRef->at(column);

            //-------------------------------------------------------------
            // Add matrix lines with damping.
            //-------------------------------------------------------------
            matrix.NewRow();

            double normMultiplier = GetNormMultiplier(modelValue, modelRefValue, normPower);
            double matrixValue = alpha * dampingWeightValue * normMultiplier;

            matrix.Add(matrixValue, column);

            //----------------------------------------------------------------
            // Add corresponding damping contribution to the right-hand side.
            //----------------------------------------------------------------
            double rhsValue = - matrixValue * (modelValue - modelRefValue);

            b_loc[column] = rhsValue;
        }
        else
        {
            // Adding an empty line.
            matrix.NewRow();
        }
    }

    // Finalize matrix.
    matrix.Finalize(nelements);

    //------------------------------------------------------------------------------
    // Set the full right-hand side.
    //------------------------------------------------------------------------------
    ParallelTools::get_full_array_in_place(nelements, b_loc, true, myrank, nbproc, matrix.GetComm());

    for (size_t i = 0; i < nelementsTotal; ++i)
    {
        size_t index = b.size() - nelementsTotal + i;
        b[index] = b_loc[i];
    }
}

double ModelDamping::GetNormMultiplier(double model, double modelRef, double normPower) const
{
    if (normPower == 2.0) return 1.0;
    if (model == modelRef) return 1.0;

    return pow(fabs(model - modelRef), normPower / 2.0 - 1.0);
}

}} // namespace askap.lsqr

