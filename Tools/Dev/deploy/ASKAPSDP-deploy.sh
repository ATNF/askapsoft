#!/bin/bash -l

# ASKAPSDP-deploy.sh
# Deploys the ASKAP modules:
#   askapsoft         The askapsoft applications
#   askappipeline     The askapsoft pipeline scripts
#   askapcli          The askap command line interface
#   askapservices     The ingest processes, cp manager and real-time services
#   askapsoft doco    The user documentation
#
main() {

  doUserCheck
  
  # Usage
  me=$(basename "$0")
  USAGE="$me [-h | ?] [-t <svn-tag>] [-l] [-c <cfg-file>]"
  
  # Defaults, these can be overriden in config file (see -c switch)
  OVERWRITE_EXISTING=FALSE
  MODULES_DIR="/group/askap/modulefiles"
  WORKING_DIR="/group/astronomy856/askapops/workspace/askapsdp/"
  DEPLOY_DIR="/group/askap/"
  DEPLOY_ASKAPSOFT=true
  DEPLOY_ASKAP_USER_DOC=true
  DEPLOY_ASKAP_PIPLINE=true
  DEPLOY_ASKAP_SERVICES=true   # THIS NEEDS TO BE BUILT ON INGEST CLUSTER
  DEPLOY_ASKAP_CLI=true
  MAKE_MODULES_DEFAULT=true
  REPO_URL="https://svn.atnf.csiro.au/"
  LOG_FILE="${NOW}-ASKAPSDP-deploy.log"

  # Globals
  NOW=`date "+%Y%m%d%H%M%S"`
  NOW_LONG=`date`
  CFG_FILE="RELEASE.cfg"
  CURRENT_DIR=$PWD
  TAG=""
  TAG_FORMAT="^CP-([0-9])+\.([0-9])+\.([0-9])+(.)*$"
  VERSION=$(echo $TAG | cut -d'-' -f 2)
  ASKAPSOFT_URL="${REPO_URL}/askapsdp/"
  TOS_URL="${REPO_URL}/askapsoft/trunk/"
  BUILD_DIR=""
  VERBOSE=false
  INGEST_NODE=galaxy-ingest01.pawsey.org.au
  
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
  TOLD_TO_DIE=72

  trap "{ cleanup ${TOLD_TO_DIE}; }" SIGHUP

  processCmdLine "$@" 

  doLockCheck
  
  detectHost

  # Overrides are in the cfg file
  if [ -e $CFG_FILE ]; then
    source $CFG_FILE
    if [ $? -ne 0 ]; then
      echo "Could not process overrides file ${CFG_FILE}" >&2
      cleanup $ERROR_BAD_OVERRIDES;
    fi
  fi

  # Create log file
  LOG_FILE="${WORKING_DIR}/${TAG}/${LOG_FILE}"
  touch $LOG_FILE
  log "DEPLOYMENT OF ASKAP MODULES ${NOW_LONG}\n\n"

  # The TAG will have been provided on the command line with -t or defaulted with -l
  log "TAG = ${TAG}\n"
  ASKAPSOFT_URL=${ASKAPSOFT_URL}/tags/${TAG}

  # Log the environment we are using
  logENV

  # Check if it already exists in the modules area and if overwrite is set to false then will bail
  doOverwriteChecks

  # Perform the necessary builds
  
  mkdir ${WORKING_DIR}/${TAG}
  mkdir ${WORKING_DIR}/${TAG}/build
  BUILD_DIR="${WORKING_DIR}/${TAG}/build"

  if [ "$DEPLOY_ASKAPSOFT" == true ] || [ "$DEPLOY_ASKAP_PIPLINE" == true ]; then
    buildAskap
  fi

  if [ "$DEPLOY_ASKAP_SERVICES" == true ]; then
    buildServices
  fi

  if [ "$DEPLOY_ASKAP_CLI" == true ]; then
    buildCli
  fi
  
  # Stage the build
  stage

  # Deploy the modules
  deploy
  
  # Finish
  cleanup $NO_ERROR
}

# Build the ASKAP modules askapsoft and pipelines and include a doco update.
buildAskap() {
  log "Building the ASKAP modules\n"

  cd ${WORKING_DIR}/${TAG}

  # Get the code if it's not there
  if [ ! -e askapsoft-${TAG} ]; then
    svn export -q ${ASKAPSOFT_URL} askapsoft-${TAG}
  fi
  
  cd askapsoft-${TAG}
  
  # Copy for ingest cluster build if required
  if [ "$DEPLOY_ASKAP_SERVICES" == true ]; then
    cp -R ../askapsoft-${TAG} ../askapingest-${TAG}
  fi
  
  # Create a build command which will run initialisation and all required builds in the background.
  local BUILD_COMMAND="python2.7 bootstrap.py -n;
                       source initaskap.sh; "
  
  # build askapsoft if specified
  if [ "$DEPLOY_ASKAPSOFT" == true ]; then
    BUILD_COMMAND="${BUILD_COMMAND} rbuild -n -V --release-name ASKAPsoft-cpapps-${TAG} --stage-dir ASKAPsoft-cpapps-${TAG} -t release Code/Systems/cpapps; "
  fi

  # build pipelines if specified
  if [ "$DEPLOY_ASKAP_PIPLINE" == true ]; then
    BUILD_COMMAND="${BUILD_COMMAND} rbuild -n -V --release-name ASKAPsoft-pipelines-${TAG} --stage-dir ASKAPsoft-pipelines-${TAG} -t release Code/Systems/pipelines; "
  fi

  # build askapdoc if specified
  if [ "$DEPLOY_ASKAP_USER_DOC" == true ]; then
  
    # Update version numbers on documentation conf.py file by replacing the default snapshot svn revision number (version = 'r' ...)
    # with the release number. Only do this if it hasn't been done already (someone may have done this manually in svn before).
    if [ $(grep -q "^version = 'r'.*" Code/Doc/user/current/doc/conf.py) == 0 ]; then
      sed -e -i "s/^version = 'r'/#version = 'r'/" -e "s/^release = version/#release = version/" -e "s/^#version = '0.1'/version = '${VERSION}'/" -e "s/^#release = '0.1-draft'/release = '${VERSION}'/" Code/Doc/user/current/doc/conf.py
      svn ci -m "Updated version numbers for " Code/Doc/user/current/doc/conf.py
    fi
    
    BUILD_COMMAND="${BUILD_COMMAND} rbuild -n -t doc Code/Doc/user/current; "
  fi
  
  BUILD_COMMAND="${BUILD_COMMAND} touch ${BUILD_DIR}/build-askapsoft-done; "
  
  (eval $BUILD_COMMAND) &

}

# Build the ASKAP services including the CP Manager. This will need to be built on ingest cluster.
# However a checkout will not be necessary as it can be done from this environment (i.e. /group).
# q. Can this be done remotely from here?
buildServices() {
  log "Building the ASKAP services\n"

  cd ${WORKING_DIR}/${TAG}

  # Check it out of it's not there
  if [ ! -e askapingest-${TAG} ]; then
    svn export -q ${ASKAPSOFT_URL} askapingest-${TAG}
  fi

  # This needs to be done on the ingest nodes
  # build ingest if specified
  local BUILD_COMMAND="(cd ${WORKING_DIR}/${TAG}/askapingest-${TAG};
                        python2.7 bootstrap.py -n;
                        source initaskap.sh; 
                        rbuild -n -V --release-name ASKAPsoft-cpsvcs-${TAG} --stage-dir ASKAPsoft-cpsvcs-${TAG} -t release Code/Systems/cpsvcs;
                        touch ${BUILD_DIR}/build-cpsvcs-done) "
                       
  # run this on ingest
  ssh -v -t askapops@${INGEST_NODE} "${BUILD_COMMAND}"
}

# Build the CLI tools. A checkout of the TOS trunk is required for this.
# This will extend the time taken to run the script as it has to checkout and initialise and build a 
# separate environment.
buildCli() {
  log "Building the CLI tools\n"

  cd ${WORKING_DIR}/${TAG}

  # Create a build command which will run initialisation and all required builds in the background.
  local BUILD_COMMAND="python2.7 bootstrap.py -n;
                       source initaskap.sh; 
                       rbuild -n -V --release-name ASKAP-cli-${TAG} --stage-dir ASKAP-cli-${TAG} -t release Code/Components/UI/cli/current; 
                       touch ${BUILD_DIR}/build-cli-done"
                       
  svn export -q ${TOS_URL} askaptos-trunk
  cd askaptos-trunk
    
  (eval $BUILD_COMMAND) &

}

# Wait for the artefacts to be built and then stage the build
stage() {
  cd ${BUILD_DIR}

  # Wait for builds to finish
  # true if the build-xxxx-done files are present for the required builds (including the services one running on ingest)
  local ALL_DONE=false
  local WAIT_TIME=60
  local declare -a SEMAPHORES
  local IDX=0
  if [ "$DEPLOY_ASKAPSOFT" == true || "$DEPLOY_ASKAP_PIPLINE" == true || "$DEPLOY_ASKAP_USER_DOC" == true ]; then
    SEMAPHORES[$IDX]="build-askapsoft-done"
    IDX=IDX+1
  fi
  if [ "$DEPLOY_ASKAP_CLI" == true ]; then
    SEMAPHORES[$IDX]="build-cli-done"
    IDX=IDX+1
  fi
  if [ "$DEPLOY_ASKAP_SERVICES" == true ]; then
    SEMAPHORES[$IDX]="build-cpsvcs-done"
    IDX=IDX+1
  fi
  
  until [ $ALL_DONE == true ]; do
  
    # check all the semaphores
    for FLAG in ${SEMAPHORES[@]}; do
      if [ -e $FLAG ]; then
        ALL_DONE=true
      else
        ALL_DONE=false
      fi
    done
    
    # if any were not done then wait again
    if [ $ALL_DONE == false ]; then
      sleep $WAIT_TIME
    fi
  done

  # stage askapsoft if specified
  if [ "$DEPLOY_ASKAPSOFT" == true ]; then
    tar xf askapsoft-${TAG}/ASKAPsoft-cpapps-${TAG}.tgz
    cd ASKAPsoft-cpapps-${TAG}; rm -rf build-1 config docs ICE_LICENSE include initenv.py local man share slice ssl; cd ..
    
    # fix up permissions
    chgrp -R askap ASKAPsoft-cpapps-${TAG}
    find ASKAPsoft-cpapps-${TAG} -type f | xargs chmod oug+r
    find ASKAPsoft-cpapps-${TAG} -type d | xargs chmod oug+rx
    
    # stage
    mv ASKAPsoft-cpapps-${TAG} ${BUILD_DIR}
    sed -e "s/\${_VERSION}.*/${VERSION}/" askapsoft-${TAG}/Tools/Dev/deploy/askapsoft.module-template > ${BUILD_DIR}/askapsoft.module
    
  fi

  # stage pipelines if specified
  if [ "$DEPLOY_ASKAP_PIPLINE" == true ]; then
    tar xf ${askapsoft-${TAG}/ASKAPsoft-pipelines-${TAG}.tgz
    
    # fix up permissions
    chgrp -R askap ASKAPsoft-pipelines-${TAG}
    find ASKAPsoft-pipelines-${TAG} -type f | xargs chmod oug+r
    find ASKAPsoft-pipelines-${TAG} -type d | xargs chmod oug+rx
    
    # stage
    mv ASKAPsoft-pipelines-${TAG} ${BUILD_DIR}
    sed -e "s/\${_VERSION}.*/${VERSION}/" askapsoft-${TAG}/Tools/Dev/deploy/askappipeline.module-template > ${BUILD_DIR}/askappipeline.module

  fi

  # stage askapcli if specified
  if [ "$DEPLOY_ASKAP_CLI" == true ]; then
    tar xf askaptos-trunk/ASKAP-cli-${TAG}

    # fix up permissions
    chgrp -R askap ASKAP-cli-${TAG}
    find ASKAP-cli-${TAG} -type f | xargs chmod oug+r
    find ASKAP-cli-${TAG} -type d | xargs chmod oug+rx
    
    mv ASKAP-cli-${TAG} ${BUILD_DIR}
    sed -e "s/\${_VERSION}.*/${VERSION}/" askapsoft-${TAG}/Tools/Dev/deploy/askapcli.module-template > ${BUILD_DIR}/askapcli.module
  fi

  # stage askapdoc if specified
  if [ "$DEPLOY_ASKAP_USER_DOC" == true ]; then
    mv askapsoft-${TAG}/Code/Doc/user/current/doc/_build/html ${BUILD_DIR}
  fi
  
  # stage askapservices if specified
  if [ "$DEPLOY_ASKAP_SERVICES" == true ]; then
    tar xf askapingest-${TAG}/ASKAPsoft-cpapps-${TAG}.tgz
    cd ASKAPsoft-cpapps-${TAG}; rm -rf build-1 config docs ICE_LICENSE include initenv.py local man share slice ssl; cd ..
    
    # fix up permissions
    chgrp -R askap ASKAPsoft-cpapps-${TAG}
    find ASKAPsoft-cpapps-${TAG} -type f | xargs chmod oug+r
    find ASKAPsoft-cpapps-${TAG} -type d | xargs chmod oug+rx
    
    # stage
    mv ASKAPsoft-cpapps-${TAG} ${BUILD_DIR}
    sed -e "s/\${_VERSION}.*/${VERSION}/" askapingest-${TAG}/Tools/Dev/deploy/askapservices.module-template > ${BUILD_DIR}/askapservices.module
  fi
}

# Deploy the built modules; some might not have been built, but the ones that have are in the staging dir (build).
# This should not be called if there are any errors in the build process.
deploy() {
  log "Deploying the ASKAP modules to ${DEPLOY_DIR}\n"

  # Deploy the askap apps, if they were built, along with module configuration file
  if [ -e ${BUILD_DIR}/ASKAPsoft-cpapps-${TAG} ]; then
    mv ${BUILD_DIR}/ASKAPsoft-cpapps-${TAG} ${DEPLOY_DIR}/askapsoft/${TAG}
    mv ${BUILD_DIR}/askapsoft.module ${MODULES_DIR}/askapsoft/${VERSION}
    if [ "$MAKE_DEFAULT" == true ]; then
      # change the default referenced version
      sed -i -e "s/^set ModulesVersion.*/set ModulesVersion \"${VERSION}\"/" ${MODULES_DIR}/askapsoft/.version
    fi
  fi
  
  # Deploy the askap pipeline scripts, if they were built, along with module configuration file
  if [ -e ${BUILD_DIR}/ASKAPsoft-pipelines-${TAG} ]; then
    mv ${BUILD_DIR}/ASKAPsoft-pipelines-${TAG} ${DEPLOY_DIR}/askappipeline/${TAG}
    mv ${BUILD_DIR}/askappipeline.module ${MODULES_DIR}/askappipeline/${VERSION}
    if [ "$MAKE_DEFAULT" == true ]; then
      # change the default referenced version
      sed -i -e "s/^set ModulesVersion.*/set ModulesVersion \"${VERSION}\"/" ${MODULES_DIR}/askappipeline/.version
    fi
  fi
  
  # Deploy the askap cli, if it was built, module configuration file is always 'current'
  if [ -e ${BUILD_DIR}/ASKAP-cli-${TAG} ]; then
    mv ${BUILD_DIR}/ASKAP-cli-${TAG} ${DEPLOY_DIR}/askapcli/${TAG}
    rm ${DEPLOY_DIR}/askapcli/current
    ln -s ${DEPLOY_DIR}/askapcli/${TAG} ${DEPLOY_DIR}/askapcli/current
  fi
}

# Cleanup. Only call this once we are definitely running, so after command line processing and lock check.
cleanup () {
  if [ -e /tmp/ASKAPDEPLOY.lock ]; then
    rm /tmp/ASKAPDEPLOY.lock
    #rm -rf ${WORKING_DIR}/${TAG}
  fi
  exit $1
}

showHelp() {
  printf "$USAGE \n"
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
  printf "  export WORKING_DIR='/group/astronomy856/askapops/workspace/askapsdp/'\n"
  printf "  export OVERWRITE_EXISTING=false\n"
  printf "  export DEPLOY_DIR='/group/askap/'\n"
  printf "  export MODULES_DIR='/group/askap/modulefiles'\n"
  printf "  export DEPLOY_ASKAPSOFT=true\n"
  printf "  export DEPLOY_ASKAP_USER_DOC=true\n"
  printf "  export DEPLOY_ASKAP_PIPLINE=true\n"
  printf "  export DEPLOY_ASKAP_SERVICES=true\n"
  printf "  export DEPLOY_ASKAP_CLI=true\n"
  printf "  export MAKE_MODULES_DEFAULT=true\n"
  printf "  export REPO_URL='https://svn.atnf.csiro.au/'\n"
  printf "  export LOG_FILE='./${NOW}-ASKAPSDP-deploy.log'\n"
}

# Logger for logging messages
log() {
  printf "`date "+%Y%m%d %H:%M:%S"` - $@" >> $LOG_FILE
  if [ "$VERBOSE" == true ]; then
    printf "$@" 
  fi
}
logerr() {
  # Log to file and stderr
  log "ERROR: $@"
  printf "ERROR: $@" 1>&2
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
#
processCmdLine() {

  # For option processing
  OPTIND=1

  # Process our options
  while getopts "h?vlt:c:" opt "$@"; do
      case "$opt" in
      
      h|\?)
          showHelp
          exit $NO_ERROR
          ;;
      v)
          VERBOSE=true
          ;;
      t)
          if [[ ${OPTARG} =~ $TAG_FORMAT ]]; then
            TAG=$OPTARG
          else
            printf "The tag %s does not match semantic versioning string format [CP-0.12.3]\n" ${OPTARG}
            exit $ERROR_BAD_SVN_TAG;
          fi
          ;;
      l)
          findLatestTag
          ;;
      c)
          if  [ -e $OPTARG ]; then
            CFG_FILE=$OPTARG
          else
            printf "The given config file %s does not exist\n" ${OPTARG}
            exit $ERROR_MISSING_CFG_FILE;
          fi
          ;;
      esac
  done

  shift $((OPTIND-1))

  [ "$1" = "--" ] && shift
  
  if [ "$TAG" == "" ]; then 
    printf "No TAG has been specified\n"
    printf "${USAGE}\n"
    exit $ERROR_NO_TAG;
  fi

}

# find the latest tag; turn on extended patterns; match semantic versions, i.e. MM.mm.pp, e.g. 0.12.3
findLatestTag() {
  shopt -s extglob

  log "Using the latest tag from subversion for ASKAPsoft\n"

  local LATEST_TAG=`svn ls -v https://svn.atnf.csiro.au/askapsdp/tags | sort | tail -n 1 | awk -F " " '{print $NF}'`

  if [[ ${LATEST_TAG} =~ $TAG_FORMAT ]]; then
    log "The latest tag is %s\n" ${LATEST_TAG}
    TAG=${LATEST_TAG}
  else
    logerr "The tag %s does not match semantic versioning string format [CP-0.12.3]\n" ${LATEST_TAG}
    cleanup $ERROR_BAD_SVN_TAG;
  fi
}

# Print the environment to the log file
logENV () {
  log "OVERWRITE_EXISTING = %s\n" $([ "$OVERWRITE_EXISTING" == true ] && echo "true" || echo "false")
  log "MAKE_MODULES_DEFAULT = %s\n" $([ "$MAKE_MODULES_DEFAULT" == true ] && echo "true" || echo "false")
  log "MODULES_DIR = %s\n" $MODULES_DIR
  log "WORKING_DIR = %s\n" $WORKING_DIR
  log "DEPLOY_DIR = %s\n" $DEPLOY_DIR
  log "DEPLOY_ASKAPSOFT = %s\n" $([ "$DEPLOY_ASKAPSOFT" == true ] && echo "true" || echo "false")
  log "DEPLOY_ASKAP_PIPLINE = %s\n" $([ "$DEPLOY_ASKAP_PIPLINE" == true ] && echo "true" || echo "false")
  log "DEPLOY_ASKAP_SERVICES = %s\n" $([ "$DEPLOY_ASKAP_SERVICES" == true ] && echo "true" || echo "false")
  log "DEPLOY_ASKAP_CLI = %s\n" $([ "$DEPLOY_ASKAP_CLI" == true ] && echo "true" || echo "false")
  log "ASKAPSOFT_URL = %s\n" $ASKAPSOFT_URL
  log "TOS_URL = %s\n" $TOS_URL
}

# Check what is going to be overwritten. Combined with existing cfg after this we will know what we have
# to do (if anything).
doOverwriteChecks() {
  local BAIL=false

  if [ -e ${MODULES_DIR} ]; then

    # Check for existing askapsoft module
    if [ "$OVERWRITE_EXISTING" == false ] && [ "$DEPLOY_ASKAPSOFT" == true ] && [ -e ${MODULES_DIR}/askapsoft/${TAG} ]; then
      logerr "Cannot overwrite existing module %s\n" ${MODULES_DIR}/askapsoft/${TAG}
      BAIL=true
    fi
    if [ "$OVERWRITE_EXISTING" == true ] && [ "$DEPLOY_ASKAPSOFT" == true ] && [ -e ${MODULES_DIR}/askapsoft/${TAG} ]; then
      log "OVERWRITE existing module %s\n" ${MODULES_DIR}/askapsoft/${TAG}
    fi

    # Check for existing askappipeline module
    if [ "$OVERWRITE_EXISTING" == false ] && [ "$DEPLOY_ASKAP_PIPELINE" == true ] && [ -e ${MODULES_DIR}/askappipeline/${TAG} ]; then
      logerr "Cannot overwrite existing module %s\n" ${MODULES_DIR}/askappipeline/${TAG}
      BAIL=true
    fi
    if [ "$OVERWRITE_EXISTING" == true ] && [ "$DEPLOY_ASKAP_PIPELINE" == true ] && [ -e ${MODULES_DIR}/askappipeline/${TAG} ]; then
      log "OVERWRITE existing module %s\n" ${MODULES_DIR}/askappipeline/${TAG}
      BAIL=true
    fi

    # Check for existing askapservices module
    if [ "$OVERWRITE_EXISTING" == false ] && [ "$DEPLOY_ASKAP_SERVICES" == true ] && [ -e ${MODULES_DIR}/askapservices/${TAG} ]; then
      logerr "Cannot overwrite existing module %s\n" ${MODULES_DIR}/askapservices/${TAG}
      BAIL=true
    fi
    if [ "$OVERWRITE_EXISTING" == true ] && [ "$DEPLOY_ASKAP_SERVICES" == true ] && [ -e ${MODULES_DIR}/askapservices/${TAG} ]; then
      log "OVERWRITE existing module %s\n" ${MODULES_DIR}/askapservices/${TAG}
    fi

    # Check for existing askapcli module
    if [ "$OVERWRITE_EXISTING" == false ] && [ "$DEPLOY_ASKAP_CLI" == true ] && [ -e ${MODULES_DIR}/askapcli/${CLI_REV} ]; then
      logerr "Cannot overwrite existing module %s\n" ${MODULES_DIR}/askapcli/${CLI_REV}
      BAIL=true
    fi
    if [ "$OVERWRITE_EXISTING" == true ] && [ "$DEPLOY_ASKAP_CLI" == true ] && [ -e ${MODULES_DIR}/askapcli/${CLI_REV} ]; then
      log "OVERWRITE existing module %s\n" ${MODULES_DIR}/askapcli/${CLI_REV}
    fi

    # exit if we have any overwrite check fails
    if [ "$BAIL" == true ]; then
      cleanup $ERROR_OVERWRITE_CHECK_FAILED
    fi
  else
    logerr "The modules directory does not exist, %s" ${MODULES_DIR} 
    cleanup $ERROR_MISSING_ENV;
  fi
}

# We only want this to run as the asksapops user
doUserCheck () {
  if [[ $USER != "bas091" ]]; then 
    logerr "This script must be run as askapops!" 
    exit $ERROR_NOT_ASKAPOPS;
  fi 
}

# Handling a semaphore file to prevent running build twice
doLockCheck () {
  if [ -e /tmp/ASKAPDEPLOY.lock ]; then
    printf "ERROR: This script is already running - aborting!\n"  1>&2
    exit $ERROR_ALREADY_RUNNING;
  fi
  touch /tmp/ASKAPDEPLOY.lock
}

# We need to know if we are running on galaxy or ingest cluster
detectHost () {
  shopt -s extglob

  if [[ ${HOSTNAME} =~ "galaxy-??" ]]; then
    GALAXY_CLUSTER=true;
  else
     printf "ERROR: Cannot run on this machine, must be galaxy or ingest cluster!\n"
     exit $ERROR_WRONG_CLUSTER;
  fi
}

main "$@"
