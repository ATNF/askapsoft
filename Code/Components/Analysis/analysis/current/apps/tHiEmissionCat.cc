#include <askap_analysis.h>

#include <askap/AskapLogging.h>
#include <askap/AskapError.h>
#include <askapparallel/AskapParallel.h>

#include <catalogues/CasdaComponent.h>
#include <catalogues/CasdaHiEmissionObject.h>
#include <catalogues/HiEmissionCatalogue.h>
#include <sourcefitting/RadioSource.h>
#include <parallelanalysis/DuchampParallel.h>

#include <Common/ParameterSet.h>
#include <images/Images/ImageOpener.h>
#include <images/Images/FITSImage.h>
#include <duchamp/Cubes/cubes.hh>
#include <vector>

using namespace askap;
using namespace askap::analysis;

ASKAP_LOGGER(logger, "tHiEmissionCat.log");

int main(int argc, const char *argv[])
{

    // This class must have scope outside the main try/catch block
    askap::askapparallel::AskapParallel comms(argc, argv);
    try {

        ImageOpener::registerOpenImageFunction(ImageOpener::FITS, FITSImage::openFITSImage);

        LOFAR::ParameterSet parset;
        parset.add("sbid", "10001");
        std::string imdir(getenv("ASKAP_ROOT"));
        imdir += "/3rdParty/Duchamp/Duchamp-1.6.1/Duchamp-1.6.1/verification/";
        parset.add("image",imdir+"verificationCube.fits");
        parset.add("snrCut","5");
        parset.add("sortingParam","-pflux");
        parset.add("minChannels", "1");
        parset.add("spectralUnits", "km/s");

        DuchampParallel finder(comms, parset);
        finder.readData();
        finder.preprocess();
        finder.gatherStats();
        finder.setThreshold();
        finder.findSources();
        finder.fitSources();
        finder.sendObjects();
        finder.receiveObjects();
        finder.cleanup();
        finder.printResults();

        std::vector<sourcefitting::RadioSource> objlist = finder.sourceList();
        duchamp::Cube *cube = finder.pCube();
        
        HiEmissionCatalogue cat(objlist, parset, *cube);
        cat.write();


    } catch (askap::AskapError& x) {
        ASKAPLOG_FATAL_STR(logger, "Askap error in " << argv[0] << ": " << x.what());
        std::cerr << "Askap error in " << argv[0] << ": " << x.what() << std::endl;
        exit(1);
        // } catch (const duchamp::DuchampError& x) {
        //     ASKAPLOG_FATAL_STR(logger, "Duchamp error in " << argv[0] << ": " << x.what());
        //     std::cerr << "Duchamp error in " << argv[0] << ": " << x.what() << std::endl;
        //     exit(1);
    } catch (std::exception& x) {
        ASKAPLOG_FATAL_STR(logger, "Unexpected exception in " << argv[0] << ": " << x.what());
        std::cerr << "Unexpected exception in " << argv[0] << ": " << x.what() << std::endl;
        exit(1);
    }

    exit(0);

}
