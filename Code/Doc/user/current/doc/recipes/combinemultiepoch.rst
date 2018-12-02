Multi-epoch data combination for spectral cubes
===============================================
(After the original guide by Dane Kleiner)

There are several ways to combine multi-epoch data with ASKAPsoft:

* Do all the calibration and imaging separately and then combine the images using linear mosaicking (program linmos-mpi)
* Combine data for the same beams and footprints in the UV-plane during imaging, gaining the advantage of better signal to noise during the deconvolution, then combine the images as before using linmos
* Combine all the data during imaging, doing a full non-linear mosaic using A-projection. This option would be preferred to best recover extended emission, but it requires knowledge of the primary beams to an accuracy we don't have yet. It also tends to requires more memory (and possibly time) than is available on galaxy and thus will not be described here at the moment.

General info:
To submit any of the example slurm files, copy the text into a <file>.sbatch, modify the necessary parts and submit using *sbatch file.sbatch*.
In any of the slurm files:

1. If you want to produce the slurm output, log and parset then you need to point each of them to a valid directory. E.g., I create a directory for each in the work directory that I run the task.
2. Submit to galaxy/magnus in the #SBATCH preamble clusters
3. Insert your email address into #SBATCH ­­mail
4. Set your work directory using #SBATCH workdir
5. Change the #SBATCH ­­output, parset and log directories
6. Change the #SBATCH ­­job­name according to the beam you are processing.


1) Combining in the Image plane
-------------------------------
This is the most straightforward method. You just feed the linmos program all the individual restored images and corresponding weights images.
Beware that this path limits your clean deconvolution to the sensitivity of a single night.
Here is an example slurm file that will use linmos to mosaic images together::

    #!/bin/bash -l
    #SBATCH --partition=workq
    #SBATCH --clusters=galaxy
    # Using the default account
    # No reservation requested
    #SBATCH --time=12:00:00
    #SBATCH --ntasks=200
    #SBATCH --ntasks-per-node=20
    #SBATCH --job-name=linmosFullScontsub
    #SBATCH --mail-user=first.last@csiro.au
    #SBATCH --mail-type=FAIL
    #SBATCH --export=NONE
    #SBATCH --workdir=/group/askap/<username>/<object>/COMBINED_IMAGE_PLANE
    #SBATCH --output=slurmOutput/slurm-linmosS-%j.out
    # Using user-defined askapsoft module
    module use /group/askap/modulefiles
    module load askapdata
    # ### Decide if we need a specific version here
    module load askapsoft
    module load askappipeline
    IMAGETYPE_SPECTRAL="casa"
    imList="$(cat imList.txt)"
    wtList="$(cat wtList.txt)"
    imageName=image.restored.wr.1.i.<object>.cube.combined16_img.contsub.linmos
    weightsImage=weights.wr.1.i.<object>.cube.combined16_img.contsub.linmos
    echo "Mosaicking to form ${imageName}"
    parset=parsets/science_linmos_${SLURM_JOB_ID}.in
    log=logs/science_linmos_${SLURM_JOB_ID}.log
    cat > "${parset}" << EOFINNER
    linmos.names        = [${imList}]
    linmos.weights      = [${wtList}]
    linmos.imagetype    = ${IMAGETYPE_SPECTRAL}
    linmos.outname      = ${imageName%%.fits}
    linmos.outweight    = ${weightsImage%%.fits}
    linmos.weightstate  = Inherent
    linmos.weighttype   = Combined
    linmos.cutoff       = 0.2
    EOFINNER

    NCORES=200
    NPPN=20
    srun --export=ALL --ntasks=${NCORES} --ntasks-per-node=${NPPN} linmos-mpi -c "$parset" > "$log"

Things to change / be aware of:

* imList is the list of images you give to linmos. In this example, I'm pointing it to a text file (imList.txt) that lists the images separated by a comma e.g. restored.contsub.image1, restored.contsub.image2 . Alternatively, you can give the list of images to the imList variable in the slurm file.
* wtList is the list of weights images produced by imager specified in exactly the same fashion as imList.
* imageName and weightsImage are the names of the output image and weight map from linmos.
* Provided no primary beam correction has been applied weightstate=Inherent and weighttype=Combined


2) Hybrid combination: UV + image plane
---------------------------------------
This is broken into 3 parts:

1. Imaging
2. Image based continuum subtraction
3. Mosaicking

2.1) Imaging
------------

Important info:

1. This method uses the imager program. You feed imager calibrated measurement sets (i.e., the sciencedata.ms not averaged.ms or averaged_cal.ms) with the same phase centre (pointing). Essentially, this can only be done on individual beams of the same footprint and orientation.
2. Each calibrated measurement set must have the same number of channels and the same starting frequency
3. This job can and should be parallelised. To do so, you need to set: #SBATCH ­­ntasks and Cimager.nchanpercore. To do this: Start with the number of channels in your measurement set (649 in this case), nchanpercore = a number (11 in this case) that evenly divides into your number of channels and ntasks-1 = (# of chans) / nchanpercore (60 in this case). NCORES (found at the bottom of the slurm file) is the same as ntasks and they should be set to the same value. To clarify ­ ntask­per­node is the number of processes that run on a single node. It has to be <=ntasks where the maximum is 20 for galaxy and 24 for magnus. The choice of this number is going to be influenced by the memory usage of your job ­ if you increase the size of images, for instance, you may not be able to fit 20 jobs on a node given the total node memory budget of 64GB)
4. The example below allows to combine old and new data with slightly different frequency labelling (up to 10 Hz in this case). The line Cimager.channeltolerance=10.0 takes care of that.
5. Below is an example slurm file that will use imager to image multiple measurement sets.::

    #!/bin/bash -l
    #SBATCH --partition=workq
    #SBATCH --clusters=galaxy
    # Using the default account
    # No reservation requested
    #SBATCH --time=12:00:00
    #SBATCH --ntasks=60
    #SBATCH --ntasks-per-node=10
    #SBATCH --job-name=spec_F00B09
    #SBATCH --mail-user=first.last@csiro.au
    #SBATCH --mail-type=FAIL
    #SBATCH --export=NONE
    #SBATCH --workdir=/group/askap/<user>/<object>/COMBINED_ALL
    #SBATCH --output=slurmOutput/slurm-science_spectral_imager-B09-%j.out
    # Using user-defined askapsoft module
    module use /group/askap/modulefiles
    module load askapdata
    module unload askapsoft
    module load askapsoft/0.19.6
    module unload askappipeline
    module load askappipeline/0.19.6
    beam_no="09"
    parset=parsets/science_spectral_imager_F00_B${beam_no}_${SLURM_JOB_ID}.in
    # Footprint A
    # Beam 09
    direction="[22h20m51.830, -45.42.58.82, J2000]"
    # Footprint A
    msin="11Aug_${beam_no}_A_1-649.ms, 12Aug_${beam_no}_A_1-649.ms, 11Oct_${beam_no}_A_1-649.ms"
    # Output name
    imOut="image.i.<object>.cube.combinedA.b${beam_no}"
    cat > "$parset" << EOF
    Cimager.dataset         = [$msin]
    Cimager.imagetype       = casa
    Cimager.channeltolerance=10.0
    #
    Cimager.Images.Names    = [$imOut]
    Cimager.Images.shape    = [1024, 1024]
    Cimager.Images.cellsize = [6arcsec, 6arcsec]
    Cimager.Images.direction= ${direction}
    Cimager.Images.restFrequency = HI
    # Options for the alternate imager
    Cimager.nchanpercore    = 11
    Cimager.usetmpfs        = false

    Cimager.tmpfs           = /dev/shm
    # barycentre and multiple solver mode not supported in continuum imaging (yet)
    Cimager.barycentre      = true
    Cimager.solverpercore   = true
    Cimager.nwriters        = 1
    Cimager.singleoutputfile= false
    #
    # This defines the parameters for the gridding.
    Cimager.gridder.snapshotimaging             = true
    Cimager.gridder.snapshotimaging.wtolerance  = 2600
    Cimager.gridder.snapshotimaging.longtrack   = true
    Cimager.gridder.snapshotimaging.clipping    = 0.01
    Cimager.gridder                             = WProject
    Cimager.gridder.WProject.wmax               = 2600
    Cimager.gridder.WProject.nwplanes           = 99
    Cimager.gridder.WProject.oversample         = 4
    Cimager.gridder.WProject.maxsupport         = 512
    Cimager.gridder.WProject.variablesupport    = true
    Cimager.gridder.WProject.offsetsupport      = true
    #
    # These parameters define the clean algorithm
    Cimager.solver                              = Clean
    Cimager.solver.Clean.algorithm              = BasisfunctionMFS
    Cimager.solver.Clean.niter                  = 5000
    Cimager.solver.Clean.gain                   = 0.1
    Cimager.solver.Clean.scales                 = [0,3,10]
    Cimager.solver.Clean.verbose                = False
    Cimager.solver.Clean.tolerance              = 0.01
    Cimager.solver.Clean.weightcutoff           = zero
    Cimager.solver.Clean.weightcutoff.clean     = false
    Cimager.solver.Clean.psfwidth               = 512
    Cimager.solver.Clean.logevery               = 50
    Cimager.threshold.minorcycle                = [40%, 9mJy]
    Cimager.threshold.majorcycle                = 10mJy
    Cimager.ncycles                             = 5
    Cimager.Images.writeAtMajorCycle            = false
    #
    Cimager.preconditioner.Names                = [Wiener]
    #Cimager.preconditioner.GaussianTaper       = 30arcsec
    Cimager.preconditioner.preservecf           = true
    Cimager.preconditioner.Wiener.robustness    = 0.5
    #
    # These parameter govern the restoring of the image and the recording of the beam
    Cimager.restore                             = true
    Cimager.restore.beam                        = fit
    Cimager.restore.beam.cutoff                 = 0.5
    Cimager.restore.beamReference               = mid
    EOF

    log=logs/science_spectral_imager_F00_B${beam_no}_${SLURM_JOB_ID}.log

    # Now run the imager
    NCORES=60
    NPPN=10
    srun --export=ALL --ntasks=${NCORES} --ntasks-per-node=${NPPN} imager -c "$parset" > "$log"

Things to change / be aware of:

* Change the beam_no, object name etc.
* Change the workdir accordingly and make sure­ the calibrated measurement sets are all there.
* The direction is essential. It can be found in the science_imager parset from when you ran the pipeline on the same beam and orientation. You will need to look this up and replace it with the relevant co­ordinates.
* Change msin to point to all the measurement sets you would like to image. I.e., If you want to put 3 images together, there should be 3 measurement sets in msin.
* imOut sets the image name of the output.
* Change the imaging parameters according to your preferred weighting, cleaning algorithm etc. See documentation on imager.
* The most important parameters to change are the cleaning thresholds: Cimager.threshold.minorcycle and Cimager.threshold.majorcycle. minorcyle should be ~4 sigma and majorcycle should be ~4.1 sigma based on the expected rms given the number of images you are feeding imager.
* Remember to set Cimager.nchanpercore and ntasks accordingly. ­ NCORES (bottom of slurm file) is the same as ntasks, make sure you change it as well.


2.2) Image based continuum subtraction
--------------------------------------
­As you have just imaged N MeasurementSets together, it is likely that continuum residuals have become visible and a (second) continuum subtraction is needed.
This can be accomplished by feeding imcontsub the combined image cube produced in the previous step.
Here is an example slurm file that will use imcontsub to remove continuum residuals::


    #!/bin/bash -l
    #SBATCH --partition=workq
    #SBATCH --clusters=galaxy
    # Using the default account
    # No reservation requested
    #SBATCH --time=12:00:00
    #SBATCH --ntasks=1
    #SBATCH --ntasks-per-node=1
    #SBATCH --job-name=imcontsub1_F00B09
    #SBATCH --mail-user=first.last@csiro.au
    #SBATCH --mail-type=FAIL
    #SBATCH --export=NONE
    #SBATCH --workdir=/group/askap/<user>/<object>/COMBINED_ALL
    #SBATCH --output=slurmOutput/slurm-imcontsubSL-%j.out
    # Swapping to the requested askapsoft module
    module use /group/askap/modulefiles
    module load askapdata
    module load askapsoft
    module load askappipeline
    module load casa
    beam_no="09"
    IMAGE_BASE_SPECTRAL=i.<object>.cube.combinedA.beam${beam_no}
    imageName=image.restored.wr.1.${IMAGE_BASE_SPECTRAL}
    if [ ! -e "${imageName}" ]; then
        echo "Image cube ${imageName} does not exist."
        echo "Not running image-based continuum subtraction"
    else
        # Make a working directory - the casapy & ipython log files will go in here.
        # This will prevent conflicts between simultaneous jobs
        workdir=imcontsub-working-beam${BEAM}
        mkdir -p $workdir
        cd $workdir
        pyscript=../parsets/spectral_imcontsub_F00_B${beam_no}_${SLURM_JOB_ID}.py
        cat > "$pyscript" << EOFINNER
    #!/usr/bin/env python
    # Need to import this from ACES
    import sys
    sys.path.append('/group/askap/acesops/ACES-r47210/tools')
    from robust_contsub import robust_contsub
    image="../${imageName}"
    threshold=2.0
    fit_order=2
    n_every=1
    rc=robust_contsub()
    rc.poly(infile=image,threshold=threshold,verbose=True,fit_order=fit_order,n_every=n_every,log_every=10)
    EOFINNER
        log=logs/spectral_imcontsub_F00_B${beam_no}_${SLURM_JOB_ID}.log
        NCORES=1
        NPPN=1
        #srun --export=ALL --ntasks=${NCORES} --ntasks-per-node=${NPPN} casa --nogui --nologger --log2term -c $ACES/tools/robust_contsub.py
        srun --export=ALL --ntasks=${NCORES} --ntasks-per-node=${NPPN} casa --nogui --nologger --log2term -c "$pyscript" > "$log"
        err=$?
        cd ..
        if [ $err != 0 ]; then
            exit $err
        fi
        if [ "${imageName%%.fits}" != "${imageName}" ]; then
            # Want image.contsub.fits, not image.fits.contsub
            echo "Renaming ${imageName}.contsub to ${imageName%%.fits}.contsub.fits"
            mv ${imageName}.contsub ${imageName%%.fits}.contsub.fits
        fi
    fi

Things to change / be aware of:

* Change the beam_no accordingly.
* IMAGE_BASE_SPECTRAL and imageName is where you tell imcontsub the input image. Alternatively, you could directly give it in imageName.

2.3 Mosaicking
--------------
This step is almost the same as the mosaicking task when combining in the image plane:­­ feed linmos combined, continuum subtracted images you previously made instead of the images of each individual night.
Here is an example slurm file that will use linmos to mosaic (combined) images together::

    #!/bin/bash -l
    #SBATCH --partition=workq
    #SBATCH --clusters=galaxy
    # Using the default account
    # No reservation requested
    #SBATCH --time=12:00:00
    #SBATCH --ntasks=650
    #SBATCH --ntasks-per-node=20
    #SBATCH --job-name=linmosFullScontsub
    #SBATCH --mail-user=first.last@csiro.au
    #SBATCH --mail-type=FAIL
    #SBATCH --export=NONE
    #SBATCH --workdir=/group/askap/<user>/<object>/COMBINED_ALL
    #SBATCH --output=slurmOutput/slurm-linmosS-%j.out
    # Using user-defined askapsoft module
    module use /group/askap/modulefiles
    module load askapdata
    module load askapsoft
    module load askappipeline
    IMAGETYPE_SPECTRAL="casa"
    imList="image.restored.wr.1.i.N7232.cube.B0.b10.contsub","image.restored.wr.1.i.N7232.cube.B0.b25.contsub","image.restored.wr.1.i.N7232.cube.2Arot.b08"
    wtList="weights.wr.1.i.N7232.cube.B0.b10","weights.wr.1.i.N7232.cube.B0.b25","weights.wr.1.i.N7232.cube.2Arot.b08"
    imageName=image.restored.wr.1.i.N7232.cube.combined16_R.5.linmos.contsub
    weightsImage=weights.wr.1.i.N7232.cube.combined16_R.5.linmos.contsub
    echo "Mosaicking to form ${imageName}"
    parset=parsets/science_linmos_${SLURM_JOB_ID}.in
    log=logs/science_linmos_${SLURM_JOB_ID}.log
    cat > "${parset}" << EOFINNER
    linmos.names            = [${imList}]
    linmos.weights          = [${wtList}]
    linmos.imagetype        = ${IMAGETYPE_SPECTRAL}
    linmos.outname          = ${imageName%%.fits}
    linmos.outweight        = ${weightsImage%%.fits}
    linmos.weightstate      = Inherent
    linmos.weighttype       = Combined
    linmos.cutoff           = 0.2
    EOFINNER

    NCORES=650
    NPPN=20
    srun --export=ALL --ntasks=${NCORES} --ntasks-per-node=${NPPN} linmos-mpi -c "$parset" > "$log"

Things to change / be aware of:

* imList is the list of images you give to linmos. In this example, I have given the combined, continuum subtracted images directly to the imList variable.
* wtList is the list of weights images produced by imager specified in exactly the same fashion as imList.
* imageName and weightsImage are the names of the output image and weight map from linmos.
* Provided no primary beam correction has been applied use weightstate=Inherent and weighttype=Combined
