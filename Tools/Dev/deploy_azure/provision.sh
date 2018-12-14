sudo   apt-get install -y bison
sudo   apt-get install -y boost
sudo   apt-get install -y bzip2
sudo   apt-get install -y cmake
sudo   apt-get install -y flex
sudo   apt-get install -y g++
sudo   apt-get install -y gfortran
sudo   apt-get install -y git-svn
sudo   apt-get install -y junit4
sudo   apt-get install -y libatlas-base-dev
sudo   apt-get install -y libblas-dev
sudo   apt-get install -y libblitz++
sudo   apt-get install -y libboost
sudo   apt-get install -y libboost-dev
sudo   apt-get install -y libboost1.65-all-dev
sudo   apt-get install -y libcfitsio-dev
sudo   apt-get install -y libcfitsio5
sudo   apt-get install -y libcppunit-dev
sudo   apt-get install -y libfreetype6-dev
sudo   apt-get install -y libhealpix-cxx-dev
sudo   apt-get install -y liblapack-dev
sudo   apt-get install -y liblog4cplus
sudo   apt-get install -y liblog4cplus-dev
sudo   apt-get install -y liblog4cxx-dev
sudo   apt-get install -y libncurses5
sudo   apt-get install -y libncurses5-dev
sudo   apt-get install -y libopenmpi-dev
sudo   apt-get install -y libpng12-dev
sudo   apt-get install -y libreadline
sudo   apt-get install -y libreadline-dev
sudo   apt-get install -y libreadline5
sudo   apt-get install -y libsqlite3
sudo   apt-get install -y libsqlite3-dev
sudo   apt-get install -y libzeroc-ice-dev
sudo   apt-get install -y libzeroc-ice-java
sudo   apt-get install -y libzeroc-ice3.7
sudo   apt-get install -y libzeroc-ice3.7-java
sudo   apt-get install -y libzeroc-icestorm3.7
sudo   apt-get install -y log4cplus
sudo   apt-get install -y make
sudo   apt-get install -y numpy-stl
sudo   apt-get install -y openjdk-8-jdk
sudo   apt-get install -y openmpi-bin
sudo   apt-get install -y patch
sudo   apt-get install -y pip
sudo   apt-get install -y python-APLpy
sudo   apt-get install -y python-astropy
sudo   apt-get install -y python-astropy-helpers
sudo   apt-get install -y python-configparser
sudo   apt-get install -y python-configparser   
sudo   apt-get install -y python-dev
sudo   apt-get install -y python-future
sudo   apt-get install -y python-future  
sudo   apt-get install -y python-matplotlib
sudo   apt-get install -y python-networkx
sudo   apt-get install -y python-pip
sudo   apt-get install -y python-scipy
sudo   apt-get install -y python-virtualenv  
sudo   apt-get install -y readline
sudo   apt-get install -y scons
sudo   apt-get install -y sqlite3
sudo   apt-get install -y sqlite3-dev
sudo   apt-get install -y ssh
sudo   apt-get install -y subversion
sudo   apt-get install -y virtualenv
sudo   apt-get install -y zeroc
sudo   apt-get install -y zeroc-ice-all-dev
sudo sed  -i 's#/usr/bin/python#/usr/bin/env python#g' `which scons`

if [[ -z "${CLASSPATH}" ]]; then

  echo "CLASSPATH=${CLASSPATH}:/usr/share/java/ice-3.7.1.jar:/usr/local/java/icestorm-3.7.1.jar:/usr/share/java/ice-compat-3.7.1.jar:/usr/share/java/icestorm-compat-3.7.1.jar" >> ${HOME}/.bashrc

else

  echo "CLASSPATH=/usr/share/java/ice-3.7.1.jar:/usr/local/java/icestorm-3.7.1.jar:/usr/share/java/ice-compat-3.7.1.jar:/usr/share/java/icestorm-compat-3.7.1.jar" >> ${HOME}/.bashrc

fi


