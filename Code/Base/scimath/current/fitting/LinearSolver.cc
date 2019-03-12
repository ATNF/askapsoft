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

#include <askap/AskapError.h>
#include <profile/AskapProfiler.h>
#include <boost/config.hpp>

#include <casacore/casa/aips.h>
#include <casacore/casa/Arrays/Array.h>
#include <casacore/casa/Arrays/Matrix.h>
#include <casacore/casa/Arrays/Vector.h>

#include <gsl/gsl_matrix.h>
#include <gsl/gsl_vector.h>
#include <gsl/gsl_linalg.h>

#include <lsqr_solver/GlobalTypedefs.h>
#include <lsqr_solver/SparseMatrix.h>
#include <lsqr_solver/LSQRSolver.h>
#include <lsqr_solver/ModelDamping.h>

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
           itsMaxCondNumber(maxCondNumber) 
    {
      ASKAPASSERT(itsMaxCondNumber!=0);
    };

    
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
    
    
/// @brief solve for a subset of parameters
/// @details This method is used in solveNormalEquations
/// @param[in] params parameters to be updated           
/// @param[in] quality Quality of the solution
/// @param[in] names names of the parameters to solve for 
std::pair<double,double>  LinearSolver::solveSubsetOfNormalEquations(Params &params, Quality& quality, 
                   const std::vector<std::string> &names) const
{
    ASKAPTRACE("LinearSolver::solveSubsetOfNormalEquations");
    std::pair<double,double> result(0.,0.);

// Solving A^T Q^-1 V = (A^T Q^-1 A) P

    int nParameters = 0;

    std::vector<std::pair<string, int> > indices(names.size());
    {
      std::vector<std::pair<string, int> >::iterator it = indices.begin();
      for (vector<string>::const_iterator cit=names.begin(); cit!=names.end(); ++cit,++it)
      {
        ASKAPDEBUGASSERT(it != indices.end());
        it->second = nParameters;
        it->first = *cit;
        ASKAPLOG_DEBUG_STR(logger, "Processing "<<*cit<<" "<<nParameters);
        const casa::uInt newParameters = normalEquations().dataVector(*cit).nelements();
        nParameters += newParameters;
        ASKAPDEBUGASSERT((params.isFree(*cit) ? params.value(*cit).nelements() : newParameters) == newParameters);
      }
    }
    ASKAPLOG_DEBUG_STR(logger, "Done");
    ASKAPCHECK(nParameters>0, "No free parameters in a subset of normal equations");

    ASKAPDEBUGASSERT(indices.size() > 0);

    // Convert the normal equations to gsl format
    gsl_matrix * A = gsl_matrix_alloc (nParameters, nParameters);

    gsl_vector * B = 0;
    gsl_vector * X = 0;

    if (algorithm() != "LSQR") {
        B = gsl_vector_alloc (nParameters);
        X = gsl_vector_alloc (nParameters);
    }

    for (std::vector<std::pair<string, int> >::const_iterator indit2=indices.begin();indit2!=indices.end(); ++indit2)  {
        for (std::vector<std::pair<string, int> >::const_iterator indit1=indices.begin();indit1!=indices.end(); ++indit1)  {
             // Axes are dof, dof for each parameter
             // Take a deep breath for const-safe indexing into the double layered map
             const casa::Matrix<double>& nm = normalEquations().normalMatrix(indit1->first, indit2->first);

             for (size_t row=0; row<nm.nrow(); ++row)  {
                  for (size_t col=0; col<nm.ncolumn(); ++col) {
                       const double elem = nm(row,col);
                       ASKAPCHECK(!std::isnan(elem), "Normal matrix seems to have NaN for row = "<<row<<" and col = "<<col<<", this shouldn't happem!");
                       gsl_matrix_set(A, row+(indit1->second), col+(indit2->second), elem);
                       //std::cout << "A " << row+(indit1->second) << " " << col+(indit2->second) << " " << nm(row,col) << std::endl;
                  }
             }
         }
    }

    if (algorithm() != "LSQR") {
        for (std::vector<std::pair<string, int> >::const_iterator indit1=indices.begin();indit1!=indices.end(); ++indit1) {
            const casa::Vector<double> &dv = normalEquations().dataVector(indit1->first);
            for (size_t row=0; row<dv.nelements(); ++row) {
                 const double elem = dv(row);
                 ASKAPCHECK(!std::isnan(elem), "Data vector seems to have NaN for row = "<<row<<", this shouldn't happem!");
                 gsl_vector_set(B, row+(indit1->second), elem);
    //          std::cout << "B " << row << " " << dv(row) << std::endl;
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

        const int status = gsl_linalg_SV_decomp (A, V, S, work);
        ASKAPCHECK(status == 0, "gsl_linalg_SV_decomp failed, status = "<<status);

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
         quality.setRank(rank);
         quality.setCond(smax/smin);
         if(rank==nParameters) {
            quality.setInfo("SVD decomposition rank complete");
         } else {
            quality.setInfo("SVD decomposition rank deficient");
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
    else if (algorithm() == "LSQR") {
        int myrank = 0;
        int nbproc = 1;

        // Setting damping parameters.
        double alpha = 0.01;
        if (parameters().count("alpha") > 0) {
            alpha = std::atof(parameters().at("alpha").c_str());
        }

        double norm = 2.0;
        if (parameters().count("norm") > 0) {
            norm = std::atof(parameters().at("norm").c_str());
        }

        ASKAPLOG_INFO_STR(logger, "Solving normal equations using the LSQR solver, with alpha = " << alpha);

        size_t nrows = nParameters;
        size_t ncolumms = nParameters;

        lsqr::SparseMatrix matrix(nrows, nrows * ncolumms);
        lsqr::Vector b_RHS(nrows, 0.0);

        // Set the matrix.
        for (size_t j = 0; j < nrows; ++j) {
            matrix.NewRow();
            for (size_t i = 0; i < ncolumms; ++i) {
                double value = gsl_matrix_get(A, j, i);
                if (value != 0.0) {
                    matrix.Add(value, i);
                }
            }
        }
        matrix.Finalize(ncolumms);

        // A simple approximation for the upper bound of the rank of the  A'A matrix.
        size_t rank_approx = matrix.GetNumberNonemptyRows();

        //std::cout << "Matrix sparsity: " << (double)matrix.GetNumberElements() / (double)(nrows * ncolumms) << std::endl;

        for (std::vector<std::pair<string, int> >::const_iterator indit1=indices.begin();indit1!=indices.end(); ++indit1) {
            const casa::Vector<double> &dv = normalEquations().dataVector(indit1->first);
            for (size_t row=0; row<dv.nelements(); ++row) {
                 const double elem = dv(row);
                 ASKAPCHECK(!std::isnan(elem), "Data vector seems to have NaN for row = "<<row<<", this shouldn't happem!");
                 //gsl_vector_set(B, row+(indit1->second), elem);
                 b_RHS[row+(indit1->second)] = elem;
            }
        }

        //-----------------------------------------------
        // Add damping.
        //-----------------------------------------------

        lsqr::ModelDamping damping(ncolumms);
        damping.Add(alpha, norm, matrix, b_RHS, NULL, NULL, NULL, myrank, nbproc);

        //-------------------------------------
        // Solve matrix system.
        //-------------------------------------

        // Setting solver parameters.

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

        lsqr::Vector x(ncolumms, 0.0);
        lsqr::LSQRSolver solver(matrix.GetCurrentNumberRows(), ncolumms);

        solver.Solve(niter, rmin, matrix, b_RHS, x, myrank, nbproc, suppress_output);

        //------------------------------------------------------------------------
        // Update the parameters for the calculated changes. Exploit reference
        // semantics of casa::Array.
         std::vector<std::pair<string, int> >::const_iterator indit;
         for (indit=indices.begin();indit!=indices.end();++indit) {
              casa::IPosition vecShape(1, params.value(indit->first).nelements());
              casa::Vector<double> value(params.value(indit->first).reform(vecShape));
              for (size_t i=0; i<value.nelements(); ++i)  {
                   //const double adjustment = gsl_vector_get(X, indit->second+i);
                   const double adjustment = x[indit->second+i];
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

    // Free up gsl storage.
    gsl_matrix_free(A);

    if (algorithm() != "LSQR")
    {
        gsl_vector_free(B);
        gsl_vector_free(X);
    }

    return result;
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
     
// Find all the free parameters
      vector<string> names(params.freeNames());
      if (names.size() == 0) {
          // list of parameters is empty, will solve for all 
          // unknowns in the equation 
          names = normalEquations().unknowns();
      }
      ASKAPCHECK(names.size()>0, "No free parameters in Linear Solver");

      if (names.size() < 100) {
          // no need to extract independent blocks if number of unknowns is small
          solveSubsetOfNormalEquations(params,quality,names);
      } else {
          while (names.size() > 0) {
              const std::vector<std::string> subsetNames = getIndependentSubset(names,1e-6);
              /* this is now being done inside getIndependentSubset
              if (subsetNames.size() == names.size()) {
                  names.resize(0);
              } else {
                  // remove the elements corresponding to the current subset from the list of names prepared for the
                  // following integration
                  std::vector<size_t> indicesToRemove(subsetNames.size(),subsetNames.size());                                   
                  for (size_t index = 0,indexToRemove = 0; index < names.size(); ++index) {
                       if (find(subsetNames.begin(),subsetNames.end(),names[index]) != subsetNames.end()) {
                           ASKAPDEBUGASSERT(indexToRemove < indicesToRemove.size());
                           indicesToRemove[indexToRemove++] = index;
                       }
                  }
                  
                  // the following could be done more elegantly/faster, but leave the optimisation to later time                  
                  for (std::vector<size_t>::const_reverse_iterator ci = indicesToRemove.rbegin(); ci!=indicesToRemove.rend(); ++ci) {
                       ASKAPDEBUGASSERT(*ci < names.size());
                       names.erase(names.begin() + *ci);
                  }
              }
              */
              solveSubsetOfNormalEquations(params,quality, subsetNames);
          } 
      }
        
      return true;
    };

    Solver::ShPtr LinearSolver::clone() const
    {
      return Solver::ShPtr(new LinearSolver(*this));
    }

  }
}
