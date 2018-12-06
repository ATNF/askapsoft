/// @file
///
///
///
/// @copyright (c) 2014 CSIRO
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
/// @author Matthew Whiting <Matthew.Whiting@csiro.au>
///
#include <outputs/ResultsWriter.h>
#include <askap_analysis.h>

#include <askap/AskapLogging.h>
#include <askap/AskapError.h>
#include <askapparallel/AskapParallel.h>

#include <parallelanalysis/DuchampParallel.h>
#include <parallelanalysis/DistributedContinuumParameterisation.h>
#include <sourcefitting/RadioSource.h>
#include <sourcefitting/FittingParameters.h>
#include <outputs/AskapComponentParsetWriter.h>
#include <outputs/CataloguePreparation.h>
#include <catalogues/IslandCatalogue.h>
#include <catalogues/ComponentCatalogue.h>
#include <catalogues/HiEmissionCatalogue.h>
#include <catalogues/RMCatalogue.h>
#include <catalogues/FitCatalogue.h>
#include <imageaccess/ImageAccessFactory.h>

#include <duchamp/Cubes/cubes.hh>
#include <duchamp/Outputs/CatalogueSpecification.hh>
#include <duchamp/Outputs/KarmaAnnotationWriter.hh>
#include <duchamp/Outputs/DS9AnnotationWriter.hh>
#include <duchamp/Outputs/CasaAnnotationWriter.hh>
#include <Common/ParameterSet.h>
#include <boost/shared_ptr.hpp>
#include <boost/filesystem.hpp>

///@brief Where the log messages go.
ASKAP_LOGGER(logger, ".resultsWriter");

using namespace duchamp;

namespace askap {

namespace analysis {

ResultsWriter::ResultsWriter(DuchampParallel *finder, askap::askapparallel::AskapParallel &comms):
    itsParset(finder->parset()),
    itsComms(comms),
    itsCube(finder->cube()),
    itsSourceList(finder->rSourceList()),
    itsFitParams(finder->fitParams()),
    itsFlag2D(finder->is2D())
{
}

void ResultsWriter::setFlag2D(bool flag2D)
{
    itsFlag2D = flag2D;
}


void ResultsWriter::duchampOutput()
{

    if (itsComms.isMaster()) {

        if (itsParset.getBool("writeDuchampFiles", true)) {

            // Write standard Duchamp results file
            ASKAPLOG_INFO_STR(logger,
                              "Writing to output catalogue " << itsCube.pars().getOutFile());
            itsCube.outputCatalogue();

            if (itsCube.pars().getFlagLog() && (itsCube.getNumObj() > 0)) {
                // Write the log summary only if required
                itsCube.logSummary();
            }

            // Write all Duchamp annotation files
            itsCube.outputAnnotations();

            if (itsCube.pars().getFlagVOT()) {
                ASKAPLOG_INFO_STR(logger,
                                  "Writing to output VOTable " << itsCube.pars().getVOTFile());
                // Write the standard Duchamp VOTable (not the CASDA islands table!)
                itsCube.outputDetectionsVOTable();
            }

            if (itsCube.pars().getFlagTextSpectra()) {
                ASKAPLOG_INFO_STR(logger, "Saving spectra to text file " <<
                                  itsCube.pars().getSpectraTextFile());
                // Write a text file containing identified spectra
                itsCube.writeSpectralData();
            }

            if (itsCube.pars().getFlagWriteBinaryCatalogue() &&
                    (itsCube.getNumObj() > 0)) {
                ASKAPLOG_INFO_STR(logger,
                                  "Creating binary catalogue of detections, called " <<
                                  itsCube.pars().getBinaryCatalogue());
                // Write the standard Duchamp-format binary catalogue.
                itsCube.writeBinaryCatalogue();
            }

        }
    }
}

void ResultsWriter::writeContinuumCatalogues()
{
    if (itsFlag2D || itsFitParams.doFit()) {
        DistributedContinuumParameterisation distribCont(itsComms, itsParset, itsSourceList);
        distribCont.distribute();
        distribCont.parameterise();
        distribCont.gather();
        std::vector<CasdaIsland> islandList = distribCont.finalIslandList();
        std::vector<CasdaComponent> compList = distribCont.finalComponentList();

        IslandCatalogue islandCat(islandList, itsParset, &itsCube);
        ComponentCatalogue compCat(compList, itsParset, &itsCube);

        if (itsComms.isMaster()) {
            islandCat.write();
            compCat.write();

            writeComponentMaps(distribCont);

        }

    }


}

void ResultsWriter::writeComponentMaps(DistributedContinuumParameterisation &dcp)
{

    casa::Array<float> componentImage = dcp.componentImage();
    
    std::string inputImageName = itsParset.getString("image", "");
    const boost::filesystem::path infile(inputImageName);
    ASKAPCHECK(inputImageName != "", "No image name provided in parset with parameter 'image'");

    DuchampParallel dp(itsComms,itsParset);
    ASKAPCHECK(dp.getCASA(IMAGE) == duchamp::SUCCESS, "Reading data from input image failed");
    
    casa::Slicer theSlicer = analysisutilities::subsectionToSlicer(dp.cube().pars().section());
    
    casa::Array<float> inputImage(componentImage.shape(),dp.cube().getArray(),SHARE);
    casa::Vector<bool> maskVec(dp.cube().makeBlankMask());
    casa::Array<bool> mask(componentImage.shape(),maskVec.data(),SHARE);

    ASKAPLOG_INFO_STR(logger, "mask shapes: maskVec->"<<maskVec.shape()<<", mask->"<<mask.shape());
    
    int nstokes=1;
    casa::CoordinateSystem coords = analysisutilities::wcsToCASAcoord(dp.cube().header().getWCS(), nstokes);
    
    LOFAR::ParameterSet fitParset = itsParset.makeSubset("Fitter.");
    if (!fitParset.isDefined("imagetype")){
        fitParset.add("imagetype","fits");
    }

    boost::shared_ptr<accessors::IImageAccess> imageAcc = accessors::imageAccessFactory(fitParset);
    boost::shared_ptr<accessors::IImageAccess> inputImageAcc = accessors::imageAccessFactory(fitParset);
    std::string image_no_ext=inputImageName;
    if(image_no_ext.find(".fits") != std::string::npos){
        if (image_no_ext.substr(image_no_ext.rfind("."), std::string::npos) == ".fits") {
                image_no_ext.erase(image_no_ext.rfind("."), std::string::npos);
            }
        }
    casa::Vector<casa::Quantum<double> > beam = inputImageAcc->beamInfo(image_no_ext);

    bool doComponentMap = fitParset.getBool("writeComponentMap", true);
    if (doComponentMap) {
        std::string componentMap = "componentMap_" + infile.filename().string();
        // Need to remove any ".fits" extension, as this will be added by the accessor
        if (componentMap.find(".fits") != std::string::npos) {
            if (componentMap.substr(componentMap.rfind("."), std::string::npos) == ".fits") {
                componentMap.erase(componentMap.rfind("."), std::string::npos);
            }
        }
        casa::IPosition blc(componentImage.ndim(),0);
        imageAcc->create(componentMap, componentImage.shape(), coords);
        imageAcc->write(componentMap, componentImage);
        imageAcc->makeDefaultMask(componentMap);
        imageAcc->writeMask(componentMap, mask, casa::IPosition(componentImage.shape().nelements(),0));
        imageAcc->setBeamInfo(componentMap, beam[0].getValue("rad"), beam[1].getValue("rad"), beam[2].getValue("rad"));
        imageAcc->addHistory(componentMap, "Map of fitted components, made by Selavy");
        imageAcc->addHistory(componentMap, "Original image: " + inputImageName);
        if (itsParset.isDefined("imageHistory")) {
            std::vector<std::string> historyMessages = itsParset.getStringVector("imageHistory", "");
            if (historyMessages.size() > 0) {
                for (std::vector<std::string>::iterator history = historyMessages.begin();
                     history < historyMessages.end(); history++) {
                    imageAcc->addHistory(componentMap, *history);
                }
            }
        }
        
        
        std::string componentResidualMap = "componentResidual_" + infile.filename().string();
        // Need to remove any ".fits" extension, as this will be added by the accessor
        if (componentResidualMap.find(".fits") != std::string::npos) {
            if (componentResidualMap.substr(componentResidualMap.rfind("."), std::string::npos) == ".fits") {
                componentResidualMap.erase(componentResidualMap.rfind("."), std::string::npos);
            }
        }
        casa::Array<float> residual = inputImage - componentImage;
        imageAcc->create(componentResidualMap, residual.shape(), coords);
        imageAcc->write(componentResidualMap, residual);
        imageAcc->makeDefaultMask(componentResidualMap);
        imageAcc->writeMask(componentResidualMap, mask, casa::IPosition(componentImage.shape().nelements(),0));
        imageAcc->setBeamInfo(componentResidualMap, beam[0].getValue("rad"), beam[1].getValue("rad"), beam[2].getValue("rad"));
        imageAcc->addHistory(componentResidualMap, "Residual after subtracting fitted components, made by Selavy");
        imageAcc->addHistory(componentResidualMap, "Original image: " + inputImageName);
        if (itsParset.isDefined("imageHistory")) {
            std::vector<std::string> historyMessages = itsParset.getStringVector("imageHistory", "");
            if (historyMessages.size() > 0) {
                for (std::vector<std::string>::iterator history = historyMessages.begin();
                     history < historyMessages.end(); history++) {
                    imageAcc->addHistory(componentResidualMap, *history);
                }
            }
        }

    }


}

void ResultsWriter::writeIslandCatalogue()
{
    if (itsComms.isMaster()) {
        if (itsFlag2D) {

            IslandCatalogue cat(itsSourceList, itsParset, &itsCube);
            cat.write();

        }
    }
}

void ResultsWriter::writeComponentCatalogue()
{
    if (itsComms.isMaster()) {
        if (itsFitParams.doFit()) {

            ComponentCatalogue cat(itsSourceList, itsParset, &itsCube);
            cat.write();

        }
    }
}

void ResultsWriter::writeHiEmissionCatalogue()
{
    if (itsParset.getBool("HiEmissionCatalogue", false)) {

        HiEmissionCatalogue cat(itsSourceList, itsParset, &itsCube, itsComms);
        if (itsComms.isMaster()) {
            cat.write();
        }
    }
}


void ResultsWriter::writePolarisationCatalogue()
{

    if (itsParset.getBool("RMSynthesis", false)) {

        RMCatalogue cat(itsSourceList, itsParset, &itsCube, itsComms);
        if (itsComms.isMaster()) {
            cat.write();
        }
    }

}


void ResultsWriter::writeFitResults()
{
    if (itsComms.isMaster()) {
        if (itsFitParams.doFit()) {

            if (itsParset.getBool("writeFitResults", false)) {

                std::vector<std::string> outtypes = itsFitParams.fitTypes();
                outtypes.push_back("best");

                for (size_t t = 0; t < outtypes.size(); t++) {

                    FitCatalogue cat(itsSourceList, itsParset, &itsCube, outtypes[t]);
                    cat.write();

                }

            }

        }
    }
}

void ResultsWriter::writeFitAnnotations()
{
    if (itsComms.isMaster()) {
        if (itsFitParams.doFit()) {

            std::string fitBoxAnnotationFile = itsParset.getString("fitBoxAnnotationFile",
                                               "selavy-fitResults.boxes.ann");
            if (!itsFitParams.fitJustDetection()) {

                if (itsSourceList.size() > 0) {

                    for (int i = 0; i < 3; i++) {
                        boost::shared_ptr<duchamp::AnnotationWriter> writerFit;
                        boost::shared_ptr<duchamp::AnnotationWriter> writerBox;
                        std::string filename;
                        size_t loc;
                        switch (i) {
                            case 0: //Karma
                                writerBox = boost::shared_ptr<KarmaAnnotationWriter>(
                                                new KarmaAnnotationWriter(fitBoxAnnotationFile));
                                break;
                            case 1://DS9
                                filename = fitBoxAnnotationFile;
                                loc = filename.rfind(".ann");
                                if (loc == std::string::npos) filename += ".reg";
                                else filename.replace(loc, 4, ".reg");
                                writerBox = boost::shared_ptr<DS9AnnotationWriter>(
                                                new DS9AnnotationWriter(filename));
                                break;
                            case 2://CASA
                                filename = fitBoxAnnotationFile;
                                loc = filename.rfind(".ann");
                                if (loc == std::string::npos) filename += ".reg";
                                else filename.replace(loc, 4, ".reg");
                                writerBox =
                                    boost::shared_ptr<CasaAnnotationWriter>(
                                        new CasaAnnotationWriter(filename));
                                break;
                        }

                        if (writerBox.get() != 0) {
                            writerBox->setup(&itsCube);
                            writerBox->openCatalogue();
                            writerBox->setColourString("BLUE");
                            writerBox->writeHeader();
                            writerBox->writeParameters();
                            writerBox->writeStats();
                            writerBox->writeTableHeader();

                            std::vector<sourcefitting::RadioSource>::iterator src;
                            int num = 1;
                            for (src = itsSourceList.begin(); src < itsSourceList.end(); src++) {
                                src->writeFitToAnnotationFile(writerBox, num++, false, true);
                            }

                            writerBox->writeFooter();
                            writerBox->closeCatalogue();
                        }

                        writerBox.reset();

                    }

                }

            }

        }
    }
}

void ResultsWriter::writeComponentParset()
{
    if (itsComms.isMaster()) {
        if (itsFitParams.doFit()) {
            if (itsParset.getBool("outputComponentParset", false)) {
                /// @todo Instantiate the writer from a parset - then don't have to find the flags etc
                LOFAR::ParameterSet subset = itsParset.makeSubset("outputComponentParset.");
                ASKAPLOG_INFO_STR(logger, "Writing Fit results to parset named "
                                  << subset.getString("filename"));
                AskapComponentParsetWriter pwriter(subset, &itsCube);
                pwriter.setFitType("best");
                pwriter.setSourceList(&itsSourceList);
                pwriter.openCatalogue();
                pwriter.writeTableHeader();
                pwriter.writeEntries();
                pwriter.writeFooter();
                pwriter.closeCatalogue();
            }
        }
    }
}


}

}
