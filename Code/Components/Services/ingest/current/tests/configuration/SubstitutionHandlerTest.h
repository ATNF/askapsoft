/// @file SubstitutionHandlerTest.h
///
/// @copyright (c) 2011 CSIRO
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

// CPPUnit includes
#include <cppunit/extensions/HelperMacros.h>

// Support classes
#include <string>
#include <set>
#include <boost/tuple/tuple.hpp>
#include <boost/shared_ptr.hpp>

// Classes to test
#include "configuration/SubstitutionHandler.h"
#include "configuration/ISubstitutionRule.h"

// other ASKAPsoft includes
#include "askap/AskapUtil.h"

using namespace std;

namespace askap {
namespace cp {
namespace ingest {

class SubstitutionHandlerTest : public CppUnit::TestFixture {
        CPPUNIT_TEST_SUITE(SubstitutionHandlerTest);
        CPPUNIT_TEST(testVoidSubstitution);
        CPPUNIT_TEST(testIntersection);
        CPPUNIT_TEST(testExtractKeywords);
        CPPUNIT_TEST_SUITE_END();

        // helper class to get access to protected data members
        struct ModifiedSubstitutionHandler : public SubstitutionHandler {
            using SubstitutionHandler::parseString;
            using SubstitutionHandler::extractKeywords;
            using SubstitutionHandler::intersection;
        };

        // helper class to represent a rule with pre-defined behaviour
        struct TestRule : public ISubstitutionRule {

            TestRule(const std::string &kw, const std::string &val, bool rankIndependent) :
                  itsKeyword(kw), itsValue(val), itsRankIndependent(rankIndependent) {}

             virtual std::set<std::string> keywords() const  { 
                  std::set<std::string> res; 
                  res.insert(itsKeyword);
                  return res;
             }

             virtual void initialise()  {}

             virtual std::string operator()(const std::string &kw) const 
             {
                 CPPUNIT_ASSERT_EQUAL(itsKeyword, kw);
                 return itsValue;
             }

              virtual bool isRankIndependent() const { return itsRankIndependent; }
        private:
             std::string itsKeyword;
             std::string itsValue;
             bool itsRankIndependent;
        };

    public:
        

        void testVoidSubstitution() {
             SubstitutionHandler sh;
             // void stubstitution - no keywords are setup
             const std::set<std::string> kws = sh.extractKeywords("Test%d_%t_%s_NoSubstitution");
             CPPUNIT_ASSERT_EQUAL(size_t(0u), kws.size());
     
             const std::string testStr = "ThisString_%s_Should_%d_BePassedAsIs";
             CPPUNIT_ASSERT_EQUAL(testStr, sh(testStr));
        };

        void testIntersection() {
             std::set<std::string> s1;
             std::set<std::string> s2;
             s1.insert("test");
             s1.insert("kw1");
             s1.insert("kw2");
             s1.insert("kw3");

             std::set<std::string> res = ModifiedSubstitutionHandler::intersection(s1,s2);
             CPPUNIT_ASSERT_EQUAL(size_t(0u), res.size());
             
             s2.insert("not_quite_a_test");

             res = ModifiedSubstitutionHandler::intersection(s1,s2);
             CPPUNIT_ASSERT_EQUAL(size_t(0u), res.size());

             s2.insert("kw5");

             res = ModifiedSubstitutionHandler::intersection(s1,s2);
             CPPUNIT_ASSERT_EQUAL(size_t(0u), res.size());

             s2.insert("kw2");

             res = ModifiedSubstitutionHandler::intersection(s1,s2);
             CPPUNIT_ASSERT_EQUAL(size_t(1u), res.size());
             CPPUNIT_ASSERT_EQUAL(std::string("kw2"), *res.begin());

             s2.insert("kw6");

             res = ModifiedSubstitutionHandler::intersection(s1,s2);
             CPPUNIT_ASSERT_EQUAL(size_t(1u), res.size());
             CPPUNIT_ASSERT_EQUAL(std::string("kw2"), *res.begin());

             s2.insert("test");
             res = ModifiedSubstitutionHandler::intersection(s1,s2);
             CPPUNIT_ASSERT_EQUAL(size_t(2u), res.size());
             CPPUNIT_ASSERT(res.find("kw2") != res.end());
             CPPUNIT_ASSERT(res.find("test") != res.end());
        }
      
        void testExtractKeywords() {
             ModifiedSubstitutionHandler msh;
             TestRule tr("test", "result", false);
             msh.add(boost::shared_ptr<TestRule>(&tr, utility::NullDeleter()));
             
             // fake parsed string
             std::vector<boost::tuple<size_t, std::string, size_t> > vec;
             vec.push_back(boost::tuple<size_t, std::string, size_t>(1u, "TestStr",0u));
             vec.push_back(boost::tuple<size_t, std::string, size_t>(1u, "%",0u));
             vec.push_back(boost::tuple<size_t, std::string, size_t>(1u, "_Or_",1u));
             vec.push_back(boost::tuple<size_t, std::string, size_t>(0u, "test",1u));
             vec.push_back(boost::tuple<size_t, std::string, size_t>(1u, "_Value",0u));
             vec.push_back(boost::tuple<size_t, std::string, size_t>(0u, "somethingelse",0u));

             const std::set<std::string> res = msh.extractKeywords(vec);
             CPPUNIT_ASSERT_EQUAL(size_t(2u), res.size());
             CPPUNIT_ASSERT(res.find("test") != res.end());
             CPPUNIT_ASSERT(res.find("somethingelse") != res.end());
        }
};

}   // End namespace ingest
}   // End namespace cp
}   // End namespace askap
