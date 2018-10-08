/// @file SubstitutionHandler.cc
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

// std includes
#include <algorithm>

// own includes
#include "configuration/SubstitutionHandler.h"
#include "askap/AskapError.h"

namespace askap {
namespace cp {
namespace ingest {

/// @brief constructor
SubstitutionHandler::SubstitutionHandler() : itsInitialiseCalled(false) 
{
   itsRules.reserve(10);
   itsRuleInitialised.reserve(10);
}

/// @brief add a new substitution rule
/// @details Before this class can be used, all supported substitute rules 
/// have to be added. The order doesn't matter as duplicated keywords are not supported.
/// @param[in] rule shared pointer to the rule to add
void SubstitutionHandler::add(const boost::shared_ptr<ISubstitutionRule> &rule)
{
   ASKAPCHECK(rule, "Expect non-zero shared pointer with the substitution rule");
   ASKAPCHECK(!itsInitialiseCalled, "An attempt to add a new substitution rule after the handler has already been initialised");
   // cross-check to avoid duplicated keywords
   const std::set<std::string> new_kws = rule->keywords();
   for (std::vector<boost::shared_ptr<ISubstitutionRule> >::const_iterator ci = itsRules.begin(); ci != itsRules.end(); ++ci) {
        ASKAPDEBUGASSERT(*ci);
        const std::set<std::string> kws = (*ci)->keywords();
        for (std::set<std::string>::const_iterator kwIt1 = kws.begin(); kwIt1 != kws.end(); ++kwIt1) {
             for (std::set<std::string>::const_iterator kwIt2 = new_kws.begin(); kwIt2 != new_kws.end(); ++kwIt2) {
                  ASKAPCHECK(*kwIt1 != *kwIt2, "Dulicated substitution rule for keyword '"<<*kwIt1<<"'");
             }
        }
   }
   //
   itsRules.push_back(rule);
   itsRuleInitialised.push_back(false);
}

/// @brief initialise substitution
/// @details After all rules are setup and while MPI collectives can be used, this class
/// must be initialised. Later on, access to operator() would do the substitution based on
/// cached values.  It is possible not to call this method explicitly. It will be called on
/// the first invocation of the operator(). However, one must ensure that MPI collectives can
/// be used there. 
/// @param[in] keywords a set of keywords to initialise
void SubstitutionHandler::initialise(const std::set<std::string> &keywords)
{
   ASKAPCHECK(!itsInitialiseCalled, "SubstitutionHandler::initialise is supposed to be called only once");
   itsInitialiseCalled = true;
   // initialisation flags, if true the particular rule is in use and needs initialisation
   std::vector<bool> initFlags(itsRules.size(), false);
   std::vector<bool>::iterator it = initFlags.begin();
   for (std::vector<boost::shared_ptr<ISubstitutionRule> >::const_iterator ci = itsRules.begin(); ci != itsRules.end(); ++ci,++it) {
        ASKAPDEBUGASSERT(*ci);
        ASKAPDEBUGASSERT(it != initFlags.end());
        const std::set<std::string> kws = (*ci)->keywords();
        for (std::set<std::string>::const_iterator kwIt1 = kws.begin(); kwIt1 != kws.end(); ++kwIt1) {
             for (std::set<std::string>::const_iterator kwIt2 = keywords.begin(); kwIt2 != keywords.end(); ++kwIt2) {
                  if (*kwIt1 == *kwIt2) {
                      
                  }
             }
        }
   }
}

/// @brief perform substitution
/// @details This method performs actual substitution. Initialisation is perfromed on-demand.
/// @param[in] in input string
/// @return processed string
std::string SubstitutionHandler::operator()(const std::string &in)
{

   return in;
}

/// @brief extract all keywords used in the given string
/// @param[in] in input string
/// @return set of used keywords
std::set<std::string> SubstitutionHandler::extractKeywords(const std::string &in) const
{
   return extractKeywords(parseString(in));
}

/// @brief parse string taking current rules into account
/// @details This is the method with the main parsing logic. It
/// decomposes the supplied string into a vector of references
/// to the rule (by index in itsRules), keywords and flags showing
/// whether the value should be included if turns out to be the same
/// for all ranks. These 3 values are presented in a tuple. If the 
/// index into itsRules (the first parameter) happens to be equal to
/// itsRules.size() then the "keyword" is the string which should be added as is.
/// @param[in] in string to parse
/// @return a vector of tuples (as described above) in the order items are in the input string
std::vector<boost::tuple<size_t, std::string, bool> > SubstitutionHandler::parseString(const std::string &in) const
{
   return std::vector<boost::tuple<size_t, std::string, bool> >();
}

/// @brief helper method to extract a set of used keywords
/// @details It turns the vector of tuples returned by parseString into a set of keywords
/// @param[in] vec vector of 3-element tuples as provided by parseString
/// @return a set of used kewords (aggregation of all second parameters if they are not 
/// representing an explicit string
std::set<std::string> SubstitutionHandler::extractKeywords(const std::vector<boost::tuple<size_t, std::string, bool> > &vec)
{
   return std::set<std::string>();
}


}
}
}

