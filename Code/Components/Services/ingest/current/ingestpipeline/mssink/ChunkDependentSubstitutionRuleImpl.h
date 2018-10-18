/// @file ChunkDependentSubstitutionRuleImpl.h
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

#ifndef ASKAP_CP_INGEST_CHUNKDEPENDENTSUBSTITUTIONRULEIMPL_H
#define ASKAP_CP_INGEST_CHUNKDEPENDENTSUBSTITUTIONRULEIMPL_H

// ASKAPsoft includes
#include "configuration/IChunkDependentSubstitutionRule.h"
#include "configuration/Configuration.h"

namespace askap {
namespace cp {
namespace ingest {

/// @brief Common behaviour of our data-dependent classes.
/// @details This can be refactored later on, as there seems to be some commonality with 
/// non-data-dependent methods and lack of generality w.r.t. non-integer value types.
/// Only implement what is necessary now.
class ChunkDependentSubstitutionRuleImpl : virtual public IChunkDependentSubstitutionRule {
public:

   /// @brief constructor
   /// @details
   /// @param[in] kw keyword string to represent
   /// @param[in] rank this rank
   /// @param[in] nprocs number of ranks
   ChunkDependentSubstitutionRuleImpl(const std::string &kw, int rank, int nprocs);

   // implementation of interface mentods

   /// @brief obtain keywords handled by this object
   /// @details This method returns a set of string keywords
   /// (without leading % sign in our implementation, but in general this 
   /// can be just logical full-string keyword, we don't have to limit ourselves
   /// to particular single character tags) which this class recognises. Any of these
   /// keyword can be passed to operator() once the object is initialised
   /// @return set of keywords this object recognises
   virtual std::set<std::string> keywords() const; 

   /// @brief obtain value of a particular keyword
   /// @details This is the main access method which is supposed to be called after
   /// initialise(). 
   /// @param[in] kw keyword to access, must be from the set returned by keywords
   /// @return value of the requested keyword
   /// @note  An exception may be thrown if the initialise() method is not called
   /// prior to an attempt to access the value.
   virtual std::string operator()(const std::string &kw) const;

   /// @brief check if values are rank-independent
   /// @details The implementation of this interface should evaluate a flag and return it
   /// in this method to show whether the value for a particular keyword is
   /// rank-independent or not. This is required to encapsulate all MPI related calls in
   /// the initialise. Sometimes, the value of the flag can be known up front, e.g. if
   /// the value is the result of gather-scatter operation or if it is based on rank number.
   /// @param[in] kw keyword to check the flag for
   /// @return true, if the given keyword has the same value for all ranks
   virtual bool isRankIndependent() const;

   /// @brief check that the rule is in use
   /// @return true, if this particular rule has been initialised and, therefore, is in use
   bool inUse() const; 

   /// @brief get rank
   /// @return rank passed via constructor
   int rank() const;

   /// @brief get number of ranks
   /// @return number of ranks passed via constructor
   int nprocs() const;

   /// @brief initialise the object
   /// @details This overrides implementation in base class to set in use flag and to
   /// aggregate values after initialisation to set up rank dependence flags.
   /// For all practical purposes, that implementation is used and the actual entry point
   /// is via the pure virtual initialise method which accepts chunk as arguments (to be
   /// defined in derived methods).
   virtual void initialise();

protected:
   
   /// @brief set the value represented by this class
   /// @param[in] val value to set
   /// @note An exception is raised if the value is set after the class has been initialised
   void setValue(int val);

   /// @brief obtain value
   /// @return value corresponding to this rank
   /// @note,operator() does the same with additional cross-checks and string conversion
   inline int value() const  { return itsValue; }

private:
   
   /// @brief keyword name handled by this class
   std::string itsKeyword;

   /// @brief value for this rank
   int itsValue;

   /// @brief number of ranks (needed for delayed initialisation)
   int itsNProcs;

   /// @brief this rank number
   int itsRank;

   /// @brief rank-independence flag, setup at initialisation
   bool itsRankIndependent;

   /// @brief true, if rule has been initialised
   bool itsHasBeenInitialised;
};


};
};
};
#endif // #ifndef ASKAP_CP_INGEST_CHUNKDEPENDENTSUBSTITUTIONRULEIMPL_H

