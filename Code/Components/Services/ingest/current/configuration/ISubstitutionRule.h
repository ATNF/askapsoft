/// @file ISubstitutionRule.h
///
/// @copyright (c) 2012 CSIRO
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
/// @author Max Voronkov <maxim.voronkov@csiro.au>

#ifndef ASKAP_CP_INGEST_ISUBSTITUTIONRULE_H
#define ASKAP_CP_INGEST_ISUBSTITUTIONRULE_H

// System include
#include <set>
#include <string>

namespace askap {
namespace cp {
namespace ingest {

/// @brief Interface for substitution rule
/// @details This is a generic interface for a subsitution rule replacing
/// some keywords (e.g. %w) by some string which may require MPI collective calls
/// or other access to MPI-dependent info which, in general, cannot be accessed at all
/// times necesitating a two-stage approach. The interface encapsulates one such effect and
/// the actual substitution is done by an instance of SubstitutionHandler class. The interface 
/// informs the handler which keywords are handled by this particular object, provides an entry 
/// point for initialisation (when values should be obtained via MPI, if necessary), access method
/// to particular values and also a check method that values are identical for all ranks (this may
/// require another MPI call, so the appropriate information should be gathered during initialisation.
/// In some cases, however, this is known from the context up front (i.e. due to gathering action
/// done during initialisation)
///
/// @note This interface and SubstitutionHandler are generic enough and could be moved to Base
/// at some point, but particular implementations are probably specific to ingest. Leave everything here
/// for now.
class ISubstitutionRule {
   public:

   /// @brief virtual destructor to keep the compiler happy
   ~ISubstitutionRule();

   /// @brief obtain keywords handled by this object
   /// @details This method returns a set of string keywords
   /// (without leading % sign in our implementation, but in general this 
   /// can be just logical full-string keyword, we don't have to limit ourselves
   /// to particular single character tags) which this class recognises. Any of these
   /// keyword can be passed to operator() once the object is initialised
   /// @return set of keywords this object recognises
   virtual std::set<std::string> keywords() const = 0;

   /// @brief initialise the object
   /// @details This is the only place where MPI calls may happen. Therefore, 
   /// initialisation has to be done at the appropriate time in the program.
   /// It is also expected that only substitution rules which are actually needed
   /// will be initialised  and used. So construction/destruction should be a light
   /// operation. In this method, the implementations are expected to provide a
   /// mechanism to obtain values for all keywords handled by this object.
   virtual void initialise() = 0;

   /// @brief obtain value of a particular keyword
   /// @details This is the main access method which is supposed to be called after
   /// initialise(). 
   /// @param[in] kw keyword to access, must be from the set returned by keywords
   /// @return value of the requested keyword
   /// @note  An exception may be thrown if the initialise() method is not called
   /// prior to an attempt to access the value.
   virtual std::string operator()(const std::string &kw) const = 0;

   /// @brief check if values are rank-independent
   /// @details The implementation of this interface should evaluate a flag and return it
   /// in this method to show whether the value for a particular keyword is
   /// rank-independent or not. This is required to encapsulate all MPI related calls in
   /// the initialise. Sometimes, the value of the flag can be known up front, e.g. if
   /// the value is the result of gather-scatter operation or if it is based on rank number.
   /// @param[in] kw keyword to check the flag for
   /// @return true, if the given keyword has the same value for all ranks
   virtual bool isRankIndependent() const = 0;
};


};
};
};
#endif // #ifndef ASKAP_CP_INGEST_ISUBSTITUTIONRULE_H

