/// @file
///
/// A linear solver for parameters from the normal equations
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
#include <fitting/LinearSolver.h>
#include <fitting/GenericNormalEquations.h>

#include <askap/AskapError.h>
#include <profile/AskapProfiler.h>
#include <boost/config.hpp>
#include <algorithm>

#include <casacore/casa/aips.h>
#include <casacore/casa/Arrays/Array.h>
#include <casacore/casa/Arrays/Matrix.h>
#include <casacore/casa/Arrays/Vector.h>
#include <casacore/casa/OS/Timer.h>

#include <gsl/gsl_matrix.h>
#include <gsl/gsl_vector.h>
#include <gsl/gsl_linalg.h>

#include <lsqr_solver/LSQRSolver.h>
#include <lsqr_solver/ModelDamping.h>
#include <lsqr_solver/ParallelTools.h>

#include <askap/AskapLogging.h>
ASKAP_LOGGER(logger, ".linearsolver");

#include <iostream>

#include <string>
#include <map>

#include <cmath>
using std::abs;
using std::map;
using std::string;

namespace askap
{
  namespace scimath
  {
    BOOST_CONSTEXPR_OR_CONST double LinearSolver::KeepAllSingularValues;

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
LinearSolver::LinearSolver(double maxCondNumber) :
       itsMaxCondNumber(maxCondNumber),
       itsMajorLoopIterationNumber(0)
#ifdef HAVE_MPI
       ,itsWorkersComm(MPI_COMM_NULL)
#endif
{
    ASKAPASSERT(itsMaxCondNumber != 0);
};

LinearSolver::~LinearSolver()
{
#ifdef HAVE_MPI
    if (itsWorkersComm != MPI_COMM_NULL) {
        MPI_Comm_free(&itsWorkersComm);
    }
#endif
}

void LinearSolver::init()
{
    resetNormalEquations();
}

/// @brief test that all matrix elements are below tolerance by absolute value
/// @details This is a helper method to test all matrix elements
/// @param[in] matr matrix to test
/// @param[in] tolerance tolerance on the element absolute values
/// @return true if all elements are zero within the tolerance
bool LinearSolver::allMatrixElementsAreZeros(const casa::Matrix<double> &matr, const double tolerance)
{
  for (casa::uInt row = 0; row < matr.nrow(); ++row) {
       for (casa::uInt col = 0; col < matr.ncolumn(); ++col) {
            if (abs(matr(row,col)) > tolerance) {
                return false;
            }
       }
  }
  return true;
}

/// @brief extract an independent subset of parameters
/// @details This method analyses the normal equations and forms a subset of 
/// parameters which can be solved for independently. Although the SVD is more than
/// capable of dealing with degeneracies, it is often too slow if the number of parameters is large.
/// This method essentially gives the solver a hint based on the structure of the equations
/// @param[in] names names for parameters to choose from
/// @param[in] tolerance tolerance on the matrix elements to decide whether they can be considered independent
/// @return names of parameters in this subset
std::vector<std::string> LinearSolver::getIndependentSubset(std::vector<std::string> &names, const double tolerance) const
{
    ASKAPTRACE("LinearSolver::getIndependentSubset");
    ASKAPDEBUGASSERT(names.size() > 0);
    std::vector<std::string> resultNames;
    resultNames.reserve(names.size());
    resultNames.push_back(names[0]);

    // this has been added to the subset, so delete it
    names.erase(names.begin());

    // for each name in subset (which grows as matches are found), check all remaining names for associates.
    for (std::vector<std::string>::const_iterator ciRes = resultNames.begin(); ciRes != resultNames.end(); ++ciRes) {

        // keep a temporary list of added names to erase from the main list
        std::vector<std::string> toErase;
        toErase.reserve(names.size());

        for (std::vector<std::string>::const_iterator ci = names.begin(); ci != names.end(); ++ci) {
            const casa::Matrix<double>& nm1 = normalEquations().normalMatrix(*ci, *ciRes);
            const casa::Matrix<double>& nm2 = normalEquations().normalMatrix(*ciRes, *ci);

            if (!allMatrixElementsAreZeros(nm1,tolerance) || !allMatrixElementsAreZeros(nm2,tolerance)) {
                // this parameter (iterated in the inner loop) belongs to the subset
                resultNames.push_back(*ci);
                // also add it to the temporary list of added names to erased
                toErase.push_back(*ci);
            } 
        }

        // erase all of the names that have just been added from the main list
        for (std::vector<std::string>::iterator it0 = toErase.begin(); it0 != toErase.end(); ++it0) {
            std::vector<std::string>::iterator it = std::find(names.begin(), names.end(), *it0);
            if (it != names.end()) names.erase(it);
        }

    } 
    return resultNames;
}

bool LinearSolver::compareGainNames(const std::string& gainA, const std::string& gainB) {
    try {
        std::pair<casa::uInt, std::string> paramInfoA = LinearSolver::extractChannelInfo(gainA);
        std::pair<casa::uInt, std::string> paramInfoB = LinearSolver::extractChannelInfo(gainB);

        // Parameter name excluding channel number.
        std::string parNameA = paramInfoA.second;
        std::string parNameB = paramInfoB.second;

        casa::uInt chanA = paramInfoA.first;
        casa::uInt chanB = paramInfoB.first;

        int res = parNameA.compare(parNameB);

        if (res == 0) {
        // Same names, sort by channel number.
            return (chanA <= chanB);
        } else {
            if (res < 0) {
                return true;
            } else {
                return false;
            }
        }
    }
    catch (AskapError &e) {
        return (gainA.compare(gainB) < 0);
    }
}

/// @brief solve for a subset of parameters
/// @details This method is used in solveNormalEquations
/// @param[in] params parameters to be updated
/// @param[in] quality Quality of the solution
/// @param[in] names names of the parameters to solve for 
std::pair<double,double> LinearSolver::solveSubsetOfNormalEquations(Params &params, Quality& quality,
                   const std::vector<std::string> &__names) const
{
    ASKAPTRACE("LinearSolver::solveSubsetOfNormalEquations");
#ifdef HAVE_MPI
    ASKAPLOG_INFO_STR(logger, "Started LinearSolver::solveSubsetOfNormalEquations, with MPI.");
#else
    ASKAPLOG_INFO_STR(logger, "Started LinearSolver::solveSubsetOfNormalEquations, without MPI.");
#endif

    std::pair<double, double> result(0.,0.);

    bool algorithmLSQR = (algorithm() == "LSQR");

    std::vector<std::string> names = __names;
    std::sort(names.begin(), names.end(), compareGainNames);

    // Solving A^T Q^-1 V = (A^T Q^-1 A) P

    int nParameters = 0;

    std::vector<std::pair<string, int> > indices(names.size());
    std::map<string, size_t> indicesMap;

    {
        std::vector<std::pair<string, int> >::iterator it = indices.begin();
        for (vector<string>::const_iterator cit=names.begin(); cit!=names.end(); ++cit,++it) {
            ASKAPDEBUGASSERT(it != indices.end());
            it->second = nParameters;
            it->first = *cit;

            indicesMap[it->first] = (size_t)(it->second);

            ASKAPLOG_DEBUG_STR(logger, "Processing " << *cit << " " << nParameters);
            const casa::uInt newParameters = normalEquations().dataVector(*cit).nelements();
            nParameters += newParameters;
            ASKAPDEBUGASSERT((params.isFree(*cit) ? params.value(*cit).nelements() : newParameters) == newParameters);
        }
    }
    ASKAPLOG_DEBUG_STR(logger, "Done");
    ASKAPCHECK(nParameters > 0, "No free parameters in a subset of normal equations");

    ASKAPDEBUGASSERT(indices.size() > 0);

    gsl_matrix * A = 0;
    gsl_vector * B = 0;
    gsl_vector * X = 0;

    if (!algorithmLSQR) {
        // Containers to convert the normal equations to gsl format.
        A = gsl_matrix_alloc (nParameters, nParameters);
        B = gsl_vector_alloc (nParameters);
        X = gsl_vector_alloc (nParameters);
    }

    //------------------------------------------------------------------------------
    // Define MPI partitioning (needed for LSQR solver).
    //------------------------------------------------------------------------------

    int myrank = 0;
    int nbproc = 1;
    bool matrixIsParallel = false;
#ifdef HAVE_MPI
    MPI_Comm mpi_comm = MPI_COMM_WORLD;
#endif

    if (algorithmLSQR) {
#ifdef HAVE_MPI
        if (parameters().count("parallelMatrix") > 0
            && parameters().at("parallelMatrix") == "true") {
            matrixIsParallel = true;
        }

        if (matrixIsParallel) {
        // The parallel matrix case - need to define the parallel partitioning.
            ASKAPCHECK(itsWorkersComm != MPI_COMM_NULL, "Workers communicator is not defined!");
            MPI_Comm_rank(itsWorkersComm, &myrank);
            MPI_Comm_size(itsWorkersComm, &nbproc);

            mpi_comm = itsWorkersComm;
        } else {
            mpi_comm = MPI_COMM_SELF;
        }
#endif
        if (myrank == 0)
            ASKAPLOG_DEBUG_STR(logger, "it, matrixIsParallel, nbproc = " << itsMajorLoopIterationNumber
                                        << ", " << matrixIsParallel << ", " << nbproc);
    }

    //------------------------------------------------------------------------------
    // Define LSQR solver sparse matrix.
    //------------------------------------------------------------------------------
#ifdef HAVE_MPI
    size_t nParametersTotal = lsqr::ParallelTools::get_total_number_elements(nParameters, nbproc, itsWorkersComm);
    size_t nParametersSmaller = lsqr::ParallelTools::get_nsmaller(nParameters, myrank, nbproc, itsWorkersComm);
#else
    size_t nParametersTotal = nParameters;
    size_t nParametersSmaller = 0;
#endif
    if (myrank == 0) ASKAPLOG_DEBUG_STR(logger, "nParameters = " << nParameters);
    if (myrank == 0) ASKAPLOG_DEBUG_STR(logger, "nParametersTotal = " << nParametersTotal);

    size_t nrows = 0;
    if (algorithmLSQR) {
        nrows = nParametersTotal;
    }

#ifdef HAVE_MPI
    lsqr::SparseMatrix matrix(nrows, mpi_comm);
#else
    lsqr::SparseMatrix matrix(nrows);
#endif

    //--------------------------------------------------------------------------------------------
    // Copy matrix elements from normal matrix (map of map of matrixes) to the solver matrix:
    //      - to gsl NxN matrix, for the SVD solver;
    //      - to sparse matrix (CSR format), for the LSQR solver.
    //--------------------------------------------------------------------------------------------
    const GenericNormalEquations& gne = dynamic_cast<const GenericNormalEquations&>(normalEquations());

    if (algorithmLSQR && matrixIsParallel) {
        for (size_t i = 0; i < nParametersSmaller; ++i) {
            matrix.NewRow();
        }
    }

    // Loop over matrix rows.
    for (std::vector<std::pair<string, int> >::const_iterator indit1 = indices.begin();
            indit1 != indices.end(); ++indit1) {

        const std::map<string, casa::Matrix<double> >::const_iterator colItBeg = gne.getNormalMatrixRowBegin(indit1->first);
        const std::map<string, casa::Matrix<double> >::const_iterator colItEnd = gne.getNormalMatrixRowEnd(indit1->first);

        if (colItBeg != colItEnd) {

            const size_t nrow = colItBeg->second.nrow();

            for (size_t row = 0; row < nrow; ++row) {

                if (algorithmLSQR) {
                    matrix.NewRow();
                }

                // Loop over column elements.
                for (std::map<string, casa::Matrix<double> >::const_iterator colIt = colItBeg;
                        colIt != colItEnd; ++colIt) {

                    const std::map<string, size_t>::const_iterator indicesMapIt = indicesMap.find(colIt->first);
                    if (indicesMapIt != indicesMap.end()) {
                    // It is a parameter to solve for, adding it to the matrix.

                        const size_t colIndex = indicesMapIt->second;
                        const casa::Matrix<double>& nm = colIt->second;

                        ASKAPCHECK(nrow == nm.nrow(), "Not consistent normal matrix element element dimension!");

                        const size_t ncolumn = nm.ncolumn();
                        for (size_t col = 0; col < ncolumn; ++col) {
                             const double elem = nm(row, col);
                             ASKAPCHECK(!std::isnan(elem), "Normal matrix seems to have NaN for row = "<< row << " and col = " << col << ", this shouldn't happen!");

                             if (algorithmLSQR) {
                                 matrix.Add(elem, col + colIndex);

                             } else {
                                 gsl_matrix_set(A, row + (indit1->second), col + colIndex, elem);
                             }
                        }
                    }
                }
            }

        } else {
        // Empty normal matrix row.
            if (algorithmLSQR) {
                // Need to add corresponding empty rows in the sparse matrix.
                const size_t nrow = normalEquations().dataVector(indit1->first).nelements();
                for (size_t i = 0; i < nrow; ++i) {
                    matrix.NewRow();
                }
            }
        }
    }

    if (algorithmLSQR) {
        ASKAPCHECK(matrix.GetCurrentNumberRows() == (nParametersSmaller + nParameters), "Wrong number of matrix rows!");
        if (matrixIsParallel) {
            // Adding ending matrix empty rows, i.e., the rows in a big block-diagonal matrix below the current block.
            size_t nEndRows = nParametersTotal - nParametersSmaller - nParameters;
            for (size_t i = 0; i < nEndRows; ++i) {
                matrix.NewRow();
            }
        }
        ASKAPCHECK(matrix.GetCurrentNumberRows() == nParametersTotal, "Wrong number of matrix rows!");
    }
    matrix.Finalize(nParameters);

    //----------------------------------------------------------------------------------------------------------------------------
    if (algorithmLSQR) {
        size_t nonzeros = matrix.GetNumberElements();
        double sparsity = (double)(nonzeros) / (double)(nParameters) / (double)(nParameters);
        ASKAPLOG_DEBUG_STR(logger, "Jacobian nonzeros, sparsity = " << nonzeros << ", " << sparsity << " on rank " << myrank);
    }

    if (!algorithmLSQR) {
        for (std::vector<std::pair<string, int> >::const_iterator indit1=indices.begin();indit1!=indices.end(); ++indit1) {
            const casa::Vector<double> &dv = normalEquations().dataVector(indit1->first);
            for (size_t row=0; row<dv.nelements(); ++row) {
                 const double elem = dv(row);
                 ASKAPCHECK(!std::isnan(elem), "Data vector seems to have NaN for row = "<<row<<", this shouldn't happen!");
                 gsl_vector_set(B, row+(indit1->second), elem);
            }
        }
    }

      /*
      // temporary code to export matrices, which cause problems with the GSL
      // to write up a clear case
      {
        std::ofstream os("dbg.dat");
        os<<nParameters<<std::endl;
        for (int row=0;row<nParameters;++row) {
             for (int col=0;col<nParameters;++col) {
                  if (col) {
                      os<<" ";
                  }
                  os<<gsl_matrix_get(A,row,col);
             }
             os<<std::endl;
        }
      }
      // end of the temporary code
      */

    if (algorithm() == "SVD") {
        ASKAPLOG_INFO_STR(logger, "Solving normal equations using the SVD solver");

        gsl_matrix * V = gsl_matrix_alloc (nParameters, nParameters);
        ASKAPDEBUGASSERT(V!=NULL);
        gsl_vector * S = gsl_vector_alloc (nParameters);
        ASKAPDEBUGASSERT(S!=NULL);
        gsl_vector * work = gsl_vector_alloc (nParameters);
        ASKAPDEBUGASSERT(work!=NULL);

        gsl_error_handler_t *oldhandler=gsl_set_error_handler_off();
        ASKAPLOG_DEBUG_STR(logger, "Running SV decomp");
        const int status = gsl_linalg_SV_decomp (A, V, S, work);
        // ASKAPCHECK(status == 0, "gsl_linalg_SV_decomp failed, status = "<<status);
        gsl_set_error_handler(oldhandler);

        // a hack for now. For some reason, for some matrices gsl_linalg_SV_decomp may return NaN as singular value, perhaps some
        // numerical precision issue inside SVD. Although it needs to be investigated further  (see ASKAPSDP-2270), for now trying
        // to replace those singular values with zeros to exclude them from processing. Note, singular vectors may also contain NaNs
        for (int i=0; i<nParameters; ++i) {
          if (std::isnan(gsl_vector_get(S,i))) {
              gsl_vector_set(S,i,0.);
          }
          for (int k=0; k < nParameters; ++k) {
               ASKAPCHECK(!std::isnan(gsl_matrix_get(V,i,k)), "NaN in V: i="<<i<<" k="<<k);
          }
        }

         // end of the hack

         //SVDecomp (A, V, S);

         // code to put a limit on the condition number of the system
         const double singularValueLimit = nParameters>1 ?
                     gsl_vector_get(S,0)/itsMaxCondNumber : -1.;
         for (int i=1; i<nParameters; ++i) {
              if (gsl_vector_get(S,i)<singularValueLimit) {
                  gsl_vector_set(S,i,0.);
              }
         }

        /*
        // temporary code for debugging
        {
          std::ofstream os("dbg2.dat");
          for (int i=0; i<nParameters; ++i) {
               os<<i<<" "<<gsl_vector_get(S,i)<<std::endl;
          }

          //std::cout<<"new singular value spectrum is ready"<<std::endl;
          //char tst;
          //std::cin>>tst;

        }
        // end of temporary code
        */

         gsl_vector * X = gsl_vector_alloc(nParameters);
         ASKAPDEBUGASSERT(X!=NULL);

         const int solveStatus = gsl_linalg_SV_solve (A, V, S, B, X);
         ASKAPCHECK(solveStatus == 0, "gsl_linalg_SV_solve failed");

// Now find the statistics for the decomposition
         int rank=0;
         double smin = 1e50;
         double smax = 0.0;
         for (int i=0;i<nParameters; ++i) {
              const double sValue = std::abs(gsl_vector_get(S, i));
              ASKAPCHECK(!std::isnan(sValue), "Got NaN as a singular value for normal matrix, this shouldn't happen S[i]="<<gsl_vector_get(S,i)<<" parameter "<<i<<" singularValueLimit="<<singularValueLimit);
              if(sValue>0.0) {
                 ++rank;
                 if ((sValue>smax) || (i == 0)) {
                     smax=sValue;
                 }
                 if ((sValue<smin) || (i == 0)) {
                     smin=sValue;
                 }
               }
         }
         result.first = smin;
         result.second = smax;

         quality.setDOF(nParameters);
         if (status != 0) {
             ASKAPLOG_WARN_STR(logger, "Solution is considered invalid due to gsl_linalg_SV_decomp failure, main matrix is effectively rank zero");
             quality.setRank(0);
             quality.setCond(0.);
             quality.setInfo("SVD decomposition rank deficient");
         } else {
             quality.setRank(rank);
             quality.setCond(smax/smin);
             if (rank==nParameters) {
                 quality.setInfo("SVD decomposition rank complete");
             } else {
                 quality.setInfo("SVD decomposition rank deficient");
             }
         }

// Update the parameters for the calculated changes. Exploit reference
// semantics of casa::Array.
         std::vector<std::pair<string, int> >::const_iterator indit;
         for (indit=indices.begin();indit!=indices.end();++indit) {
              casa::IPosition vecShape(1, params.value(indit->first).nelements());
              casa::Vector<double> value(params.value(indit->first).reform(vecShape));
              for (size_t i=0; i<value.nelements(); ++i)  {
//                 std::cout << value(i) << " " << gsl_vector_get(X, indit->second+i) << std::endl;
                   const double adjustment = gsl_vector_get(X, indit->second+i);
                   ASKAPCHECK(!std::isnan(adjustment), "Solution resulted in NaN as an update for parameter "<<(indit->second + i));
                   value(i) += adjustment;
              }
          }
          gsl_vector_free(S);
          gsl_vector_free(work);
          gsl_matrix_free(V);
    }
    else if (algorithmLSQR) {
        if (myrank == 0) ASKAPLOG_INFO_STR(logger, "Solving normal equations using the LSQR solver");

        //----------------------------------------------------
        // Define the right-hand side (the data misfit part).
        //----------------------------------------------------
        lsqr::Vector b_RHS(nParametersTotal, 0.);

        size_t nDataAdded = 0;
        for (std::vector<std::pair<string, int> >::const_iterator indit1=indices.begin();indit1!=indices.end(); ++indit1) {
            const casa::Vector<double> &dv = normalEquations().dataVector(indit1->first);
            for (size_t row = 0; row < dv.nelements(); ++row) {
                 const double elem = dv(row);
                 ASKAPCHECK(!std::isnan(elem), "Data vector seems to have NaN for row = " << row << ", this shouldn't happen!");
                 b_RHS[row+(indit1->second)] = elem;
                 nDataAdded++;
            }
        }
        ASKAPCHECK(nDataAdded == (size_t)(nParameters), "Wrong number of data added on rank " << myrank);

        if (matrixIsParallel) {
#ifdef HAVE_MPI
            lsqr::ParallelTools::get_full_array_in_place(nParameters, b_RHS, true, myrank, nbproc, itsWorkersComm);
#endif
        }

        //------------------------------------------------------------------
        // Adding smoothness constraints.
        //------------------------------------------------------------------
        bool addSmoothing = false;
        if (parameters().count("smoothing") > 0
            && parameters().at("smoothing") == "true") {
            addSmoothing = true;
        }

        if (addSmoothing) {
            ASKAPCHECK(matrixIsParallel, "Smoothing constraints should be used in the parallel matrix mode!");

            // Reading the number of channels.
            size_t nChannels = 0;
            if (parameters().count("nChan") > 0) {
                nChannels = std::atoi(parameters().at("nChan").c_str());
            }

            double smoothingWeight = getSmoothingWeight();

            addSmoothnessConstraints(matrix, b_RHS, indices, params, nParameters, nChannels, smoothingWeight);
        }
        if (myrank == 0) ASKAPLOG_DEBUG_STR(logger, "Matrix nelements = " << matrix.GetNumberElements());

        // A simple approximation for the upper bound of the rank of the  A'A matrix.
        size_t rank_approx = matrix.GetNumberNonemptyRows();

        //------------------------------------------------------------------
        // Adding damping.
        //------------------------------------------------------------------
        // Setting damping parameters.
        double alpha = 0.01;
        if (parameters().count("alpha") > 0) {
            alpha = std::atof(parameters().at("alpha").c_str());
        }

        double norm = 2.0;
        if (parameters().count("norm") > 0) {
            norm = std::atof(parameters().at("norm").c_str());
        }

        if (myrank == 0) ASKAPLOG_INFO_STR(logger, "Adding model damping, with alpha = " << alpha);

        lsqr::ModelDamping damping(nParameters);
        damping.Add(alpha, norm, matrix, b_RHS, NULL, NULL, NULL, myrank, nbproc);

        //-----------------------------------------------
        // Calculating the total cost.
        //-----------------------------------------------
        double total_cost = 0.;
        for (size_t i = 0; i < b_RHS.size(); ++i) {
            total_cost += b_RHS[i] * b_RHS[i];
        }
        if (myrank == 0) ASKAPLOG_INFO_STR(logger, "Total cost = " << total_cost);

        //-----------------------------------------------
        // Setting solver parameters.
        //-----------------------------------------------
        int niter = 100;
        if (parameters().count("niter") > 0) {
            niter = std::atoi(parameters().at("niter").c_str());
        }

        double rmin = 1.e-13;
        if (parameters().count("rmin") > 0) {
            rmin = std::atof(parameters().at("rmin").c_str());
        }

        bool suppress_output = true;
        if (parameters().count("verbose") > 0
            && parameters().at("verbose") == "true") {
            suppress_output = false;
        }

        //-----------------------------------------------
        // Solving the matrix system.
        //-----------------------------------------------
        casa::Timer timer;
        timer.mark();

        lsqr::Vector x(nParameters, 0.0);
        lsqr::LSQRSolver solver(matrix.GetCurrentNumberRows(), nParameters);

        solver.Solve(niter, rmin, matrix, b_RHS, x, suppress_output);

        ASKAPLOG_INFO_STR(logger, "Completed LSQR in " << timer.real() << " seconds on rank " << myrank);

        //------------------------------------------------------------------------
        // Update the parameters for the calculated changes.
        // Exploit reference semantics of casa::Array.
        //------------------------------------------------------------------------
        std::vector<std::pair<string, int> >::const_iterator indit;
        for (indit=indices.begin();indit!=indices.end();++indit) {
            casa::IPosition vecShape(1, params.value(indit->first).nelements());
            casa::Vector<double> value(params.value(indit->first).reform(vecShape));
            for (size_t i=0; i<value.nelements(); ++i) {
                const double adjustment = x[indit->second + i];
                ASKAPCHECK(!std::isnan(adjustment), "Solution resulted in NaN as an update for parameter "<<(indit->second + i));
                value(i) += adjustment;
            }
        }

         //------------------------------------------------------------------------
         // Set approximate solution quality.
         quality.setDOF(nParameters);
         quality.setRank(rank_approx);
    }
    else if (algorithm() == "Chol") {
        // TODO: It seems this branch is never actually used. Need to remove it?
        ASKAPLOG_INFO_STR(logger, "Solving normal equations using the Cholesky decomposition solver");

        quality.setInfo("Cholesky decomposition");
        gsl_linalg_cholesky_decomp(A);
        gsl_linalg_cholesky_solve(A, B, X);
        // Update the parameters for the calculated changes.
        std::vector<std::pair<string, int> >::const_iterator indit;
        for (indit=indices.begin();indit!=indices.end();++indit)
        {
          casa::IPosition vecShape(1, params.value(indit->first).nelements());
          casa::Vector<double> value(params.value(indit->first).reform(vecShape));
          for (size_t i=0; i<value.nelements(); ++i)  {
               value(i)+=gsl_vector_get(X, indit->second+i);
          }
        }
    }
    else {
        ASKAPTHROW(AskapError, "Unknown calibration solver type: " << algorithm());
    }

    if (!algorithmLSQR) {
        // Free up gsl storage.
        gsl_matrix_free(A);
        gsl_vector_free(B);
        gsl_vector_free(X);
    }

    return result;
}

double LinearSolver::getSmoothingWeight() const {

    double smoothingWeight = 0.;

    double smoothingMinWeight = 0.;
    if (parameters().count("smoothingMinWeight") > 0) {
        smoothingMinWeight = std::atof(parameters().at("smoothingMinWeight").c_str());
    }

    double smoothingMaxWeight = 3.e+6;
    if (parameters().count("smoothingMaxWeight") > 0) {
        smoothingMaxWeight = std::atof(parameters().at("smoothingMaxWeight").c_str());
    }

    size_t smoothingNsteps = 10;
    if (parameters().count("smoothingNsteps") > 0) {
        smoothingNsteps = std::atoi(parameters().at("smoothingNsteps").c_str());
    }

    if (itsMajorLoopIterationNumber < smoothingNsteps) {
        if (smoothingMinWeight == smoothingMaxWeight) {
            smoothingWeight = smoothingMaxWeight;
        } else {
            double span = smoothingMaxWeight - smoothingMinWeight;
            ASKAPCHECK(span > 0, "Wrong smoothing weight!");

            // Logarithmic sweep (between the min and max weights).
            smoothingWeight = smoothingMinWeight + std::pow(10., log10(span) / (double)(smoothingNsteps) * (double)(itsMajorLoopIterationNumber));
        }
    } else {
        // Relaxation with constant weight.
        smoothingWeight = smoothingMaxWeight;
    }
    return smoothingWeight;
}

/// @brief solve for parameters
/// The solution is constructed from the normal equations and given
/// parameters are updated. If there are no free parameters in the
/// given Params class, all unknowns in the normal
/// equatons will be solved for.
/// @param[in] params parameters to be updated
/// @param[in] quality Quality of solution
/// @note This is fully general solver for the normal equations for any shape
/// parameters.
bool LinearSolver::solveNormalEquations(Params &params, Quality& quality)
{
    ASKAPTRACE("LinearSolver::solveNormalEquations");

    // Solving A^T Q^-1 V = (A^T Q^-1 A) P
    // Find all the free parameters.
    vector<string> names(params.freeNames());
    if (names.size() == 0) {
        // List of parameters is empty, will solve for all
        // unknowns in the equation.
        names = normalEquations().unknowns();
    }
    ASKAPCHECK(names.size() > 0, "No free parameters in Linear Solver");

    if (names.size() < 100 // No need to extract independent blocks if number of unknowns is small.
        || algorithm() == "LSQR") {
        solveSubsetOfNormalEquations(params, quality, names);
    } else {
        while (names.size() > 0) {
            ASKAPLOG_INFO_STR(logger, "Solving independent subset of parameters");
            const std::vector<std::string> subsetNames = getIndependentSubset(names,1e-6);
            solveSubsetOfNormalEquations(params, quality, subsetNames);
        }
    }
    return true;
};

Solver::ShPtr LinearSolver::clone() const
{
    return Solver::ShPtr(new LinearSolver(*this));
}

#ifdef HAVE_MPI
    void LinearSolver::setWorkersCommunicator(const MPI_Comm &comm)
    {
        assert(comm != MPI_COMM_NULL);
        MPI_Comm_dup(comm, &itsWorkersComm);
    }
#endif

void LinearSolver::setMajorLoopIterationNumber(size_t it)
{
    itsMajorLoopIterationNumber = it;
}

// NOTE: Copied from "calibaccess/CalParamNameHelper.h", as currently accessors depends of scimath.
/// @brief extract coded channel and parameter name
/// @details This is a reverse operation to codeInChannel. Note, no checks are done that the name passed
/// has coded channel present.
/// @param[in] name full name of the parameter
/// @return a pair with extracted channel and the base parameter name
std::pair<casa::uInt, std::string> LinearSolver::extractChannelInfo(const std::string &name)
{
    size_t pos = name.rfind(".");
    ASKAPCHECK(pos != std::string::npos, "Expect dot in the parameter name passed to extractChannelInfo, name="<<name);
    ASKAPCHECK(pos + 1 != name.size(), "Parameter name="<<name<<" ends with a dot");
    return std::pair<casa::uInt, std::string>(utility::fromString<casa::uInt>(name.substr(pos+1)),name.substr(0,pos));
}

/// @brief Adding smoothness constraints to the system of equations.
void addSmoothnessConstraints(lsqr::SparseMatrix& matrix, lsqr::Vector& b_RHS,
                              const std::vector<std::pair<std::string, int> >& indices,
                              const Params& params,
                              size_t nParameters,
                              size_t nChannels,
                              double smoothingWeight,
                              int gradientType)
{
    ASKAPCHECK(nChannels > 1, "Wrong number of channels for smoothness constraints!");

#ifdef HAVE_MPI
    MPI_Comm workersComm = matrix.GetComm();
    ASKAPCHECK(workersComm != MPI_COMM_NULL, "Workers communicator is not defined!");

    int myrank, nbproc;
    MPI_Comm_rank(workersComm, &myrank);
    MPI_Comm_size(workersComm, &nbproc);

    size_t nParametersTotal = lsqr::ParallelTools::get_total_number_elements(nParameters, nbproc, workersComm);
    size_t nParametersSmaller = lsqr::ParallelTools::get_nsmaller(nParameters, myrank, nbproc, workersComm);
#else
    int myrank = 0;
    int nbproc = 1;
    size_t nParametersTotal = nParameters;
    size_t nParametersSmaller = 0;
#endif

    if (myrank == 0) ASKAPLOG_INFO_STR(logger, "Adding smoothness constraints, with weight = " << smoothingWeight);

    matrix.Extend(nParametersTotal);
    b_RHS.resize(b_RHS.size() + nParametersTotal);

    //-----------------------------------------------------------------------------
    // Extract the solution at the current major iteration (before the update).
    //-----------------------------------------------------------------------------
    std::vector<double> x0(nParametersTotal);
    size_t counter = 0;
    for (std::vector<std::pair<string, int> >::const_iterator indit = indices.begin();
                     indit != indices.end(); ++indit) {
        casa::IPosition vecShape(1, params.value(indit->first).nelements());
        casa::Vector<double> value(params.value(indit->first).reform(vecShape));
        for (size_t i=0; i<value.nelements(); ++i) {
            x0[indit->second + i] = value(i);
            counter++;
        }
    }
    ASKAPCHECK(counter == nParameters, "Wrong number of parameters!");

#ifdef HAVE_MPI
    lsqr::ParallelTools::get_full_array_in_place(nParameters, x0, true, myrank, nbproc, workersComm);
#endif

    //-----------------------------------------------------------------------------
    // Assume the same number of channels at every CPU.
    size_t nChannelsLocal = nChannels / (nParametersTotal / nParameters);
    size_t nextChannelIndexShift = nParameters - (nChannelsLocal - 1) * 2;

    if (myrank == 0) ASKAPLOG_DEBUG_STR(logger, "nChannelsLocal = " << nChannelsLocal);

    std::vector<int> leftIndexGlobal(nParametersTotal);
    std::vector<int> rightIndexGlobal(nParametersTotal);

    // NOTE: Assume channels are ordered with the MPI rank order, i.e., the higher the rank the higher the channel number.
    // E.g.: for 40 channels and 4 workers, rank 0 has channels 0-9, rank 1: 10-19, rank 2: 20-29, and rank 3: 30-39.

    if (gradientType == 0) {
    // Forawrd difference scheme.

        size_t localChannelNumber = 0;
        for (size_t i = 0; i < nParametersTotal; i += 2) {

            bool lastLocalChannel = (localChannelNumber == nChannelsLocal - 1);

            if (lastLocalChannel) {
                size_t shiftedIndex = i + nextChannelIndexShift;

                if (shiftedIndex < nParametersTotal) {
                // Reached last local channel - shift the 'next' index.
                    // Real part.
                    leftIndexGlobal[i] = i;
                    rightIndexGlobal[i] = shiftedIndex;
                    // Imaginary part.
                    leftIndexGlobal[i + 1] = leftIndexGlobal[i] + 1;
                    rightIndexGlobal[i + 1] = rightIndexGlobal[i] + 1;
                } else {
                // Reached last global channel - do not add constraints - it is already coupled with previous one.
                    // Real part.
                    leftIndexGlobal[i] = -1;
                    rightIndexGlobal[i] = -1;
                    // Imaginary part.
                    leftIndexGlobal[i + 1] = -1;
                    rightIndexGlobal[i + 1] = -1;
                }

            } else {
                // Real part.
                leftIndexGlobal[i] = i;
                rightIndexGlobal[i] = i + 2;
                // Imaginary part.
                leftIndexGlobal[i + 1] = leftIndexGlobal[i] + 1;
                rightIndexGlobal[i + 1] = rightIndexGlobal[i] + 1;
            }

            if (lastLocalChannel) {
                // Reset local channel counter.
                localChannelNumber = 0;
            } else {
                localChannelNumber++;
            }
        }
    } else {
    // Central difference scheme.

        size_t localChannelNumber = 0;
        for (size_t i = 0; i < nParametersTotal; i += 2) {

            bool firstLocalChannel = (localChannelNumber == 0);
            bool lastLocalChannel = (localChannelNumber == nChannelsLocal - 1);

            int shiftedLeftIndex = i - nextChannelIndexShift;
            size_t shiftedRightIndex = i + nextChannelIndexShift;

            if (firstLocalChannel && lastLocalChannel) {
            // One local channel.
                if ((shiftedLeftIndex >= 0) && (shiftedRightIndex < nParametersTotal)) {
                    // Real part.
                    leftIndexGlobal[i] = shiftedLeftIndex;
                    rightIndexGlobal[i] = shiftedRightIndex;
                    // Imaginary part.
                    leftIndexGlobal[i + 1] = leftIndexGlobal[i] + 1;
                    rightIndexGlobal[i + 1] = rightIndexGlobal[i] + 1;

                } else {
                // First/last global channel - do not add constraints - it is already coupled with next/previous one.
                    // Real part.
                    leftIndexGlobal[i] = -1;
                    rightIndexGlobal[i] = -1;
                    // Imaginary part.
                    leftIndexGlobal[i + 1] = -1;
                    rightIndexGlobal[i + 1] = -1;
                }

            } else if (firstLocalChannel) {

                if (shiftedLeftIndex >= 0) {
                // First local channel - shift the 'left' index.
                    // Real part.
                    leftIndexGlobal[i] = shiftedLeftIndex;
                    rightIndexGlobal[i] = i + 2;
                    // Imaginary part.
                    leftIndexGlobal[i + 1] = leftIndexGlobal[i] + 1;
                    rightIndexGlobal[i + 1] = rightIndexGlobal[i] + 1;

                } else {
                // First/last global channel - do not add constraints - it is already coupled with next/previous one.
                    // Real part.
                    leftIndexGlobal[i] = -1;
                    rightIndexGlobal[i] = -1;
                    // Imaginary part.
                    leftIndexGlobal[i + 1] = -1;
                    rightIndexGlobal[i + 1] = -1;
                }
            } else if (lastLocalChannel) {

                if (shiftedRightIndex < nParametersTotal) {
                // Last local channel - shift the 'right' index.
                    // Real part.
                    leftIndexGlobal[i] = i - 2;
                    rightIndexGlobal[i] = shiftedRightIndex;
                    // Imaginary part.
                    leftIndexGlobal[i + 1] = leftIndexGlobal[i] + 1;
                    rightIndexGlobal[i + 1] = rightIndexGlobal[i] + 1;
                } else {
                // Reached last global channel - do not add constraints - it is already coupled with previous one.
                    // Real part.
                    leftIndexGlobal[i] = -1;
                    rightIndexGlobal[i] = -1;
                    // Imaginary part.
                    leftIndexGlobal[i + 1] = -1;
                    rightIndexGlobal[i + 1] = -1;
                }

            } else {
                // Real part.
                leftIndexGlobal[i] = i - 2;
                rightIndexGlobal[i] = i + 2;
                // Imaginary part.
                leftIndexGlobal[i + 1] = leftIndexGlobal[i] + 1;
                rightIndexGlobal[i + 1] = rightIndexGlobal[i] + 1;
            }

            if (lastLocalChannel) {
                // Reset local channel counter.
                localChannelNumber = 0;
            } else {
                localChannelNumber++;
            }
        }
    }

    //-----------------------------------------------------------------------------
    // Adding Jacobian of the gradient to the matrix.
    //-----------------------------------------------------------------------------
    double cost = 0.;

    double matrix_val[2];
    matrix_val[0] = - smoothingWeight;
    matrix_val[1] = + smoothingWeight;

    for (size_t i = 0; i < nParametersTotal; i++) {

        //----------------------------------------------------
        // Adding matrix values.
        //----------------------------------------------------
        matrix.NewRow();

        // Global matrix column indexes.
        size_t globIndex[2];
        globIndex[0] = leftIndexGlobal[i];  // Left index: "minus" term in finite difference (e.g. -x[i] for x' =  x[i+1] - x[i])
        globIndex[1] = rightIndexGlobal[i]; // Right index: "plus" term in finite difference (e.g. x[i+1] for x' =  x[i+1] - x[i]).

        for (size_t k = 0; k < 2; k++) {
            if (globIndex[k] >= 0
                && globIndex[k] >= nParametersSmaller
                && globIndex[k] < nParametersSmaller + nParameters) {

                // Local matrix column index (at the current CPU).
                size_t localColumnIndex = globIndex[k] - nParametersSmaller;
                matrix.Add(matrix_val[k], localColumnIndex);
            }
        }

        //----------------------------------------------------
        // Adding the Right-Hand Side.
        //----------------------------------------------------
        double b_RHS_value = 0.;
        if (leftIndexGlobal[i] >= 0 && rightIndexGlobal[i] >= 0) {
            b_RHS_value = - smoothingWeight * (x0[rightIndexGlobal[i]] - x0[leftIndexGlobal[i]]);
        } else {
            ASKAPCHECK(leftIndexGlobal[i] == -1 && rightIndexGlobal[i] == -1,
                    "Wrong finite difference indexes: " << i << ", " << leftIndexGlobal[i] << ", " << rightIndexGlobal[i]);
        }
        size_t b_index = matrix.GetCurrentNumberRows() - 1;
        b_RHS[b_index] = b_RHS_value;

        cost += b_RHS_value * b_RHS_value;
    }

    if (myrank == 0) ASKAPLOG_INFO_STR(logger, "Smoothness constraints cost = " << cost / (smoothingWeight * smoothingWeight));

    matrix.Finalize(nParameters);
}

  }
}
