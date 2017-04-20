#!/bin/bash -l

# ASKAPSDP-deploy.sh
# Deploys the ASKAP modules:
#   askapsoft         The askapsoft applications   
#   askappipeline     The askapsoft pipeline scripts
#   askapsoft doco    The user documentation
#   askapcli          The askap command line interface 
#   askapservices     The ingest processes, cp manager and real-time services
#
# This script attempts to build, where it can, the various modules in the background and waits until they are all finished
# until staging and deploying the results.
#
# The modules are specified in a config file, either RELEASE.cfg in the same directory as this script or whatever is
# specified with the -c switch.
#
# The ASKAP services, which run on Ingest cluster and therefore have to be built there, are remotely built by running
# the commands from a ssh shell. Communicating the finishing of builds is done through semaphore files.
#
main() {
  NOW=$(date "+%Y%m%d%H%M%S")
  NOW_LONG=$(date)

  # We have to be askapops user to run this.
  doUserCheck
  
  # Usage
  me=$(basename "$0")
  USAGE="$me [-h | ?] [-t <svn-tag>] [-l] [-c <cfg-file>]"
  
  # Defaults, these can be overridden in config file (see -c switch)
  OVERWRITE_EXISTING=FALSE
  MODULES_DIR="/group/askap/modulefiles"
  WORKING_DIR="/group/askap/askapops/workspace/askapsdp/"
  DEPLOY_DIR="/group/askap/"
  DEPLOY_ASKAPSOFT=true
  DEPLOY_ASKAP_USER_DOC=true
  DEPLOY_ASKAP_PIPELINE=true
  DEPLOY_ASKAP_SERVICES=true   # THIS NEEDS TO BE BUILT ON INGEST CLUSTER
  DEPLOY_ASKAP_CLI=true
  MAKE_MODULES_DEFAULT=true
  REPO_URL="https://svn.atnf.csiro.au/"
  LOG_FILE="${NOW}-ASKAPSDP-deploy.log"

  # Globals
  CFG_FILE="RELEASE.cfg"       # default
  CURRENT_DIR=$PWD
  TAG=""                       # e.g. CP-0.12.3
  TAG_FORMAT="^CP-([0-9])+\.([0-9])+\.([0-9])+(.)*$"
  VERSION=""                   # e.g. 0.12.3
  ASKAPSOFT_URL="${REPO_URL}/askapsdp/"
  TOS_URL="${REPO_URL}/askapsoft/Src/trunk/"
  BUILD_DIR=""
  VERBOSE=false               # change with -v, if true then echos log entries to the console
  INGEST_NODE=galaxy-ingest01.pawsey.org.au
  FORCE_RESET=false           # use -x to force reset (clears lock file)
  
  # Build finished flags
  BUILD_ASKAPSOFT_DONE_FLAG="build-askapsoft-done"
  BUILD_CPSVCS_DONE_FLAG="build-cpsvcs-done"
  BUILD_CLI_DONE_FLAG="build-cli-done"
  BUILD_FAILED_FLAG="build-error"

  # svn settings - these have to be overridden in svn.cfg, if not then SSubversion will prompt for them so not good for unattended run
  SVN_USERNAME=""
  SVN_PASSWORD=""
  
  # Errors
  NO_ERROR=0
  ERROR_BAD_OVERRIDES=64
  ERROR_OVERWRITE_CONFLICT=65
  ERROR_BAD_SVN_TAG=66
  ERROR_MISSING_ENV=67
  ERROR_MISSING_CFG_FILE=68
  ERROR_NOT_ASKAPOPS=69
  ERROR_ALREADY_RUNNING=70
  ERROR_WRONG_CLUSTER=71
  ERROR_TIMEOUT=72
  ERROR_BUILD_FAILED=73
  TOLD_TO_DIE=74             # used on terminate SIGnals
  
  TIMEOUT_VALUE=14400        # 4 hours

  trap '{ cleanup ${TOLD_TO_DIE}; }' SIGHUP SIGINT SIGTERM

  # svn overrides are in the svn.cfg file
  if [ -e "svn.cfg" ]; then
    if ! source "svn.cfg"; then
      echo "Could not process overrides file svn.cfg" >&2
      exit $ERROR_BAD_OVERRIDES;
    fi
  fi

  # Process the command line switches
  processCmdLine "$@" 

  # Overrides are in the cfg file
  if [ -e "$CFG_FILE" ]; then
    if ! source "${CFG_FILE}"; then
      echo "Could not process overrides file ${CFG_FILE}" >&2
      exit $ERROR_BAD_OVERRIDES;
    fi
  fi

  # We can only run this on galaxy
  detectHost

  # We only want to run once at a time
  doLockCheck
  
  # Set up the dirs
  BUILD_DIR="${WORKING_DIR}/${TAG}/build"
  mkdir -p "${BUILD_DIR}"

  # Create log file
  LOG_FILE="${WORKING_DIR}/${TAG}/${LOG_FILE}"
  touch "${LOG_FILE}"
  log "DEPLOYMENT OF ASKAP MODULES ${NOW_LONG}"
  
  # Check if we actually have anything to do
  if [ "$DEPLOY_ASKAPSOFT" == false ] && [ "$DEPLOY_ASKAP_PIPELINE" == false ] && [ "$DEPLOY_ASKAP_SERVICES" == false ] && [ "$DEPLOY_ASKAP_CLI" == false ] && [ "$DEPLOY_ASKAP_USER_DOC" == false ]; then
    logerr "There is nothing to do; all targets are set to NOT deploy"
    cleanup $NO_ERROR;
  fi

  # The TAG will have been provided on the command line with -t or defaulted with -l
  log "TAG = ${TAG}"
  VERSION=$(echo $TAG | cut -d'-' -f 2)
  ASKAPSOFT_URL="${ASKAPSOFT_URL}/tags/${TAG}"

  # Log the environment we are using
  logENV

  # Check if it already exists in the modules area and if overwrite is set to false then will bail
  doOverwriteChecks

  # Do the builds
  if [ "$DEPLOY_ASKAPSOFT" == true ] || [ "$DEPLOY_ASKAP_PIPELINE" == true ] || [ "$DEPLOY_ASKAP_USER_DOC" == true ]; then
    buildAskap
    log "building ASKAPsoft"
  fi

  if [ "$DEPLOY_ASKAP_SERVICES" == true ]; then
    buildServices
    log "building services"
  fi

  if [ "$DEPLOY_ASKAP_CLI" == true ]; then
    buildCli
    log "building cli"
  fi
  
  # Stage the build
  stage
  log "staging build"

  # Deploy the modules
  deploy
  log "deploying build"
  
  # Finish
  cleanup $NO_ERROR
  
  cd "${CURRENT_DIR}" || exit 0
}

# Build the ASKAP modules askapsoft and pipelines and include a doco update.
buildAskap() {
  log "Building the ASKAP modules"

  cd "${WORKING_DIR}/${TAG}" || cleanup $ERROR_MISSING_ENV

  # Get the code if it's not there
  if [ ! -e "askapsoft-${TAG}" ]; then
    log "Retrieving the code base from ${ASKAPSOFT_URL} into askapsoft-${TAG}"
    svn co -q --username ${SVN_USERNAME} --password ${SVN_PASSWORD} "${ASKAPSOFT_URL}" "askapsoft-${TAG}"
  else
    log "The askapsoft-${TAG} directory already exists, using that"
    svn update -q --username ${SVN_USERNAME} --password ${SVN_PASSWORD} "askapsoft-${TAG}"
  fi
  
  # Copy for ingest cluster build if required
  if [ "$DEPLOY_ASKAP_SERVICES" == true ]; then
    log "CP services required and will be built later; but need to build on ingest cluster; so need an independent environment"
    if [ -e "askapingest-${TAG}" ]; then
      log "askapingest-${TAG} already exists, so using that"
    else
      log "Copying askapsoft-${TAG} to askapingest-${TAG}"
      cp -R "askapsoft-${TAG}" "askapingest-${TAG}"
    fi
  fi
  
  # Create a build command which will run initialisation and all required builds in the background.
  local BUILD_COMMAND=""
  
  # Check to see if we have already bootstrapped the env
  if [ ! -e "askapsoft-${TAG}/initaskap.sh" ]; then
    BUILD_COMMAND+="python2.7 bootstrap.py -n; "
  fi
  
  BUILD_COMMAND+="source initaskap.sh; "

    # build askapsoft if specified
  if [ "$DEPLOY_ASKAPSOFT" == true ]; then
    log "ASKAPsoft cpapps will be built and deployed"
    BUILD_COMMAND+="rbuild -n --release-name \"ASKAPsoft-cpapps-${TAG}\" --stage-dir \"ASKAPsoft-cpapps-${TAG}\" -t release Code/Systems/cpapps; "
  fi

  # build pipelines if specified
  if [ "$DEPLOY_ASKAP_PIPELINE" == true ]; then
    log "ASKAPsoft processing pipeline will be built and deployed"
    BUILD_COMMAND+="rbuild -n -V --release-name \"ASKAPsoft-pipelines-${TAG}\" --stage-dir \"ASKAPsoft-pipelines-${TAG}\" -t release Code/Systems/pipelines; "
  fi

  # build askapdoc if specified
  if [ "$DEPLOY_ASKAP_USER_DOC" == true ]; then
    log "ASKAPsoft user documentation will be built and deployed"

    # Update version numbers on documentation conf.py file by replacing the default snapshot svn revision number (version = 'r' ...)
    # with the release number. Only do this if it hasn't been done already (someone may have done this manually in svn before).
    if [ "$(grep -q '^version .* svnversion' askapsoft-${TAG}/Code/Doc/user/current/doc/conf.py)" == 0 ]; then
      sed -e -i "s/^version = 'r'/#version = 'r'/" -e "s/^release = version/#release = version/" -e "s/^#version = '0.1'/version = '${VERSION}'/" -e "s/^#release = '0.1-draft'/release = '${VERSION}'/" askapsoft-${TAG}/Code/Doc/user/current/doc/conf.py
      svn ci -m "Updated version numbers for ASKAPsoft documentation." askapsoft-${TAG}/Code/Doc/user/current/doc/conf.py --username ${SVN_USERNAME} --password ${SVN_PASSWORD}
    fi
    
    BUILD_COMMAND+="rbuild -n -t doc Code/Doc/user/current; "
  fi
  
  BUILD_COMMAND+="touch \"${BUILD_DIR}/build-askapsoft-done\"; "
  
  cd "askapsoft-${TAG}" || cleanup $ERROR_MISSING_ENV
  
  log "Starting ASKAPsoft build in background"
  log "$BUILD_COMMAND"
  (eval ${BUILD_COMMAND}) &

}

# Build the ASKAP services including the CP Manager. This will need to be built on ingest cluster.
# However a checkout will not be necessary as it can be done from this environment (i.e. /group).
# q. Can this be done remotely from here?
buildServices() {
  log "Building the ASKAP services"

  cd "${WORKING_DIR}/${TAG}" || cleanup $ERROR_MISSING_ENV
  
  # Create a build command that will be run on the ingest node. 
  local BUILD_COMMAND="(cd \"${WORKING_DIR}/${TAG}/askapingest-${TAG}\"; "

  # Check it out of it's not there
  if [ ! -e "askapingest-${TAG}" ]; then
    log "Retrieving the code base from ${ASKAPSOFT_URL} into askapsoft-${TAG}"
    svn co -q --username ${SVN_USERNAME} --password ${SVN_PASSWORD} "${ASKAPSOFT_URL}" "askapingest-${TAG}"
  else
    log "The askapingest-${TAG} directory already exists, using that"
    svn update -q --username ${SVN_USERNAME} --password ${SVN_PASSWORD} "askapingest-${TAG}"
  fi

  # Check to see if we have already bootstrapped the env
  if [ ! -e "askapingest-${TAG}/initaskap.sh" ]; then
    BUILD_COMMAND="${BUILD_COMMAND} \
                   python2.7 bootstrap.py -n; "
  fi

  BUILD_COMMAND="${BUILD_COMMAND} \
                 source initaskap.sh; \
                 rbuild -n -V --release-name \"ASKAPsoft-cpsvcs-${TAG}\" --stage-dir \"ASKAPsoft-cpsvcs-${TAG}\" -t release Code/Systems/cpsvcs; \
                 touch \"${BUILD_DIR}/build-cpsvcs-done\") "
                       
  # run this on ingest
  log "Starting services build on ingest (${INGEST_NODE})"
  ssh -v -t askapops@${INGEST_NODE} "${BUILD_COMMAND}" &
}

# Build the CLI tools. A checkout of the TOS trunk is required for this.
# This will extend the time taken to run the script as it has to checkout and initialise and build a 
# separate environment.
buildCli() {
  log "Building the CLI tools"

  cd "${WORKING_DIR}/${TAG}" || cleanup $ERROR_MISSING_ENV

  # Create a build command which will run initialisation and all required builds in the background.
  local BUILD_COMMAND=""
                       
  # Check it out of it's not there
  if [ ! -e "askaptos-trunk" ]; then
    log "Retrieving the code base from ${TOS_URL} into askaptos-trunk"
    svn co -q --username ${SVN_USERNAME} --password ${SVN_PASSWORD} "${TOS_URL}" askaptos-trunk
  else
    log "The askaptos-trunk directory already exists, using that"
    svn update -q --username ${SVN_USERNAME} --password ${SVN_PASSWORD} "askaptos-trunk"
  fi
  
  # Check to see if we have already bootstrapped the env
  if [ ! -e askaptos-trunk/initaskap.sh ]; then
    BUILD_COMMAND="${BUILD_COMMAND} \
                   python2.7 bootstrap.py -n; "
  fi
  
  BUILD_COMMAND="${BUILD_COMMAND} \
                 source initaskap.sh; \
                 rbuild -n -V --release-name \"ASKAP-cli-${TAG}\" --stage-dir \"ASKAP-cli-${TAG}\" -t release Code/Components/UI/cli/current; \
                 touch \"${BUILD_DIR}/build-cli-done\""
  
  cd askaptos-trunk || cleanup $ERROR_MISSING_ENV
    
  log "Starting ASKAP CLI build in background"
  log "$BUILD_COMMAND"
  (eval "${BUILD_COMMAND}") &

}

# Wait for the artefacts to be built and then stage the build.
# Times out of it has to wait too long (see $TIMEOUT_VALUE).
stage() {
  cd "${BUILD_DIR}" || cleanup $ERROR_MISSING_ENV "Cannot find ${BUILD_DIR}"

  # Wait for builds to finish
  # true if the build-xxxx-done files are present for the required builds (including the services one running on ingest)
  log "Waiting for background and remote builds to finish"
  local ALL_DONE=false
  local WAIT_TIME=60
  local ELAPSED_TIME=0
  declare -a SEMAPHORES
  local IDX=0

  # Check for ASKAPsoft done if we are building any of askap apps, askap pipeline, askap doc
  if [ "$DEPLOY_ASKAPSOFT" == true ] || [ "$DEPLOY_ASKAP_PIPELINE" == true ] || [ "$DEPLOY_ASKAP_USER_DOC" == true ]; then
    SEMAPHORES[$IDX]="$BUILD_ASKAPSOFT_DONE_FLAG"
    IDX=$((IDX + 1))
  fi

  # Check for ASKAP CLI done if we are building the command line interface
  if [ "$DEPLOY_ASKAP_CLI" == true ]; then
    SEMAPHORES[$IDX]="$BUILD_CLI_DONE_FLAG"
    IDX=$((IDX + 1))
  fi

  # Check for ASKAP Services done if we are building ingest (on ingest cluster)
  if [ "$DEPLOY_ASKAP_SERVICES" == true ]; then
    SEMAPHORES[$IDX]="$BUILD_CPSVCS_DONE_FLAG"
    IDX=$((IDX + 1))
  fi
  
  while [ "$ALL_DONE" == false ]; do
  
    # check all the semaphores - all semaphore files will have to be present for ALL_DONE to stay true after this loop
    # Assume ALL_DONE
    ALL_DONE=true

    # Now prove it wrong ...
    for FLAG in "${SEMAPHORES[@]}"; do
      if [ ! -e $FLAG ]; then
        ALL_DONE=false
        break
      else
        log "${FLAG}"
      fi
    done
    
    # if any were not done then wait again
    if [ "$ALL_DONE" == false ]; then
      if [ "$ELAPSED_TIME" -gt "$TIMEOUT_VALUE" ]; then
        cleanup $ERROR_TIMEOUT "Timeout reached while waiting for builds to finish - aborting!\n";
      fi
      if [ -e "$BUILD_FAILED_FLAG" ]; then
        cleanup $ERROR_BUILD_FAILED "One or more of the necessary builds failed to finish - aborting!\n"; 
      fi
      sleep $WAIT_TIME
      ELAPSED_TIME=$((ELAPSED_TIME + WAIT_TIME))
    fi
  done
  log "All required builds complete; staging the build ..."

  # stage askapsoft if specified
  if [ "$DEPLOY_ASKAPSOFT" == true ]; then
    
    (tar xf "${WORKING_DIR}/${TAG}/askapsoft-${TAG}/Code/Systems/cpapps/ASKAPsoft-cpapps-${TAG}.tgz" || exit 1; \
     cd "ASKAPsoft-cpapps-${TAG}" || exit 1; \
     rm -rf build-1 config docs ICE_LICENSE include initenv.py local man share slice ssl || exit 1;) || \
    cleanup $ERROR_MISSING_ENV "Cannot prepare tar file askapsoft-${TAG}/ASKAPsoft-cpapps-${TAG}.tgz"
    
    # fix up permissions
    chgrp -R askap "ASKAPsoft-cpapps-${TAG}"
    find "ASKAPsoft-cpapps-${TAG}" -type f -exec chmod oug+r {} +
    find "ASKAPsoft-cpapps-${TAG}" -type d -exec chmod oug+rx {} +
    
    # Create a module file
    sed -e "s/\${_VERSION}.*/${VERSION}/" "${WORKING_DIR}/${TAG}/askapsoft-${TAG}/Tools/Dev/deploy/askapsoft.module-template" > "askapsoft.module"
    
  fi

  # stage pipelines if specified
  if [ "$DEPLOY_ASKAP_PIPELINE" == true ]; then
    tar xf "${WORKING_DIR}/${TAG}/askapsoft-${TAG}/Code/Systems/pipelines/ASKAPsoft-pipelines-${TAG}.tgz"
    
    # fix up permissions
    chgrp -R askap "ASKAPsoft-pipelines-${TAG}"
    find "ASKAPsoft-pipelines-${TAG}" -type f -exec chmod oug+r {} +
    find "ASKAPsoft-pipelines-${TAG}" -type d -exec chmod oug+rx {} +
    
    # Create a module file
    sed -e "s/\${_VERSION}.*/${VERSION}/" "${WORKING_DIR}/${TAG}/askapsoft-${TAG}/Tools/Dev/deploy/askappipeline.module-template" > "askappipeline.module"

  fi

  # stage askapcli if specified
  if [ "$DEPLOY_ASKAP_CLI" == true ]; then
    tar xf "${WORKING_DIR}/${TAG}/askaptos-trunk/Code/Components/UI/cli/current/ASKAP-cli-${TAG}.tgz"

    # fix up permissions
    chgrp -R askap "ASKAP-cli-${TAG}"
    find "ASKAP-cli-${TAG}" -type f -exec chmod oug+r {} +
    find "ASKAP-cli-${TAG}" -type d -exec chmod oug+rx {} +
    
    # Create a module file
    sed -e "s/\${_VERSION}.*/${VERSION}/" "${WORKING_DIR}/${TAG}/askapsoft-${TAG}/Tools/Dev/deploy/askapcli.module-template" > "askapcli.module"
  fi

  # stage askapdoc if specified
  if [ "$DEPLOY_ASKAP_USER_DOC" == true ]; then
    mv "${WORKING_DIR}/${TAG}/askapsoft-${TAG}/Code/Doc/user/current/doc/_build/html" .
  fi
  
  # stage askapservices if specified
  if [ "$DEPLOY_ASKAP_SERVICES" == true ]; then
    (tar xf "${WORKING_DIR}/${TAG}/askapingest-${TAG}/Code/Systems/cpsvcs/ASKAPsoft-cpsvcs-${TAG}.tgz"; \
     cd "ASKAPsoft-cpsvcs-${TAG}"; \
     rm -rf build-1 config docs ICE_LICENSE include initenv.py local man share slice ssl;) || \
    cleanup ERROR_MISSING_ENV "Cannot prepare tar file askapingest-${TAG}/ASKAPsoft-cpsvcs-${TAG}.tgz"
    
    # fix up permissions
    chgrp -R askap "ASKAPsoft-cpsvcs-${TAG}"
    find "ASKAPsoft-cpsvcs-${TAG}" -type f -exec chmod oug+r {} +
    find "ASKAPsoft-cpsvcs-${TAG}" -type d -exec chmod oug+rx {} +
    
    # Create a module file
    sed -e "s/\${_VERSION}.*/${VERSION}/" "${WORKING_DIR}/${TAG}/askapingest-${TAG}/Tools/Dev/deploy/askapservices.module-template" > "askapservices.module"
  fi
}

# Deploy the built modules; some might not have been built, but the ones that have are in the staging dir (build).
# This should not be called if there are any errors in the build process.
deploy() {
  log "Deploying the ASKAP modules to ${DEPLOY_DIR}"

  # Deploy the askap apps, if they were built, along with module configuration file
  if [ -e "${BUILD_DIR}/ASKAPsoft-cpapps-${TAG}" ]; then

    # backup the old one if it is there
    if [ -e "${DEPLOY_DIR}/askapsoft/${VERSION}" ]; then
      mv "${DEPLOY_DIR}/askapsoft/${VERSION}" "${DEPLOY_DIR}/askapsoft/${VERSION}.${NOW}"
    fi

    mv  "${BUILD_DIR}/ASKAPsoft-cpapps-${TAG}" "${DEPLOY_DIR}/askapsoft/${VERSION}"
    mv "${BUILD_DIR}/askapsoft.module" "${MODULES_DIR}/askapsoft/${VERSION}"
    if [ "$MAKE_DEFAULT" == true ]; then
      # change the default referenced version
      sed -i -e "s/^set ModulesVersion.*/set ModulesVersion \"${VERSION}\"/" "${MODULES_DIR}/askapsoft/.version"
    fi
  fi
  
  # Deploy the askap pipeline scripts, if they were built, along with module configuration file
  if [ -e "${BUILD_DIR}/ASKAPsoft-pipelines-${TAG}" ]; then

    # backup the old one if it is there
    if [ -e "${DEPLOY_DIR}/askappipeline/${VERSION}" ]; then
      mv "${DEPLOY_DIR}/askappipeline/${VERSION}" "${DEPLOY_DIR}/askappipeline/${VERSION}.${NOW}"
    fi

    mv "${BUILD_DIR}/ASKAPsoft-pipelines-${TAG}" "${DEPLOY_DIR}/askappipeline/${VERSION}"
    mv "${BUILD_DIR}/askappipeline.module" "${MODULES_DIR}/askappipeline/${VERSION}"
    if [ "$MAKE_DEFAULT" == true ]; then
      # change the default referenced version
      sed -i -e "s/^set ModulesVersion.*/set ModulesVersion \"${VERSION}\"/" "${MODULES_DIR}/askappipeline/.version"
    fi
  fi
  
  # Deploy the askap cli, if it was built; module configuration file is always 'current'
  if [ -e "${BUILD_DIR}/ASKAP-cli-${TAG}" ]; then

    # backup the old one if it is there
    if [ -e "${DEPLOY_DIR}/askapcli/${VERSION}" ]; then
      mv "${DEPLOY_DIR}/askapcli/${VERSION}" "${DEPLOY_DIR}/askapcli/${VERSION}.${NOW}"
    fi

    mv "${BUILD_DIR}/askapcli.module" "${MODULES_DIR}/askapcli/current"
    mv "${BUILD_DIR}/ASKAP-cli-${TAG}" "${DEPLOY_DIR}/askapcli/${VERSION}"
    rm "${DEPLOY_DIR}/askapcli/current"
    ln -s "${DEPLOY_DIR}/askapcli/${TAG}" "${DEPLOY_DIR}/askapcli/current"
  fi

  # Deploy the askap cpsvcs, if they were built, along with module configuration file
  if [ -e "${BUILD_DIR}/ASKAPsoft-cpsvcs-${TAG}" ]; then

    # backup the old one if it is there
    if [ -e "${DEPLOY_DIR}/askapcpsvcs/${VERSION}" ]; then
      mv "${DEPLOY_DIR}/askapcpsvcs/${VERSION}" "${DEPLOY_DIR}/askapcpsvcs/${VERSION}.${NOW}"
    fi

    mv "${BUILD_DIR}/ASKAPsoft-cpsvcs-${TAG}" "${DEPLOY_DIR}/askapservices/${VERSION}"
    mv "${BUILD_DIR}/askapservice.module" "${MODULES_DIR}/askapservices/${VERSION}"
    if [ "$MAKE_DEFAULT" == true ]; then
      # change the default referenced version
      sed -i -e "s/^set ModulesVersion.*/set ModulesVersion \"${VERSION}\"/" "${MODULES_DIR}/askapservices/.version"
    fi
  fi
  
}

# Cleanup. Only call this once we are definitely running, so after command line processing and lock check.
cleanup () {
  if [ -e /tmp/ASKAPDEPLOY.lock ]; then
    rm /tmp/ASKAPDEPLOY.lock
    rm -rf "${BUILD_DIR}"
  fi
  if [ -n "$2" ]; then
    logerr "$2"
  fi

  # Change the group of all the build artefacts in the code tree resulting from rbuild calls otherwise
  # askapops quota will run out.
  chgrp -R askap "${WORKING_DIR}/${TAG}"

  # Kill all the sub proceses
  #if [ $1 -eq $TOLD_TO_DIE ]; then
    pkill -P $$
  #fi

  exit "$1"
}

# This will be called once command line and other setup has proceeded so appropriate paths etc are set, if reset flag encountered.
# cleanup and exit.
forceReset() {
    cleanup $NO_ERROR;
}

showHelp() {
  printf "%s \n" "$USAGE"
  printf "Make a release of ASKAPsoft.\n"
  printf "    -h | ?            this help\n"
  printf "    -v                verbose\n"
  printf "    -t svn-tag        is optional, if not given must specify -l\n"
  printf "    -l                use the latest tag in the repository (caution)\n"
  printf "    -c cfg-file       is optional, if not given and a file RELEASE.cfg is not present in the current dir then no overrides will be used\n"
  printf "\n"
  printf "You can override the following settings by editing a scriptable config file and passing it via -c switch \n"
  printf "(or use the default RELEASE.cfg). This script will be sourced to set up the deploy environment. \n"
  printf "Unless overriden in this way, the defaults are:\n\n"
  printf "  export WORKING_DIR='/group/askap/askapops/workspace/askapsdp/'\n"
  printf "  export OVERWRITE_EXISTING=false\n"
  printf "  export DEPLOY_DIR='/group/askap/'\n"
  printf "  export MODULES_DIR='/group/askap/modulefiles'\n"
  printf "  export DEPLOY_ASKAPSOFT=true\n"
  printf "  export DEPLOY_ASKAP_USER_DOC=true\n"
  printf "  export DEPLOY_ASKAP_PIPELINE=true\n"
  printf "  export DEPLOY_ASKAP_SERVICES=true\n"
  printf "  export DEPLOY_ASKAP_CLI=true\n"
  printf "  export MAKE_MODULES_DEFAULT=true\n"
  printf "  export REPO_URL='https://svn.atnf.csiro.au/'\n"
  printf "  export LOG_FILE='./%s'\n" "${NOW}-ASKAPSDP-deploy.log"
}

# Logger for logging messages
log() {
  printf "%s - " "$(date '+%Y%m%d %H:%M:%S')" >> "$LOG_FILE"
  printf "%s " "$@" >> "$LOG_FILE"
  printf "\n" >> "$LOG_FILE"
  if [ ${VERBOSE} == true ]; then
    printf "%s " "$@"
    printf "\n"
  fi
}
logerr() {
  # Log to file and stderr
  log "ERROR:" "$@"
  printf "ERROR: " 1>&2
  printf "%s " "$@" 1>&2
  printf "\n" 1>&2
}

# Process the command line parameters.
# Note: the logger has not been initialise yet so don't use log()
#
# Valid parameters:
#   -h | ?        Help
#   -t svn-tag    The tagged release to use from svn
#   -v            Verbose mode
#   -l            Use the latest tag
#   -c cfg-file   Specify a config file for overrides
#   -x            Force a reset (clears lock flag); this switch is not public and does appear in the help
#
processCmdLine() {

  # For option processing
  OPTIND=1

  # Process our options
  while getopts "h?vlt:c:x" opt "$@"; do
      case "$opt" in
      
      h|\?)
          showHelp
          exit $NO_ERROR
          ;;
      v)
          VERBOSE=true
          ;;
      t)
          if [[ "${OPTARG}" =~ $TAG_FORMAT ]]; then
            TAG=$OPTARG
          else
            printf "The tag %s does not match semantic versioning string format [CP-0.12.3]\n" "${OPTARG}"
            exit $ERROR_BAD_SVN_TAG;
          fi
          ;;
      l)
          findLatestTag
          ;;
      c)
          if  [ -e "$OPTARG" ]; then
            CFG_FILE=$OPTARG
          else
            printf "The given config file %s does not exist\n" "${OPTARG}"
            exit $ERROR_MISSING_CFG_FILE;
          fi
          ;;
      x)
      # force reset. This flag is not advertised. It is here only so people in the know can force the deletion of the i
      # process lock file.
          FORCE_RESET=true
          return;
          ;;
      esac
  done

  shift $((OPTIND-1))

  [ "$1" = "--" ] && shift
  
  if [ "$TAG" == "" ]; then 
    printf "No TAG has been specified\n"
    printf "%s\n" "${USAGE}"
    exit "$ERROR_BAD_SVN_TAG";
  fi

}

# find the latest tag; turn on extended patterns; match semantic versions, i.e. MM.mm.pp, e.g. 0.12.3
findLatestTag() {
  local LATEST_TAG=""
  shopt -s extglob

  log "Using the latest tag from subversion for ASKAPsoft"

  LATEST_TAG=$(svn --username ${SVN_USERNAME} --password ${SVN_PASSWORD} ls -v https://svn.atnf.csiro.au/askapsdp/tags | sort | tail -n 1 | awk -F ' ' '{print $NF}' | cut -d'/' -f 1)

  if [[ ${LATEST_TAG} =~ $TAG_FORMAT ]]; then
    log "The latest tag is ${LATEST_TAG}"
    TAG=${LATEST_TAG}
  else
    logerr "The tag ${LATEST_TAG} does not match semantic versioning string format [CP-0.12.3]"
    cleanup $ERROR_BAD_SVN_TAG;
  fi
}

# Print the environment to the log file
logENV () {
  log "OVERWRITE_EXISTING =" "$([ $OVERWRITE_EXISTING = true ] && echo "true" || echo "false")"
  log "MAKE_MODULES_DEFAULT =" "$([ $MAKE_MODULES_DEFAULT = true ] && echo "true" || echo "false")"
  log "MODULES_DIR =" "$MODULES_DIR"
  log "WORKING_DIR =" "$WORKING_DIR"
  log "DEPLOY_DIR =" "$DEPLOY_DIR"
  log "DEPLOY_ASKAPSOFT =" "$([ $DEPLOY_ASKAPSOFT = true ] && echo "true" || echo "false")"
  log "DEPLOY_ASKAP_PIPELINE =" "$([ $DEPLOY_ASKAP_PIPELINE = true ] && echo "true" || echo "false")"
  log "DEPLOY_ASKAP_SERVICES =" "$([ $DEPLOY_ASKAP_SERVICES = true ] && echo "true" || echo "false")"
  log "DEPLOY_ASKAP_CLI =" "$([ $DEPLOY_ASKAP_CLI = true ] && echo "true" || echo "false")"
  log "ASKAPSOFT_URL =" "$ASKAPSOFT_URL"
  log "TOS_URL =" "$TOS_URL"
  log "SVN_USERNAME =" "$SVN_USERNAME"
}

# Check what is going to be overwritten. Combined with existing cfg after this we will know what we have
# to do (if anything).
doOverwriteChecks() {
  local BAIL=false

  if [ -e "${MODULES_DIR}" ]; then

    # Check for existing askapsoft module
    if [ "$OVERWRITE_EXISTING" == false ] && [ "$DEPLOY_ASKAPSOFT" == true ] && [ -e "${MODULES_DIR}/askapsoft/${TAG}" ]; then
      logerr "Cannot overwrite existing module" "${MODULES_DIR}/askapsoft/${TAG}"
      BAIL=true
    fi
    if [ "$OVERWRITE_EXISTING" == true ] && [ "$DEPLOY_ASKAPSOFT" == true ] && [ -e "${MODULES_DIR}/askapsoft/${TAG}" ]; then
      log "OVERWRITE existing module" "${MODULES_DIR}/askapsoft/${TAG}"
    fi

    # Check for existing askappipeline module
    if [ "$OVERWRITE_EXISTING" == false ] && [ "$DEPLOY_ASKAP_PIPELINE" == true ] && [ -e "${MODULES_DIR}/askappipeline/${TAG}" ]; then
      logerr "Cannot overwrite existing module" "${MODULES_DIR}/askappipeline/${TAG}"
      BAIL=true
    fi
    if [ "$OVERWRITE_EXISTING" == true ] && [ "$DEPLOY_ASKAP_PIPELINE" == true ] && [ -e "${MODULES_DIR}/askappipeline/${TAG}" ]; then
      log "OVERWRITE existing module" "${MODULES_DIR}/askappipeline/${TAG}"
      BAIL=true
    fi

    # Check for existing askapservices module
    if [ "$OVERWRITE_EXISTING" == false ] && [ "$DEPLOY_ASKAP_SERVICES" == true ] && [ -e "${MODULES_DIR}/askapservices/${TAG}" ]; then
      logerr "Cannot overwrite existing module" "${MODULES_DIR}/askapservices/${TAG}"
      BAIL=true
    fi
    if [ "$OVERWRITE_EXISTING" == true ] && [ "$DEPLOY_ASKAP_SERVICES" == true ] && [ -e "${MODULES_DIR}/askapservices/${TAG}" ]; then
      log "OVERWRITE existing module" "${MODULES_DIR}/askapservices/${TAG}"
    fi

    # Check for existing askapcli module
    if [ "$OVERWRITE_EXISTING" == false ] && [ "$DEPLOY_ASKAP_CLI" == true ] && [ -e "${MODULES_DIR}/askapcli/${CLI_REV}" ]; then
      logerr "Cannot overwrite existing module" "${MODULES_DIR}/askapcli/${CLI_REV}"
      BAIL=true
    fi
    if [ "$OVERWRITE_EXISTING" == true ] && [ "$DEPLOY_ASKAP_CLI" == true ] && [ -e "${MODULES_DIR}/askapcli/${CLI_REV}" ]; then
      log "OVERWRITE existing module" "${MODULES_DIR}/askapcli/${CLI_REV}"
    fi

    # exit if we have any overwrite check fails
    if [ "$BAIL" == true ]; then
      cleanup $ERROR_OVERWRITE_CONFLICT
    fi
  else
    logerr "The modules directory does not exist," "${MODULES_DIR}" 
    cleanup $ERROR_MISSING_ENV;
  fi
}

# We only want this to run as the asksapops user
doUserCheck () {
  if [[ $USER != "askapops" ]]; then 
    logerr "This script must be run as askapops!" 
    exit $ERROR_NOT_ASKAPOPS;
  fi 
}

# Handling a semaphore file to prevent running build twice. Don't call cleanup here or it will clear the lock!
# If the deploy bombs out for some reason leaving the lock flag then call forceReset() which can be invoked via the -x i
# command line switch.
doLockCheck () {
  # if -x was on command line then force reset
  if [ "$FORCE_RESET" == true ]; then
      forceReset
  fi
  
  if [ -e /tmp/ASKAPDEPLOY.lock ]; then
    printf "ERROR: This script is already running - aborting!\n"  1>&2
    exit $ERROR_ALREADY_RUNNING;
  fi
  touch /tmp/ASKAPDEPLOY.lock
}

# We need to know if we are running on galaxy.
detectHost () {
  shopt -s extglob
  hostmatch="^galaxy-[0-9].*$"

  if [[ ${HOSTNAME} =~ $hostmatch ]]; then
    GALAXY_CLUSTER=true;
  else
     printf "ERROR: Cannot run on this machine, must be galaxy!\n"
     exit $ERROR_WRONG_CLUSTER;
  fi
}

main "$@"
