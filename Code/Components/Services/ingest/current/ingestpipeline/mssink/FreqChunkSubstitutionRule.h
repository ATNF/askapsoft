/// @file FreqChunkSubstitutionRule.h
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

#ifndef ASKAP_CP_INGEST_FREQCHUNKSUBSTITUTIONRULE_H
#define ASKAP_CP_INGEST_FREQCHUNKSUBSTITUTIONRULE_H

// ASKAPsoft includes
#include "configuration/IChunkDependentSubstitutionRule.h"
#include "configuration/Configuration.h"

namespace askap {
namespace cp {
namespace ingest {

/// @brief Substitute keyword by frequency chunk number
/// @details This is an example of data dependent substituion method, The result of the substitution
/// is the sequence number of the frequency chunk in the order of increasing frequency of the first
/// channel starting from zero. Verification of the chunk fails if the frequency of the first channel changes.
class FreqChunkSubstitutionRule : public IChunkDependentSubstitutionRule {
   public:

   /// @brief constructor
   /// @details
   /// @param[in] kw keyword string to represent
   /// @param[in] config configuration class
   FreqChunkSubstitutionRule(const std::string &kw, const Configuration &config);

   // implementation of interface mentods

   /// @brief obtain keywords handled by this object
   /// @details This method returns a set of string keywords
   /// (without leading % sign in our implementation, but in general this 
   /// can be just logical full-string keyword, we don't have to limit ourselves
   /// to particular single character tags) which this class recognises. Any of these
   /// keyword can be passed to operator() once the object is initialised
   /// @return set of keywords this object recognises
   virtual std::set<std::string> keywords() const; 

   /// @brief verify that the chunk conforms
   /// @details The class is setup once, at the time when MPI calls are allowed. This
   /// method allows to check that another (new) chunk still conforms with the original setup.
   /// The method exists only for cross-checks, it is not required to be called for correct
   /// operation of the whole framework.
   /// @param[in] chunk shared pointer to VisChunk to work with
   /// @note The chunk itself is unchanged. An exception is expected to be thrown 
   /// if the chunk doesn't conform.
   virtual void verifyChunk(const boost::shared_ptr<common::VisChunk> &chunk);

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

protected:

   /// @brief initialise the object
   /// @details This is the only place where MPI calls may happen. 
   /// In this method, the implementations are expected to provide a
   /// mechanism to obtain values for all keywords handled by this object.
   /// @param[in] chunk shared pointer to VisChunk to work with
   /// @note The chunk itself is unchanged
   virtual void initialise(const boost::shared_ptr<common::VisChunk> &chunk);

private:
   
   /// @brief keyword name handled by this class
   std::string itsKeyword;

   /// @brief frequency of the first channel in Hz for this rank for cross-checks
   casa::Double itsFreq;

   /// @brief sequence chunk number of this rank - result of substitution
   int itsFreqChunkId;

   /// @brief number of ranks (needed for delayed initialisation)
   int itsNProcs;

   /// @brief this rank number
   int itsRank;

   /// @brief rank-independence flag, setup at initialisation
   bool itsRankIndependent;
};


};
};
};
#endif // #ifndef ASKAP_CP_INGEST_FREQCHUNKSUBSTITUTIONRULE_H

