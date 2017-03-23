/// @file FITSImageRW.cc
/// @brief Read/Write FITS image class
/// @details This class implements the write methods that are absent
/// from the casacore FITSImage.
///
///
/// @copyright (c) 2016 CSIRO
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
/// @author Stephen Ord <stephen.ord@csiro.au
///
#include <askap_accessors.h>
#include <askap/AskapLogging.h>

#include <casacore/images/Images/FITSImage.h>
#include <casacore/casa/BasicSL/String.h>
#include <casacore/casa/Utilities/DataType.h>
#include <casacore/fits/FITS/fitsio.h>
#include <casacore/fits/FITS/FITSDateUtil.h>
#include <casacore/fits/FITS/FITSHistoryUtil.h>
#include <casacore/fits/FITS/FITSReader.h>

#include <casacore/casa/Quanta/MVTime.h>
#include <imageaccess/FITSImageRW.h>

#include <fitsio.h>
#include <iostream>
#include <fstream>

ASKAP_LOGGER(FITSlogger, ".FITSImageRW");

void printerror( int status)
{
    /*****************************************************/
    /* Print out cfitsio error messages and exit program */
    /*****************************************************/


    if (status)
    {
       ASKAPLOG_ERROR_STR(FITSlogger, status); /* print error report */

       exit( status );    /* terminate the program, returning error status */
    }
    return;
}

using namespace askap;
using namespace askap::accessors;

FITSImageRW::FITSImageRW(const std::string &name) {
    std::string fullname = name + ".fits";
    this->name = std::string(name.c_str());
}
FITSImageRW::FITSImageRW() {

}
bool FITSImageRW::create(const std::string &name, const casa::IPosition &shape,\
    const casa::CoordinateSystem &csys,\
    uint memoryInMB, bool preferVelocity,\
	bool opticalVelocity, int BITPIX, float minPix, float maxPix,\
	bool degenerateLast, bool verbose, bool stokesLast,\
	bool preferWavelength, bool airWavelength, bool primHead,\
	bool allowAppend, bool history) {

    std::string fullname = name + ".fits";

    this->name = std::string(fullname.c_str());
    this->shape = shape;
    this->csys = csys;
    this->memoryInMB = memoryInMB;
    this->preferVelocity = preferVelocity;
    this->opticalVelocity = opticalVelocity;
    this->BITPIX=BITPIX;
    this->minPix = minPix;
    this->maxPix = maxPix;
    this->degenerateLast = degenerateLast;
    this->verbose = verbose ;
    this->stokesLast = stokesLast;
    this->preferWavelength = preferWavelength;
    this->airWavelength = airWavelength;
    this->primHead = primHead;
    this->allowAppend = allowAppend;
    this->history = history;

    ASKAPLOG_INFO_STR(FITSlogger,"Creating R/W FITSImage " << this->name);

    unlink(this->name.c_str());
    std::ofstream outfile (this->name.c_str());
    ASKAPCHECK(outfile.is_open(),"Cannot open FITS file for output");




    casa::String error;
    const casa::uInt ndim = shape.nelements();
    // //
    // // Find scale factors
    // //
    casa::Record header;
    casa::Double bscale, bzero;

    if (BITPIX == -32) {

        bscale = 1.0;
        bzero = 0.0;
        header.define("bitpix", BITPIX);
        header.setComment("bitpix", "Floating point (32 bit)");

    }

    else {
        error =
        "BITPIX must be -32 (floating point)";
        return false;
    }
    //
    // At this point, for 32 floating point, we must apply the given
    // mask.  For 16bit, we may know that there are in fact no blanks
    // in the image, so we can dispense with looking at the mask again.


    //
    casa::Vector<casa::Int> naxis(ndim);
    casa::uInt i;
    for (i=0; i < ndim; i++) {
        naxis(i) = shape(i);
    }
    header.define("naxis", naxis);
    if (allowAppend)
        header.define("extend", casa::True);
    if (!primHead){
        header.define("PCOUNT", 0);
        header.define("GCOUNT", 1);
    }
    header.define("bscale", bscale);
    header.setComment("bscale", "PHYSICAL = PIXEL*BSCALE + BZERO");
    header.define("bzero", bzero);


    header.define("COMMENT1", ""); // inserts spaces
    // I should FITS-ize the units

    header.define("BUNIT", "Jy");
    header.setComment("BUNIT", "Brightness (pixel) unit");
    //
    casa::IPosition shapeCopy = shape;
    casa::CoordinateSystem cSys = csys;

    casa::Record saveHeader(header);
    casa::Bool ok = cSys.toFITSHeader(header, shapeCopy, casa::True, 'c', casa::True, // use WCS
    preferVelocity, opticalVelocity,
    preferWavelength, airWavelength);
    if (!ok) {
        ASKAPLOG_WARN_STR(FITSlogger, "Could not make a standard FITS header. Setting" \
        <<	" a simple linear coordinate system.") ;

        casa::uInt n = cSys.nWorldAxes();
        casa::Matrix<casa::Double> pc(n,n); pc=0.0; pc.diagonal() = 1.0;
        casa::LinearCoordinate linear(cSys.worldAxisNames(),
        cSys.worldAxisUnits(),
        cSys.referenceValue(),
        cSys.increment(),
        cSys.linearTransform(),
        cSys.referencePixel());
        casa::CoordinateSystem linCS;
        linCS.addCoordinate(linear);

        // Recover old header before it got mangled by toFITSHeader

        header = saveHeader;
        casa::IPosition shapeCopy = shape;
        casa::Bool ok = linCS.toFITSHeader(header, shapeCopy, casa::True, 'c', casa::False); // don't use WCS
        if (!ok) {
            ASKAPLOG_WARN_STR(FITSlogger,"Fallback linear coordinate system fails also.");
            return false;
        }
    }
    // When this if test is True, it means some pixel axes had been removed from
    // the coordinate system and degenerate axes were added.

    if (naxis.nelements() != shapeCopy.nelements()) {
        naxis.resize(shapeCopy.nelements());
        for (casa::uInt j=0; j < shapeCopy.nelements(); j++) {
            naxis(j) = shapeCopy(j);
        }
        header.define("NAXIS", naxis);
    }

    //
    // DATE
    //

    casa::String date, timesys;
    casa::Time nowtime;
    casa::MVTime now(nowtime);
    casa::FITSDateUtil::toFITS(date, timesys, now);
    header.define("date", date);
    header.setComment("date", "Date FITS file was written");
    if (!header.isDefined("timesys") && !header.isDefined("TIMESYS")) {
        header.define("timesys", timesys);
        header.setComment("timesys", "Time system for HDU");
    }
    // //
    // // ORIGIN
    // //

    header.define("ORIGIN", "ASKAPSoft");

    theKeywordList = casa::FITSKeywordUtil::makeKeywordList(primHead, casa::True);

    //kw.mk(FITS::EXTEND, True, "Tables may follow");
    // add the general keywords for WCS and so on
    ok = casa::FITSKeywordUtil::addKeywords(theKeywordList, header);
    if (! ok) {
        error = "Error creating initial FITS header";
        return false;
    }


    //
    // END
    //

    theKeywordList.end();

    // now get them into a file ...

    theKeywordList.first();
    theKeywordList.next(); // skipping an extra SIMPLE... hack
    casa::FitsKeyCardTranslator m_kc;

    char cards[2880];

    while (m_kc.build(cards,theKeywordList)) {
        outfile << cards;
    }
    outfile << cards;

    outfile.close();

    return true;

}
void FITSImageRW::print_hdr() {
    fitsfile *fptr;       /* pointer to the FITS file, defined in fitsio.h */

    int status, nkeys, keypos, hdutype, ii, jj;
    char card[FLEN_CARD];   /* standard string lengths defined in fitsioc.h */

    status = 0;

    if ( fits_open_file(&fptr, this->name.c_str(), READONLY, &status) )
         printerror( status );

    /* attempt to move to next HDU, until we get an EOF error */
    for (ii = 1; !(fits_movabs_hdu(fptr, ii, &hdutype, &status) ); ii++)
    {
        /* get no. of keywords */
        if (fits_get_hdrpos(fptr, &nkeys, &keypos, &status) )
            printerror( status );

        printf("Header listing for HDU #%d:\n", ii);
        for (jj = 1; jj <= nkeys; jj++)  {
            if ( fits_read_record(fptr, jj, card, &status) )
                 printerror( status );

            printf("%s\n", card); /* print the keyword card */
        }
        printf("END\n\n");  /* terminate listing with END */
    }

    if (status == END_OF_FILE)   /* status values are defined in fitsioc.h */
        status = 0;              /* got the expected EOF error; reset = 0  */
    else
       printerror( status );     /* got an unexpected error                */

    if ( fits_close_file(fptr, &status) )
         printerror( status );

    return;

}
bool FITSImageRW::write(const casa::Array<float> &arr) {
    ASKAPLOG_INFO_STR(FITSlogger,"Writing array to FITS image");
    fitsfile *fptr;       /* pointer to the FITS file, defined in fitsio.h */


    int status;


    status = 0;

    if ( fits_open_file(&fptr, this->name.c_str(), READWRITE, &status) )
         printerror( status );

    long fpixel = 1;                               /* first pixel to write      */
    size_t nelements = arr.nelements();          /* number of pixels to write */
    bool deleteIt;
    const float *data = arr.getStorage(deleteIt);
    void *dataptr = (void *) data;

         /* write the array of unsigned integers to the FITS file */
    if ( fits_write_img(fptr, TFLOAT, fpixel, nelements, dataptr, &status) )
        printerror( status );

    if ( fits_close_file(fptr, &status) )
             printerror( status );

    return true;
}
bool FITSImageRW::write(const casa::Array<float> &arr,const casa::IPosition &where) {
    ASKAPLOG_INFO_STR(FITSlogger,"Writing array to FITS image at (Cindex)" << where);
    fitsfile *fptr;       /* pointer to the FITS file, defined in fitsio.h */

    int status, hdutype;


    status = 0;

    if ( fits_open_file(&fptr, this->name.c_str(), READWRITE, &status) )
        printerror( status );

    if ( fits_movabs_hdu(fptr, 1, &hdutype, &status) )
        printerror( status );

    // we do not currently support postage stamps so the write has to be of
    // the correct dimension


    // we have to place the slice at the start of a channel

    if (where[0] != 0 || where[1] != 0) {
        ASKAPLOG_ERROR_STR(FITSlogger,"slice position not at channel boundary");
    }


    long fpixel[4];


    ASKAPLOG_INFO_STR(FITSlogger,"There are " << where.nelements() << " dimensions in the slice");
    fpixel[0] = 1;
    ASKAPLOG_INFO_STR(FITSlogger,"fpixel[0] = " << fpixel[0]);
    fpixel[1] = 1;
    ASKAPLOG_INFO_STR(FITSlogger,"fpixel[1] = " << fpixel[1]);
    if (where.nelements() == 3) {
        fpixel[2] = where[2] + 1;
        ASKAPLOG_INFO_STR(FITSlogger,"fpixel[2] = " << fpixel[2]);
    }
    else {
        fpixel[2] = 1;
        ASKAPLOG_INFO_STR(FITSlogger,"fpixel[2] = " << fpixel[2]);
        fpixel[3] = where[3] + 1;
        ASKAPLOG_INFO_STR(FITSlogger,"fpixel[3] = " << fpixel[2]);
    }

    int64_t nelements = arr.nelements();          /* number of pixels to write */

    ASKAPLOG_INFO_STR(FITSlogger,"We are writing " << nelements << " elements");
    bool deleteIt = false;
    const float *data = arr.getStorage(deleteIt);
    void *dataptr = (void *) data;

    status = 0;

    if ( fits_write_pix(fptr, TFLOAT,fpixel, nelements, dataptr, &status) )
        printerror( status );

    status = 0;

    if ( fits_close_file(fptr, &status) )
        printerror( status );



    return true;

}
void FITSImageRW::setUnits(const std::string &units) {
    ASKAPLOG_INFO_STR(FITSlogger,"Updating brightness units");
    fitsfile *fptr;       /* pointer to the FITS file, defined in fitsio.h */
    int status = 0;

    if ( fits_open_file(&fptr, this->name.c_str(), READWRITE, &status) )
        printerror( status );

    if ( fits_update_key(fptr, TSTRING, "BUNIT", (void *)(units.c_str()),
         "Brightness (pixel) unit", &status) )
        printerror( status );

    if ( fits_close_file(fptr, &status) )
        printerror( status );

}

void FITSImageRW::setRestoringBeam(double maj, double min, double pa) {
    ASKAPLOG_INFO_STR(FITSlogger,"Setting Beam info");
    ASKAPLOG_INFO_STR(FITSlogger,"Updating brightness units");
    fitsfile *fptr;       /* pointer to the FITS file, defined in fitsio.h */
    int status = 0;
    double radtodeg = 360./(2*M_PI);
    if ( fits_open_file(&fptr, this->name.c_str(), READWRITE, &status) )
        printerror( status );

    double value = radtodeg*maj;
    if ( fits_update_key(fptr, TDOUBLE, "BMAJ", &value,
        "Restoring beam major axis", &status) )
        printerror( status );
    value = radtodeg*min;
    if ( fits_update_key(fptr, TDOUBLE, "BMIN", &value,
        "Restoring beam minor axis", &status) )
        printerror( status );
    value = radtodeg*pa;
    if ( fits_update_key(fptr, TDOUBLE, "BPA", &value,
            "Restoring beam position angle", &status) )
            printerror( status );
    if ( fits_update_key(fptr, TSTRING, "BTYPE", (void *) "Intensity",
            " ", &status) )
            printerror( status );

    if ( fits_close_file(fptr, &status) )
        printerror( status );

}
FITSImageRW::~FITSImageRW()
{
}
