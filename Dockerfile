FROM ubuntu:22.04

RUN apt-get update && apt-get -y upgrade
RUN apt-get install --assume-yes --no-install-recommends ca-certificates build-essential cmake git
RUN apt-get install --assume-yes --no-install-recommends liburing-dev libssl-dev libtbb-dev libatlas-base-dev libgtest-dev libxerces-c-dev libgsl-dev
RUN apt-get install --assume-yes --no-install-recommends libboost-regex-dev libsqlite3-dev
RUN apt-get install --assume-yes --no-install-recommends texlive-latex-extra texlive-fonts-recommended texlive-latex-recommended texlive-fonts-extra
RUN apt-get install --assume-yes --no-install-recommends autoconf automake libtool libboost-all-dev libssl-dev

# Build sqlplot-tools
RUN git clone https://github.com/lorenzhs/sqlplot-tools.git /opt/sqlplot-tools
RUN mkdir /opt/sqlplot-tools/build
WORKDIR /opt/sqlplot-tools/build
RUN git checkout feature/attribute_mark
RUN cmake -DCMAKE_BUILD_TYPE=Release -DWITH_POSTGRESQL=OFF -DWITH_MYSQL=OFF ..
RUN cmake --build . -j 8

COPY . /opt/pachash
WORKDIR /opt/pachash/patches
RUN ./apply-patches.sh

# Build PaCHash basics
RUN mkdir /opt/pachash/external/pachash/build
WORKDIR /opt/pachash/external/pachash/build
RUN cmake -DCMAKE_BUILD_TYPE=Release ..
RUN cmake --build . -j 8

# Build PaCHash competitor experiments
WORKDIR /opt/pachash/external/silt
RUN autoupdate
RUN mkdir /opt/pachash/build
WORKDIR /opt/pachash/build
RUN cmake -DCMAKE_BUILD_TYPE=Release ..
RUN cmake --build . -j 8

# Actual benchmark
CMD bash /opt/dockerVolume/figure-4.sh
