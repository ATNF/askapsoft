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
#include "askap/askap/AskapUtil.h"

using namespace std;

namespace askap {
namespace cp {
namespace ingest {

class SubstitutionHandlerTest : public CppUnit::TestFixture {
        CPPUNIT_TEST_SUITE(SubstitutionHandlerTest);
        CPPUNIT_TEST(testVoidSubstitution);
        CPPUNIT_TEST(testIntersection);
        CPPUNIT_TEST(testExtractKeywords);
        CPPUNIT_TEST(testParseString);
        CPPUNIT_TEST_EXCEPTION(testParseStringOpenGroup1, AskapError);
        CPPUNIT_TEST_EXCEPTION(testParseStringOpenGroup2, AskapError);
        CPPUNIT_TEST_EXCEPTION(testParseStringOpenGroup3, AskapError);
        CPPUNIT_TEST(testExtractKeywords2);
        CPPUNIT_TEST(testSubstitution);
        CPPUNIT_TEST_EXCEPTION(testPartialInitialisation, AskapError);
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
             CPPUNIT_ASSERT(!sh.lastSubstitutionRankDependent());
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

        void testParseString() {
             ModifiedSubstitutionHandler msh;
             TestRule tr("test", "result", false);
             msh.add(boost::shared_ptr<TestRule>(&tr, utility::NullDeleter()));

             const std::string testStr = "test_val=%test%d%%test%{_val=%test%}";
             std::vector<boost::tuple<size_t, std::string, size_t> > vec = msh.parseString(testStr);
             CPPUNIT_ASSERT_EQUAL(size_t(7u),vec.size());
             //for (size_t index = 0; index < vec.size(); ++index) {
             //     std::cout<<"item "<<index + 1<<": `"<<vec[index].get<1>()<<"` ref="<<vec[index].get<0>()<<" group="<<vec[index].get<2>()<<std::endl;
             //}
             for (size_t index = 0; index < vec.size(); ++index) {
                  CPPUNIT_ASSERT_EQUAL(size_t(index == 1 || index == 6 ? 0u : 1u), vec[index].get<0>());
                  CPPUNIT_ASSERT_EQUAL(size_t(index >= 5 ? 1u : 0u), vec[index].get<2>());
             }
             CPPUNIT_ASSERT_EQUAL(std::string("test_val="), vec[0].get<1>());
             CPPUNIT_ASSERT_EQUAL(std::string("test"), vec[1].get<1>());
             CPPUNIT_ASSERT_EQUAL(std::string("%d"), vec[2].get<1>());
             CPPUNIT_ASSERT_EQUAL(std::string("%"), vec[3].get<1>());
             CPPUNIT_ASSERT_EQUAL(std::string("test"), vec[4].get<1>());
             CPPUNIT_ASSERT_EQUAL(std::string("_val="), vec[5].get<1>());
             CPPUNIT_ASSERT_EQUAL(std::string("test"), vec[6].get<1>());
            
             vec = msh.parseString("%test%");
             CPPUNIT_ASSERT_EQUAL(size_t(2u),vec.size());
             for (size_t index = 0; index < vec.size(); ++index) {
                  CPPUNIT_ASSERT_EQUAL(size_t(index == 0 ? 0u : 1u), vec[index].get<0>());
                  CPPUNIT_ASSERT_EQUAL(size_t(0u), vec[index].get<2>());
             }
             CPPUNIT_ASSERT_EQUAL(std::string("test"), vec[0].get<1>());
             CPPUNIT_ASSERT_EQUAL(std::string("%"), vec[1].get<1>());

             vec = msh.parseString("%d%");
             CPPUNIT_ASSERT_EQUAL(size_t(2u),vec.size());
             for (size_t index = 0; index < vec.size(); ++index) {
                  CPPUNIT_ASSERT_EQUAL(size_t(1u), vec[index].get<0>());
                  CPPUNIT_ASSERT_EQUAL(size_t(0u), vec[index].get<2>());
             }
             CPPUNIT_ASSERT_EQUAL(std::string("%d"), vec[0].get<1>());
             CPPUNIT_ASSERT_EQUAL(std::string("%"), vec[1].get<1>());

             vec = msh.parseString("%{%test%}%{%test%}");
             CPPUNIT_ASSERT_EQUAL(size_t(2u),vec.size());
             for (size_t index = 0; index < vec.size(); ++index) {
                  CPPUNIT_ASSERT_EQUAL(size_t(0u), vec[index].get<0>());
                  CPPUNIT_ASSERT_EQUAL(index+1, vec[index].get<2>());
                  CPPUNIT_ASSERT_EQUAL(std::string("test"), vec[index].get<1>());
             }

             vec = msh.parseString("%{%testing%}");
             CPPUNIT_ASSERT_EQUAL(size_t(2u),vec.size());
             for (size_t index = 0; index < vec.size(); ++index) {
                  CPPUNIT_ASSERT_EQUAL(size_t(index == 0 ? 0u : 1u), vec[index].get<0>());
                  CPPUNIT_ASSERT_EQUAL(size_t(1u), vec[index].get<2>());
             }
             CPPUNIT_ASSERT_EQUAL(std::string("test"), vec[0].get<1>());
             CPPUNIT_ASSERT_EQUAL(std::string("ing"), vec[1].get<1>());
        }

        void testParseStringOpenGroup1() {
             ModifiedSubstitutionHandler msh;
       
             // this would throw the exception
             msh.parseString("%{%test%");
        }

        void testParseStringOpenGroup2() {
             ModifiedSubstitutionHandler msh;
       
             // this would throw the exception
             msh.parseString("%}");
        }

        void testParseStringOpenGroup3() {
             ModifiedSubstitutionHandler msh;
       
             // this would throw the exception
             msh.parseString("%{%test=1%}%}");
        }

        void testExtractKeywords2() {
             SubstitutionHandler sh;
             TestRule tr1("test", "result", false);
             TestRule tr2("val", "result", false);
             sh.add(boost::shared_ptr<TestRule>(&tr1, utility::NullDeleter()));
             sh.add(boost::shared_ptr<TestRule>(&tr2, utility::NullDeleter()));

             std::set<std::string> kws = sh.extractKeywords("test_val=%test%d%%test%{_val=%test%}");
             CPPUNIT_ASSERT_EQUAL(size_t(1u), kws.size());
             CPPUNIT_ASSERT_EQUAL(std::string("test"), *kws.begin());
             
             kws = sh.extractKeywords("test_val=%test%d%%test%{_val=%val%}");
             CPPUNIT_ASSERT_EQUAL(size_t(2u), kws.size());
             CPPUNIT_ASSERT(kws.find("test") != kws.end());
             CPPUNIT_ASSERT(kws.find("val") != kws.end());
        }

        void testSubstitution() {
             SubstitutionHandler sh;
             // pretend that only the first one os rank-independent
             TestRule tr1("test", "result", true);
             TestRule tr2("val", "val", false);
             sh.add(boost::shared_ptr<TestRule>(&tr1, utility::NullDeleter()));
             sh.add(boost::shared_ptr<TestRule>(&tr2, utility::NullDeleter()));
             // get all keywords initialised first, otherwise the order of tests would matter
             // as operator() initialised only the rules which are necessary and assumes it is done only
             // once (so initialisation can include MPI collective calls)
             std::set<std::string> kws = tr1.keywords();
             const std::set<std::string> kws2 = tr2.keywords();
             kws.insert(kws2.begin(), kws2.end());
             CPPUNIT_ASSERT_EQUAL(size_t(2u), kws.size());
             sh.initialise(kws);

             // actual tests
             CPPUNIT_ASSERT_EQUAL(std::string("val=result%d%test"), sh("val=%test%d%%test%{_val=%test%}"));
             CPPUNIT_ASSERT(!sh.lastSubstitutionRankDependent());
             CPPUNIT_ASSERT_EQUAL(std::string("val=result%d%test_val=val"), sh("val=%test%d%%test%{_val=%val%}"));
             CPPUNIT_ASSERT(sh.lastSubstitutionRankDependent());
             CPPUNIT_ASSERT_EQUAL(std::string("val=result%d%test_val=resultval"), sh("val=%test%d%%test%{_val=%test%val%}"));
             CPPUNIT_ASSERT(sh.lastSubstitutionRankDependent());
             CPPUNIT_ASSERT_EQUAL(std::string("no_val_resultval"), sh("%{_%test%}%{no_%val%}_%{%test%val%}"));
             CPPUNIT_ASSERT(sh.lastSubstitutionRankDependent());
             CPPUNIT_ASSERT_EQUAL(std::string("result"), sh("%test"));
             CPPUNIT_ASSERT(!sh.lastSubstitutionRankDependent());
             CPPUNIT_ASSERT_EQUAL(std::string("val"), sh("%val"));
             CPPUNIT_ASSERT(sh.lastSubstitutionRankDependent());
        }

        void testPartialInitialisation() {
             SubstitutionHandler sh;
             // pretend that only the first one os rank-independent
             TestRule tr1("test", "result", true);
             TestRule tr2("val", "val", false);
             sh.add(boost::shared_ptr<TestRule>(&tr1, utility::NullDeleter()));
             sh.add(boost::shared_ptr<TestRule>(&tr2, utility::NullDeleter()));
             
             CPPUNIT_ASSERT_EQUAL(size_t(0u), sh("%{%test%}").size());
             // now only first rule should be initialised (although parsing is inhibited by rank-independence)
             // so the string which requires the second rule would cause an exception
             sh("%val");
        }
};

}   // End namespace ingest
}   // End namespace cp
}   // End namespace askap
