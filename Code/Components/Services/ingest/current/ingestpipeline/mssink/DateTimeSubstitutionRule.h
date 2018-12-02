/// @file DateTimeSubstitutionRule.h
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

#ifndef ASKAP_CP_INGEST_DATETIMESUBSTITUTIONRULE_H
#define ASKAP_CP_INGEST_DATETIMESUBSTITUTIONRULE_H

// ASKAPsoft includes
#include "configuration/ISubstitutionRule.h"
#include "configuration/Configuration.h"

// std includes
#include <map>

namespace askap {
namespace cp {
namespace ingest {

/// @brief Substitute date and time
/// @details This rule figures out the common current time on all ranks and provides
/// consistent substitution across all ranks.
class DateTimeSubstitutionRule : public ISubstitutionRule {
   public:

   /// @brief constructor
   /// @details
   /// @param[in] config configuration class
   /// @param[in] kwDate optional date keyword
   /// @param[in] kwTime optional time keyword
   explicit DateTimeSubstitutionRule(const Configuration &config, const std::string &kwDate = "d",
                                     const std::string &kwTime = "t");

   // implementation of interface mentods

   /// @brief obtain keywords handled by this object
   /// @details This method returns a set of string keywords
   /// (without leading % sign in our implementation, but in general this 
   /// can be just logical full-string keyword, we don't have to limit ourselves
   /// to particular single character tags) which this class recognises. Any of these
   /// keyword can be passed to operator() once the object is initialised
   /// @return set of keywords this object recognises
   virtual std::set<std::string> keywords() const; 

   /// @brief initialise the object
   /// @details This is the only place where MPI calls may happen. Therefore, 
   /// initialisation has to be done at the appropriate time in the program.
   /// It is also expected that only substitution rules which are actually needed
   /// will be initialised  and used. So construction/destruction should be a light
   /// operation. In this method, the implementations are expected to provide a
   /// mechanism to obtain values for all keywords handled by this object.
   virtual void initialise();

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
   
   /// @brief make two-character string
   /// @details Helper method to convert unsigned integer into a 2-character string.
   /// It is used to represent date and time in a more readable format
   /// @param[in] in input number
   /// @return two-element string
   static std::string makeTwoElementString(const unsigned int in);

private:
   
   /// @brief number of ranks (needed for delayed initialisation)
   int itsNProcs;

   /// @brief this rank number
   int itsRank;

   /// @brief date keyword
   std::string itsDateKeyword;

   /// @brief time keyword
   std::string itsTimeKeyword;

   /// @brief map with results
   std::map<std::string, std::string> itsResult;
};


};
};
};
#endif // #ifndef ASKAP_CP_INGEST_DATETIMESUBSTITUTIONRULE_H

