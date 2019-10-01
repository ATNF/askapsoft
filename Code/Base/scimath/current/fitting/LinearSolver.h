/// @file
///
/// LinearSolver: This solver uses SVD or LSQR to solve the normal equations.
///
/// @copyright (c) 2007 CSIRO
/// Australia Telescope National Facility (ATNF)
/// Commonwealth Scientific and Industrial Research Organisation (CSIRO)
/// PO Box 76, Epping NSW 1710, Australia
/// atnf-enquiries@csiro.au
///
/// This file is part of the ASKAP software distribution.
///
/// The ASKAP software distribution is free software: you can redistribute it
/// and/or modify it under the terms of the GNU General Public License as
/// published by the Free Software Foundation; either version 2 of the License,
/// or (at your option) any later version.
///
/// This program is distributed in the hope that it will be useful,
/// but WITHOUT ANY WARRANTY; without even the implied warranty of
/// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
/// GNU General Public License for more details.
///
/// You should have received a copy of the GNU General Public License
/// along with this program; if not, write to the Free Software
/// Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307 USA
///
/// @author Tim Cornwell <tim.cornwell@csiro.au>
/// @author Vitaliy Ogarko <vogarko@gmail.com>
///
#ifndef SCIMATHLINEARSOLVER_H_
#define SCIMATHLINEARSOLVER_H_

#ifdef HAVE_MPI
#include <mpi.h>
#endif

#include <fitting/Solver.h>
#include <fitting/INormalEquations.h>
#include <fitting/DesignMatrix.h>
#include <fitting/Params.h>

#include <lsqr_solver/SparseMatrix.h>

#include <boost/config.hpp>

#include <utility>
#include <vector>

namespace askap
{
  namespace scimath
  {
    /// Solve the normal equations for updates to the parameters
    /// @ingroup fitting
    class LinearSolver : public Solver
    {
      public:
       /// @brief no limit on the condition number
       static BOOST_CONSTEXPR_OR_CONST double KeepAllSingularValues = -1.;

       /// @brief Constructor
       /// @details Optionally, it is possible to limit the condition number of
       /// normal equation matrix to a given number.
       /// @param maxCondNumber maximum allowed condition number of the range
       /// of the normal equation matrix for the SVD algorithm. Effectively this
       /// puts the limit on the singular values, which are considered to be
       /// non-zero (all greater than the largest singular value divided by this
       /// condition number threshold). Default is 1e3. Put a negative number
       /// if you don't want to drop any singular values (may be a not very wise
       /// thing to do!). A very large threshold has the same effect. Zero
       /// threshold is not allowed and will cause an exception.
       explicit LinearSolver(double maxCondNumber = 1e3);

       ~LinearSolver();

        /// Initialize this solver
        virtual void init();

        /// @brief solve for parameters
        /// The solution is constructed from the normal equations and given
        /// parameters are updated. If there are no free parameters in the
        /// given Params class, all unknowns in the normal
        /// equatons will be solved for.
        /// @param[in] params parameters to be updated 
        /// @param[in] quality Quality of solution
        virtual bool solveNormalEquations(Params &params, Quality& quality);

        /// @brief Clone this object
        virtual Solver::ShPtr clone() const;

        // @brief Setter for the major loop iteration number.
        virtual void setMajorLoopIterationNumber(size_t it);

#ifdef HAVE_MPI
        // @brief Setter for workers communicator.
        virtual void setWorkersCommunicator(const MPI_Comm &comm);
#endif

       protected:

        /// @brief solve for a subset of parameters
        /// @details This method is used in solveNormalEquations
        /// @param[in] params parameters to be updated           
        /// @param[in] quality Quality of the solution
        /// @param[in] names names for parameters to solve for
        /// @return pair of minimum and maximum eigenvalues
        std::pair<double,double> solveSubsetOfNormalEquations(Params &params, Quality& quality,
                   const std::vector<std::string> &__names) const;

        /// @brief solve for a subset of parameters using the LSQR solver.
        std::pair<double,double> solveSubsetOfNormalEquationsLSQR(Params &params, Quality& quality,
                   const std::vector<std::string> &__names) const;

        /// @brief extract an independent subset of parameters
        /// @details This method analyses the normal equations and forms a subset of 
        /// parameters which can be solved for independently. Although the SVD is more than
        /// capable of dealing with degeneracies, it is often too slow if the number of parameters is large.
        /// This method essentially gives the solver a hint based on the structure of the equations
        /// @param[in] names names for parameters to choose from
        /// @param[in] tolerance tolerance on the matrix elements to decide whether they can be considered independent
        /// @return names of parameters in this subset
        std::vector<std::string> getIndependentSubset(std::vector<std::string> &names, const double tolerance) const;

        /// @brief test that all matrix elements are below tolerance by absolute value
        /// @details This is a helper method to test all matrix elements
        /// @param[in] matr matrix to test
        /// @param[in] tolerance tolerance on the element absolute values
        /// @return true if all elements are zero within the tolerance
        static bool allMatrixElementsAreZeros(const casa::Matrix<double> &matr, const double tolerance);

       private:
         /// @brief maximum condition number allowed
         /// @details Effectively, this is a threshold for singular values 
         /// taken into account in the svd method
         double itsMaxCondNumber;

         // Iteration number in the major loop (for LSQR solver with constraints).
         size_t itsMajorLoopIterationNumber;

         // NOTE: Copied from "calibaccess/CalParamNameHelper.h", as currently accessors depends of scimath.
         /// @brief extract coded channel and parameter name
         /// @details This is a reverse operation to codeInChannel. Note, no checks are done that the name passed
         /// has coded channel present.
         /// @param[in] name full name of the parameter
         /// @return a pair with extracted channel and the base parameter name
         static std::pair<casa::uInt, std::string> extractChannelInfo(const std::string &name);

         static bool compareGainNames(const std::string& gainA, const std::string& gainB);

         /// @brief Calculates the smoothing weight for the current major loop iteration.
         double getSmoothingWeight() const;

#ifdef HAVE_MPI
         // MPI communicator of all workers (for LSQR solver).
         MPI_Comm itsWorkersComm;
#endif
    };

    /// @brief Returns a current solution vector of doubles.
    /// @param[in] indices List of gain name/index pairs (note two parameters per gain - real & imaginary part).
    /// @param[in] params Normal equation parameters.
    /// @param[in] solution A container where the solution will be returned.
    void getCurrentSolutionVector(const std::vector<std::pair<std::string, int> >& indices,
                                  const Params& params,
                                  std::vector<double>& solution);

    /// @brief Adding smoothness constraints to the system of equations.
    /// @details Extends the matrix and right-hand side with smoothness constraints,
    /// which are defined for the least squares minimization framework.
    /// @param[in] matrix The matrix where constraints will be added.
    /// @param[in] b_RHS The right-hand side where constraints will be added.
    /// @param[in] indices List of gain name/index pairs (note two parameters per gain - real & imaginary part).
    /// @param[in] x0 The current global solution (at all workers).
    /// @param[in] nParameters Local number of parameters (at the current worker).
    /// @param[in] nChannels The total number of channels.
    /// @param[in] smoothingWeight The smoothing weight.
    /// @param[in] gradientType The type of gradient approximation (0 - forward difference, 1- central difference).
    void addSmoothnessConstraints(lsqr::SparseMatrix& matrix, lsqr::Vector& b_RHS,
                                  const std::vector<std::pair<std::string, int> >& indices,
                                  const std::vector<double>& x0,
                                  size_t nParameters,
                                  size_t nChannels,
                                  double smoothingWeight,
                                  int gradientType = 0);

  }
}
#endif
