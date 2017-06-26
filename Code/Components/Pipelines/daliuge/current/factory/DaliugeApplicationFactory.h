/// @file PrimaryBeamFactory.h
///
/// @details
/// PrimaryBeamFactory: Factory class for Primary Beam Responses


#ifndef ASKAP_DALIUGE_APPLICATION_FACTORY_H_
#define ASKAP_DALIUGE_APPLICATION_FACTORY_H_

// System includes
#include<map>

// ASKAPsoft includes

#include <boost/shared_ptr.hpp>

// Local package includes
#include <daliuge/DaliugeApplication.h>


namespace askap
{

    /// @brief Factory class for Daliuge Applications


    class DaliugeApplicationFactory
    {
    public:
      /// @brief A function pointer to a DaliugeApplication
      /// you can have as many of these as you want as long
      /// as they obey this structure. i.e. they return
      /// a shared_pointer to a DaliugeApplication and they take
      /// an Application Name

      typedef DaliugeApplication::ShPtr DaliugeApplicationCreator(const std::string& name);

      /// @brief Register a function creating a DaliugeApplication object.
      /// @param name The name of the DaliugeApplication.
      /// @param creatorFunc pointer to creator function.
      static void registerDaliugeApplication (const std::string& name,
                                   DaliugeApplicationCreator* creatorFunc);

      /// @brief Try to create a non-standard DaliugeApplication.
      /// Its name is looked up in the creator function registry.
      /// If the gridder name is unknown, a shared library with that name
      /// (in lowercase) is loaded and it executes its register<name>
      /// function which must register its creator function in the registry
      /// using function registerPrimaryBeam.
      /// @param name The name of the gridder.
      /// @param parset ParameterSet containing description of
      /// gridder to be constructed
      static DaliugeApplication::ShPtr createDaliugeApplication (const std::string& name);

      /// @brief Factory class

      static DaliugeApplication::ShPtr make(const std::string &name);

      DaliugeApplicationFactory();

    protected:
      /// @brief helper template method to add pre-defined Applications
      template<typename DaliugeApplicationType> static inline void addPreDefinedDaliugeApplication()  {
          registerDaliugeApplication(DaliugeApplicationType::ApplicationName(), DaliugeApplicationType::createDaliugeApplication);
      }


    private:
      static std::map<std::string, DaliugeApplicationCreator*> theirRegistry;
  }; // class

}// namespace

#endif
