/*
 * ModelDamping.h
 *
 * @author Vitaliy Ogarko <vogarko@gmail.com>
 */

#ifndef SRC_INVERSION_MODELDAMPING_H_
#define SRC_INVERSION_MODELDAMPING_H_

#include <lsqr_solver/SparseMatrix.h>

namespace askap { namespace lsqr {

/*
 * Class for adding into inversion Lp-norm damping term applied on the model:
 * This changes the cost function F as: F_new = F + alpha^2 {|| W (m - m_ref) ||_p}^p.
  */
class ModelDamping {
public:
    /*
     * Constructor.
     * nelements - number of model elements.
     *
     */
    ModelDamping(size_t nelements);

    /*
     * Add damping term to the matrix system.
     *
     * alpha - weight of the damping term.
     * normPower - norm power used in the Lp norm.
     * model - model obtained from previous iteration (zero if null).
     * modelRef - reference model (zero if null).
     * dampingWeight - damping weight diagonal matrix (identity matrix if null).
     *
     * For example, for normPower = 2, the new system becomes:
     *
     *     A_new = (       A )
     *             ( alpha W ),
     *     where W is a diagonal damping matrix (damping weight).
     *
     *     b_new = (            b            )
     *             ( - alpha W (m_n - m_ref) )
     */
    void Add(double alpha,
             double normPower,
             SparseMatrix& matrix,
             Vector& b,
             const Vector* model,
             const Vector* modelRef,
             const Vector* dampingWeight);

    virtual ~ModelDamping() {};

private:
    size_t nelements;

    /*
     * Returns a multiplier (for one pixel) to change L2 norm to Lp, in the LSQR method.
     */
    double GetNormMultiplier(double model, double modelRef, double normPower) const;
};

}} // namespace askap.lsqr

#endif /* SRC_INVERSION_MODELDAMPING_H_ */
