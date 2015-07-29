#include <askap_analysis.h>

#include <askap/AskapLogging.h>
#include <askap/AskapError.h>
#include <askapparallel/AskapParallel.h>

#include <catalogues/CasdaComponent.h>
#include <catalogues/CasdaAbsorptionObject.h>
#include <catalogues/AbsorptionCatalogue.h>
#include <sourcefitting/RadioSource.h>
#include <parallelanalysis/DuchampParallel.h>

#include <Common/ParameterSet.h>
#include <images/Images/ImageOpener.h>
#include <images/Images/FITSImage.h>
#include <duchamp/Cubes/cubes.hh>
#include <vector>

using namespace askap;
using namespace askap::analysis;

ASKAP_LOGGER(logger, "tAbsorptionCat.log");

int main(int argc, const char *argv[])
{

    // This class must have scope outside the main try/catch block
    askap::askapparallel::AskapParallel comms(argc, argv);
    try {

        ImageOpener::registerOpenImageFunction(ImageOpener::FITS, FITSImage::openFITSImage);

        LOFAR::ParameterSet parset;
        parset.add("sbid", "10001");
        parset.add("image","absCatTest.fits");
        parset.add("flagSubsection","true");
        parset.add("subsection","[*,*,1:1,*]");
        parset.add("snrCut","10");
        parset.add("sortingParam","-pflux");
        parset.add("doFit","true");

        DuchampParallel finder(comms, parset);
        finder.readData();
        finder.preprocess();
        finder.gatherStats();
        finder.setThreshold();
        finder.findSources();
        finder.fitSources();

        sourcefitting::RadioSource obj=finder.getSource(0);
        CasdaComponent comp(obj,parset,0);
        std::vector< std::pair<CasdaComponent,sourcefitting::RadioSource> > objlist;
        objlist.push_back(std::pair<CasdaComponent,sourcefitting::RadioSource>(comp,obj));
        duchamp::Cube *cube = finder.pCube();
        
        AbsorptionCatalogue cat(objlist, parset, *cube);
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
