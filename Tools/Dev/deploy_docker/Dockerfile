FROM ubuntu
WORKDIR /apps
COPY . /apps
RUN tar -xvf /apps/askap_apps.tar.gz 
RUN mkdir /usr/local/geodetic
RUN cd /usr/local/geodetic ; tar -xvf /apps/WSRT_Measures.ztar 
RUN apt-get install -y wget subversion build-essential gfortran python-dev flex bison openjdk-8-jdk libfreetype6-dev libpng-dev autoconf
RUN apt-get update && apt-get install -y --no-install-recommends \
    && apt-get install -y --no-install-recommends libboost-system1.65.1 \
    && apt-get install -y --no-install-recommends libboost-filesystem1.65.1 \
    && apt-get install -y --no-install-recommends libboost-regex1.65.1 \
    && apt-get install -y --no-install-recommends libboost-program-options1.65.1 \
    && apt-get install -y --no-install-recommends libboost-thread1.65.1 \
    && apt-get install -y --no-install-recommends libcfitsio5 \
    && apt-get install -y --no-install-recommends libgfortran5 \
    && apt-get install -y --no-install-recommends libgfortran4 \
    && apt-get install -y --no-install-recommends libzeroc-ice3.7 \
    && apt-get clean
RUN mkdir /build
WORKDIR build
RUN wget http://www.mpich.org/static/downloads/3.1.4/mpich-3.1.4.tar.gz
RUN tar xvf mpich-3.1.4.tar.gz
WORKDIR mpich-3.1.4
RUN bash ./configure
RUN make VERBOSE=1
RUN make install
RUN apt-get update
WORKDIR /
RUN rm -rf /build
RUN ldconfig

ENV PATH "$PATH:/apps/askap_apps"

