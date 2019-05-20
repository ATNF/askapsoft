/// @file DateTimeSubstitutionRule.cc
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

// ASKAPsoft includes
#include "ingestpipeline/mssink/DateTimeSubstitutionRule.h"
#include "askap/askap/AskapUtil.h"
#include "askap/askap/AskapError.h"

// it would be nice to get all MPI stuff in a single place, but ingest is already MPI-heavy throughout
#include <mpi.h>

// casa includes
#include "casacore/casa/OS/Time.h"


namespace askap {
namespace cp {
namespace ingest {

/// @brief constructor
/// @details
/// @param[in] config configuration class
/// @param[in] kwDate optional date keyword
/// @param[in] kwTime optional time keyword
DateTimeSubstitutionRule::DateTimeSubstitutionRule(const Configuration &config, const std::string &kwDate,
                                     const std::string &kwTime) :
       itsNProcs(config.nprocs()), itsRank(config.rank()), itsDateKeyword(kwDate), itsTimeKeyword(kwTime)
{
   ASKAPASSERT(itsRank < itsNProcs);
   ASKAPASSERT(itsNProcs > 0);
   // placeholder for the result, doing it this way allows us to be more dynamic in other methods
   // just in case it is handy in the future
   itsResult[kwDate] = "";
   itsResult[kwTime] = "";
}

// implementation of interface mentods

/// @brief obtain keywords handled by this object
/// @details This method returns a set of string keywords
/// (without leading % sign in our implementation, but in general this 
/// can be just logical full-string keyword, we don't have to limit ourselves
/// to particular single character tags) which this class recognises. Any of these
/// keyword can be passed to operator() once the object is initialised
/// @return set of keywords this object recognises
std::set<std::string> DateTimeSubstitutionRule::keywords() const
{
   // there is probably a more elegant way to do this (e.g. with a custom iterator), but it is not much of an overhead
   std::set<std::string> result;
   for (std::map<std::string, std::string>::const_iterator ci = itsResult.begin(); ci != itsResult.end(); ++ci) {
        result.insert(ci->first);
   }
   return result;
}

/// @brief initialise the object
/// @details This is the only place where MPI calls may happen. Therefore, 
/// initialisation has to be done at the appropriate time in the program.
/// It is also expected that only substitution rules which are actually needed
/// will be initialised  and used. So construction/destruction should be a light
/// operation. In this method, the implementations are expected to provide a
/// mechanism to obtain values for all keywords handled by this object.
void DateTimeSubstitutionRule::initialise()
{
   // special structure to have full control over it, otherwise could've used casacore::Time
   struct TimeBuf {
     casacore::uInt year;
     casacore::uInt month;
     casacore::uInt day;
     casacore::uInt hour;
     casacore::uInt min;
     casacore::uInt sec;

     // technically we don't need the constructor, but it is neater to have it
     TimeBuf() : year(0), month(0), day(0), hour(0), min(0), sec(0) {}
   };

   // The call to initialise method implies that date/time is used in the requested string
   TimeBuf tbuf;
   if ( (itsNProcs == 1) || (itsRank == 0) ) {
         casacore::Time tm;
         tm.now();
         tbuf.year = tm.year();
         tbuf.month = tm.month();
         tbuf.day = tm.dayOfMonth();
         tbuf.hour = tm.hours();
         tbuf.min = tm.minutes();
         tbuf.sec = tm.seconds();
   }
   if (itsNProcs > 1) {
       // distributed case - broadcast the value to all ranks
       const int response = MPI_Bcast((void*)&tbuf, sizeof(tbuf), MPI_INTEGER, 0, MPI_COMM_WORLD);
       ASKAPCHECK(response == MPI_SUCCESS, "Erroneous response from MPI_Bcast = "<<response);
   }
   // all ranks now have consistent tbuf structure with the same values
   // now form the result strings:
   //      date in YYYY-MM-DD format and time in HHMMSS format.
   ASKAPDEBUGASSERT(itsResult.size() == 2);
   itsResult[itsDateKeyword] = utility::toString(tbuf.year)+"-"+makeTwoElementString(tbuf.month)+"-"+makeTwoElementString(tbuf.day);
   itsResult[itsTimeKeyword] = makeTwoElementString(tbuf.hour) + makeTwoElementString(tbuf.min) + makeTwoElementString(tbuf.sec);
   // if we need to split results / have different formats, we can add the more options here, they're used on-demand
}

/// @brief obtain value of a particular keyword
/// @details This is the main access method which is supposed to be called after
/// initialise(). 
/// @param[in] kw keyword to access, must be from the set returned by keywords
/// @return value of the requested keyword
/// @note  An exception may be thrown if the initialise() method is not called
/// prior to an attempt to access the value.
std::string DateTimeSubstitutionRule::operator()(const std::string &kw) const
{
   const std::map<std::string, std::string>::const_iterator ci = itsResult.find(kw);
   ASKAPCHECK(ci != itsResult.end(), "Attempted to obtain keyword '"<<kw<<"' from DateTimeSubstitutionRule");
   return ci->second;
}

/// @brief check if values are rank-independent
/// @details The implementation of this interface should evaluate a flag and return it
/// in this method to show whether the value for a particular keyword is
/// rank-independent or not. This is required to encapsulate all MPI related calls in
/// the initialise. Sometimes, the value of the flag can be known up front, e.g. if
/// the value is the result of gather-scatter operation or if it is based on rank number.
/// @param[in] kw keyword to check the flag for
/// @return true, if the given keyword has the same value for all ranks
bool DateTimeSubstitutionRule::isRankIndependent() const
{
   // we do gather as part of the algorithm, so it is by design
   return true;
};

/// @brief make two-character string
/// @details Helper method to convert unsigned integer into a 2-character string.
/// It is used to represent date and time in a more readable format
/// @param[in] in input number
/// @return two-element string
std::string DateTimeSubstitutionRule::makeTwoElementString(const unsigned int in)
{
   ASKAPASSERT(in<100);
   std::string result;
   if (in<10) {
       result += "0";
   }
   result += utility::toString(in);
   return result;
}



};
};
};

