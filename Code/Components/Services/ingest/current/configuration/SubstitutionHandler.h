/// @file SubstitutionHandler.h
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

#ifndef ASKAP_CP_INGEST_SUBSTITUTIONHANDLER_H
#define ASKAP_CP_INGEST_SUBSTITUTIONHANDLER_H

// System include
#include <string>
#include <vector>
#include <set>

// own includes
#include "configuration/ISubstitutionRule.h"

// boost include
#include <boost/noncopyable.hpp>
#include <boost/shared_ptr.hpp>
#include <boost/tuple/tuple.hpp>

namespace askap {
namespace cp {
namespace ingest {

/// @brief Class to handle (file) name substitutions
/// @details This is a generic class handling substitutions according to rules given by one or
/// more ISubstitutionRule instances. Classes representing actual rules do necessary aggregation
/// via MPI, if required. This class is sufficiently generic and could be moved to Base eventually.
/// 
/// The symbols following % are compared with supported keywords and processing is done accordingly.
/// In addition %% (double per-cent sign) is translated to single % without affecting the following string.
/// Also %{ and %} brackets mean to omit the substring included in the brackets if it is the same for all
/// ranks. 
class SubstitutionHandler : public boost::noncopyable {
public:

   /// @brief constructor
   SubstitutionHandler();

   /// @brief add a new substitution rule
   /// @details Before this class can be used, all supported substitute rules 
   /// have to be added. The order doesn't matter as duplicated keywords are not supported.
   /// @param[in] rule shared pointer to the rule to add
   void add(const boost::shared_ptr<ISubstitutionRule> &rule);

   /// @brief initialise substitution
   /// @details After all rules are setup and while MPI collectives can be used, this class
   /// must be initialised. Later on, access to operator() would do the substitution based on
   /// cached values.  It is possible not to call this method explicitly. It will be called on
   /// the first invocation of the operator(). However, one must ensure that MPI collectives can
   /// be used there. 
   /// @param[in] keywords a set of keywords to initialise
   void initialise(const std::set<std::string> &keywords);

   /// @brief perform substitution
   /// @details This method performs actual substitution. Initialisation is perfromed on-demand.
   /// @param[in] in input string
   /// @return processed string
   std::string operator()(const std::string &in);

   /// @brief extract all keywords used in the given string
   /// @param[in] in input string
   /// @return set of used keywords
   std::set<std::string> extractKeywords(const std::string &in) const; 

protected:
   /// @brief parse string taking current rules into account
   /// @details This is the method with the main parsing logic. It
   /// decomposes the supplied string into a vector of references
   /// to the rule (by index in itsRules), keywords and flags showing
   /// whether the value should be included if turns out to be the same
   /// for all ranks. These 3 values are presented in a tuple. If the 
   /// index into itsRules (the first parameter) happens to be equal to
   /// itsRules.size() then the "keyword" is the string which should be added as is.
   /// The flag controlling whether to add a particular string is a number indicating a group
   /// of values which should be treated together. A value of zero means always add the group.
   /// @param[in] in string to parse
   /// @return a vector of tuples (as described above) in the order items are in the input string
   std::vector<boost::tuple<size_t, std::string, size_t> > parseString(const std::string &in) const;

   /// @brief helper method to extract a set of used keywords
   /// @details It turns the vector of tuples returned by parseString into a set of keywords
   /// @param[in] vec vector of 3-element tuples as provided by parseString
   /// @return a set of used kewords (aggregation of all second parameters if they are not 
   /// representing an explicit string
   std::set<std::string> extractKeywords(const std::vector<boost::tuple<size_t, std::string, size_t> > &vec) const;

   /// @brief helper method to compute intersection of two sets
   /// @details This is a wrapper on top of set_intersection from algorithms. 
   /// @param[in] s1 first set
   /// @param[in] s2 second set
   /// @return intersection of s1 and s2 (i.e. set with common elements)
   static std::set<std::string> intersection(const std::set<std::string> &s1, const std::set<std::string> &s2);

private:
   /// @brief substitution rules
   std::vector<boost::shared_ptr<ISubstitutionRule> > itsRules;

   /// @brief flag that corresponding rule has been initialised
   /// @details This is used for cross-checks only, we could've called the appropriate
   /// initialise() method without checking. There is one to one correspondence 
   /// with itsRules, true means that the appropriate rule has been initialised
   std::vector<bool> itsRuleInitialised;

   /// @brief true if initialise method is called
   bool itsInitialiseCalled;
};


};
};
};
#endif // #ifndef ASKAP_CP_INGEST_SUBSTITUTIONHANDLER_H

