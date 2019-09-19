pipeline {
  agent {
    docker {
      image 'sord/yanda:latest'
    }

  }
  stages {
    stage('Building lofar-common') {
      steps {
        dir(path: '.') {
          sh '''if [ -d  ]; then
echo "lofar-common directory already exists"
rm -rf lofar-common
fi
git clone https://bitbucket.csiro.au/scm/askapsdp/lofar-common.git
cd lofar-common
mkdir build
cd build
cmake -DCMAKE_INSTALL_PREFIX=${PREFIX} ../
make 
make install
'''
        }

      }
    }    
    stage('Building lofar-blob') {
      steps {
        dir(path: '.') {
          sh '''if [ -d lofar-blob ]; then
echo "lofar-blob directory already exists"
rm -rf lofar-blob
fi
git clone https://bitbucket.csiro.au/scm/askapsdp/lofar-blob.git
cd lofar-blob
mkdir build
cd build
cmake -DCMAKE_INSTALL_PREFIX=${PREFIX} ../
make 
make install
'''
        }

      }
    }
    stage('Building base-askap') {
      steps {
        dir(path: '.') {
          sh '''if [ -d base-askap ]; then
echo "base-askap directory already exists"
rm -rf base-askap
fi
git clone https://bitbucket.csiro.au/scm/askapsdp/base-askap.git
cd base-askap
mkdir build
cd build
cmake -DCMAKE_INSTALL_PREFIX=${PREFIX} ../
make 
make install
'''
        }

      }
    }
  }
  environment {
    WORKSPACE = pwd()
    PREFIX = "${WORKSPACE}/install"
  }
}

