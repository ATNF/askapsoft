FROM ubuntu:latest
WORKDIR /apps
COPY . /apps
RUN tar -xvf /apps/askap_apps.tar.gz ; rm /apps/askap_apps.tar.gz
RUN apt-get update && apt-get install -y --no-install-recommends \
    && apt-get install -y --no-install-recommends apt-utils \
    && apt-get install -y --no-install-recommends libboost-system1.65.1 \
    && apt-get install -y --no-install-recommends libboost-filesystem1.65.1 \
    && apt-get install -y --no-install-recommends libboost-regex1.65.1 \
    && apt-get install -y --no-install-recommends libopenmpi2 \
    && apt-get install -y --no-install-recommends libboost-program-options1.65.1 \
    && apt-get install -y --no-install-recommends libboost-thread1.65.1 \
    && apt-get install -y --no-install-recommends libcfitsio5 \
    && apt-get install -y --no-install-recommends libgfortran5 \
    && apt-get install -y --no-install-recommends libgfortran4 \
    && apt-get install -y --no-install-recommends libzeroc-ice3.7 \
    && apt-get install -y --no-install-recommends openmpi-bin \
    && apt-get install -y --no-install-recommends rsh-client \
    && apt-get clean

ENV PATH "$PATH:/apps/askap_apps"

