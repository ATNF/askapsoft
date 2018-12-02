/// @file PrimaryBeamFactory.h
///
/// @details
/// PrimaryBeamFactory: Factory class for Primary Beam Responses


#ifndef ASKAP_PRIMARYBEAMFACTORY_H_
#define ASKAP_PRIMARYBEAMFACTORY_H_

// System includes
#include <map>

// ASKAPsoft includes
#include <Common/ParameterSet.h>
#include <boost/shared_ptr.hpp>

// Local package includes
#include "PrimaryBeam.h"


namespace askap
{
  namespace imagemath
  {
    /// @brief Factory class for Primary Beams
    /// @ingroup gridding

    class PrimaryBeamFactory
    {
    public:
      /// @brief A function pointer to a PrimaryBeamCreator
      /// you can have as many of these as you want as long
      /// as they obey this structure. i.e. they return
      /// a shared_pointer to a primary beam and they take
      /// a parset

      typedef PrimaryBeam::ShPtr PrimaryBeamCreator(const LOFAR::ParameterSet&);

      /// @brief Register a function creating a PrimaryBeam object.
      /// @param name The name of the Beam.
      /// @param creatorFunc pointer to creator function.
      static void registerPrimaryBeam (const std::string& name,
                                   PrimaryBeamCreator* creatorFunc);

      /// @brief Try to create a non-standard Beam.
      /// Its name is looked up in the creator function registry.
      /// If the gridder name is unknown, a shared library with that name
      /// (in lowercase) is loaded and it executes its register<name>
      /// function which must register its creator function in the registry
      /// using function registerPrimaryBeam.
      /// @param name The name of the gridder.
      /// @param parset ParameterSet containing description of
      /// gridder to be constructed
      static PrimaryBeam::ShPtr createPrimaryBeam (const std::string& name,
                                               const LOFAR::ParameterSet& parset);


      /// @brief Factory class for all gridders.

      PrimaryBeamFactory();

      /// @brief Make a shared pointer for a Primary Beam
      /// @param parset ParameterSet containing description of
      /// Beam to be constructed.
      /// If needed, the Beam code is loaded dynamically.
      static PrimaryBeam::ShPtr make(const LOFAR::ParameterSet& parset);

    protected:
      /// @brief helper template method to add pre-defined gridders
      template<typename PrimaryBeamType> static inline void addPreDefinedPrimaryBeam()  {
          registerPrimaryBeam(PrimaryBeamType::PrimaryBeamName(), PrimaryBeamType::createPrimaryBeam);
      }


    private:
      static std::map<std::string, PrimaryBeamCreator*> theirRegistry;
    };

  }
}
#endif
