/*
 * ModelDamping.cpp
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

    size_t nelementsTotal = ParallelTools::get_total_number_elements(nelements, nbproc);

    // Extend matrix and right-hand size for adding damping.
    matrix.Extend(nelementsTotal, nelements);
    b.resize(b.size() + nelementsTotal);

    // Adding damping to the system.
    for (size_t i = 0; i < nelementsTotal; ++i)
    {
        // Default values for when pointers are null.
        double dampingWeightValue = 1.0;
        double modelValue = 0.0;
        double modelRefValue = 0.0;

        // Extract values.
        if (dampingWeight != NULL) dampingWeightValue = dampingWeight->at(i);
        if (model != NULL) modelValue = model->at(i);
        if (modelRef != NULL) modelRefValue = modelRef->at(i);

        //-------------------------------------------------------------
        // Add matrix lines with damping.
        //-------------------------------------------------------------
        matrix.NewRow();

        double normMultiplier = GetNormMultiplier(modelValue, modelRefValue, normPower);
        double matrixValue = alpha * dampingWeightValue * normMultiplier;

        matrix.Add(matrixValue, i);

        //----------------------------------------------------------------
        // Add corresponding damping contribution to the right-hand side.
        //----------------------------------------------------------------
        double rhsValue = - matrixValue * (modelValue - modelRefValue);

        size_t index = matrix.GetCurrentNumberRows();

        b[index - 1] = rhsValue;
    }

    // Finalize matrix.
    matrix.Finalize(nelements);
}

double ModelDamping::GetNormMultiplier(double model, double modelRef, double normPower) const
{
    if (normPower == 2.0) return 1.0;
    if (model == modelRef) return 1.0;

    return pow(fabs(model - modelRef), normPower / 2.0 - 1.0);
}

}} // namespace askap.lsqr

