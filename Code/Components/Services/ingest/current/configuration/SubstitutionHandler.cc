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
#include <map>

// own includes
#include "configuration/SubstitutionHandler.h"
#include "askap/AskapError.h"

namespace askap {
namespace cp {
namespace ingest {

/// @brief constructor
SubstitutionHandler::SubstitutionHandler() : itsInitialiseCalled(false), itsLastRankDependent(false)
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
        const std::set<std::string> commonKeywords = intersection(new_kws, (*ci)->keywords());
        ASKAPCHECK(commonKeywords.size() == 0, "Dulicated substitution rule for keyword"<<*commonKeywords.begin());
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
   ASKAPDEBUGASSERT(itsRuleInitialised.size() == itsRules.size());
   std::vector<bool>::iterator it = itsRuleInitialised.begin();
   for (std::vector<boost::shared_ptr<ISubstitutionRule> >::const_iterator ci = itsRules.begin(); ci != itsRules.end(); ++ci,++it) {
        ASKAPDEBUGASSERT(*ci);
        ASKAPDEBUGASSERT(it !=itsRuleInitialised.end());
        const std::set<std::string> keywordsHandledByThisRule = intersection(keywords, (*ci)->keywords());
        if (keywordsHandledByThisRule.size() > 0) {
            (*ci)->initialise();
            *it = true;
        }
   }
}

/// @brief perform substitution
/// @details This method performs actual substitution. Initialisation is perfromed on-demand.
/// @param[in] in input string
/// @return processed string
std::string SubstitutionHandler::operator()(const std::string &in)
{
   const std::vector<boost::tuple<size_t, std::string, size_t> > parsedStr = parseString(in);
   std::set<size_t> rulesUsed;
   // groups used, each group number is mapped to flag (setup later) which will show whether a particular group is to be included in the result
   std::map<size_t, bool> groupsUsed;
   for (std::vector<boost::tuple<size_t, std::string, size_t> >::const_iterator ci = parsedStr.begin(); ci != parsedStr.end(); ++ci) {
        if (ci->get<0>() < itsRules.size()) {
            rulesUsed.insert(ci->get<0>());
        }
        groupsUsed[ci->get<2>()] = true;
   }
   for (std::set<size_t>::const_iterator ci = rulesUsed.begin(); ci != rulesUsed.end(); ++ci) {
        if (itsInitialiseCalled) {
            ASKAPCHECK(itsRuleInitialised[*ci], "Substitution rule number "<<*ci + 1<<" is not initialised for some reason");
        } else {
            ASKAPCHECK(!itsRuleInitialised[*ci], "Substitution rule number "<<*ci + 1<<" is already initialised, this is not expected");
            ASKAPDEBUGASSERT(itsRules[*ci]);
            itsRules[*ci]->initialise();
            itsRuleInitialised[*ci] = true;
        }
   }
   itsInitialiseCalled = true;

   // set up activity flag for each group - true for non-zero group which has rank-dependent result for at least one of the fields
   for (std::map<size_t, bool>::iterator it = groupsUsed.begin(); it != groupsUsed.end(); ++it) {
        bool isRankIndependent = true;
        for (std::vector<boost::tuple<size_t, std::string, size_t> >::const_iterator ci = parsedStr.begin(); ci != parsedStr.end(); ++ci) {
             if ((ci->get<2>() == it->first) && (ci->get<0>() < itsRules.size())) {
                 const boost::shared_ptr<ISubstitutionRule> rule = itsRules[ci->get<0>()];
                 ASKAPDEBUGASSERT(rule);
                 isRankIndependent &= rule->isRankIndependent();
             }
        }
        it->second = !isRankIndependent;
   }

   // group 0 is always active, i.e. included in the output
   // however the current value stored in groupsUsed is true if the main text is rank-dependent.
   itsLastRankDependent = groupsUsed[0];
   // now turn it to true to always pass through
   groupsUsed[0] = true;

   std::string result;
   for (std::vector<boost::tuple<size_t, std::string, size_t> >::const_iterator ci = parsedStr.begin(); ci != parsedStr.end(); ++ci) {
        const std::map<size_t, bool>::const_iterator grpIt = groupsUsed.find(ci->get<2>());
        ASKAPDEBUGASSERT(grpIt != groupsUsed.end());
        if (grpIt->second) {
            if (ci->get<0>() < itsRules.size()) {
                const boost::shared_ptr<ISubstitutionRule> rule = itsRules[ci->get<0>()];
                ASKAPDEBUGASSERT(rule);
                result += rule->operator()(ci->get<1>());
            } else {
                result += ci->get<1>();
            }
            if (ci->get<2>() != 0) {
                itsLastRankDependent = true;
            }
        }
   }
   
   return result;
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
/// The flag controlling whether to add a particular string is a number indicating a group
/// of values which should be treated together. A value of zero means always add the group.
/// @param[in] in string to parse
/// @return a vector of tuples (as described above) in the order items are in the input string
std::vector<boost::tuple<size_t, std::string, size_t> > SubstitutionHandler::parseString(const std::string &in) const
{
   std::vector<boost::tuple<size_t, std::string, size_t> > result;
   size_t currentGroup = 0;
   size_t nGroups = 1;
   
   for (size_t cursor = 0; cursor < in.size(); ++cursor) {
        size_t pos = in.find("%",cursor);

        if (pos == std::string::npos) {
            ASKAPCHECK(currentGroup == 0, "Error parsing string '"<<in<<"' - no matching %}");
            result.push_back(boost::tuple<size_t, std::string, size_t>(itsRules.size(),in.substr(cursor), currentGroup));
            break;
        }
        if (pos != cursor) {
            ASKAPDEBUGASSERT(pos > cursor);
            result.push_back(boost::tuple<size_t, std::string, size_t>(itsRules.size(),in.substr(cursor, pos - cursor), currentGroup));
        }
        if (++pos == in.size()) {
            ASKAPCHECK(currentGroup == 0, "Error parsing string '"<<in<<"' - no matching %}");
            result.push_back(boost::tuple<size_t, std::string, size_t>(itsRules.size(),in.substr(pos-1,1), currentGroup));
            break;
        }
        // all checks below are working with the text past the initial '%' symbol 
        if (in[pos] == '%') {
            result.push_back(boost::tuple<size_t, std::string, size_t>(itsRules.size(),in.substr(pos,1), currentGroup));
        } else if (in[pos] == '{') {
            ASKAPCHECK(currentGroup == 0, "Encountered nested %{ %} in "<<in);
            currentGroup = nGroups++;
        } else if (in[pos] == '}') {
            ASKAPCHECK(currentGroup > 0, "Encountered %} without openning bracket in "<<in);
            currentGroup = 0;
        } else {

            // look for matching keywords
            bool matchFound = false;
            for (size_t index = 0; index < itsRules.size() && !matchFound; ++index) {
                 const boost::shared_ptr<ISubstitutionRule> rule = itsRules[index];
                 ASKAPDEBUGASSERT(rule);
                 const std::set<std::string> kw = rule->keywords();
                 for (std::set<std::string>::const_iterator ci = kw.begin(); ci != kw.end(); ++ci) {
                      if (in.find(*ci, pos) == pos) {
                          ASKAPASSERT(ci->size() > 0);
                          pos += ci->size() - 1;
                          ASKAPDEBUGASSERT(pos < in.size());
                          matchFound = true;
                          // reference the rule - match found
                          result.push_back(boost::tuple<size_t, std::string, size_t>(index,*ci, currentGroup));
                          break;
                      }
                 }
            } 

            if (!matchFound) {
                // unrecognised keyword, pass it as is
                ASKAPDEBUGASSERT(pos < in.size());
                ASKAPDEBUGASSERT(pos > 0);
                size_t posend = in.find("%", pos);
                size_t length = posend != std::string::npos ? posend - pos + 1 : in.size() - pos + 1;
                result.push_back(boost::tuple<size_t, std::string, size_t>(itsRules.size(),in.substr(pos-1,length), currentGroup));
                ASKAPDEBUGASSERT(length > 1);
                pos += length - 2;
            }
       }
       cursor = pos;
   }
   ASKAPCHECK(currentGroup == 0, "Error parsing string '"<<in<<"' - no matching %}");
   return result;
}

/// @brief helper method to extract a set of used keywords
/// @details It turns the vector of tuples returned by parseString into a set of keywords
/// @param[in] vec vector of 3-element tuples as provided by parseString
/// @return a set of used kewords (aggregation of all second parameters if they are not 
/// representing an explicit string
std::set<std::string> SubstitutionHandler::extractKeywords(const std::vector<boost::tuple<size_t, std::string, size_t> > &vec) const
{
   std::set<std::string> result;
   for (std::vector<boost::tuple<size_t, std::string, size_t> >::const_iterator ci = vec.begin(); ci != vec.end(); ++ci) {
        if (ci->get<0>() < itsRules.size()) {
            result.insert(ci->get<1>());
        }
   }
   return result;
}

/// @brief helper method to compute intersection of two sets
/// @details This is a wrapper on top of set_intersection from algorithms. 
/// @param[in] s1 first set
/// @param[in] s2 second set
/// @return intersection of s1 and s2 (i.e. set with common elements)
std::set<std::string> SubstitutionHandler::intersection(const std::set<std::string> &s1, const std::set<std::string> &s2)
{
   std::set<std::string> result;
   std::set_intersection(s1.begin(), s1.end(), s2.begin(), s2.end(), std::inserter(result, result.begin()));
   return result;
}

}
}
}

