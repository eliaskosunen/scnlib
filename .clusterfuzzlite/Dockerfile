FROM gcr.io/oss-fuzz-base/base-builder

RUN echo "CXX=$CXX"
RUN echo "CXXFLAGS=$CXXFLAGS"

RUN apt update && \
    apt install -y --no-install-recommends \
        ninja-build cmake && \
    apt-get clean && \
    rm -rf /var/lib/apt/lists/*

COPY . $SRC/scnlib
WORKDIR $SRC/scnlib
COPY ./.clusterfuzzlite/build.sh $SRC/
