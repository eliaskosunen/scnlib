FROM gcr.io/oss-fuzz-base/base-builder

RUN echo "CXX=$CXX"
RUN echo "CXXFLAGS=$CXXFLAGS"

RUN apt update && \
    apt install -y --no-install-recommends \
        ninja-build cmake && \
    apt-get clean && \
    rm -rf /var/lib/apt/lists/*

# Using re2 instead of std::regex to not time out due to too complex regular expressions
# re2 needs abseil

RUN git clone --depth 1 --branch 20240722.0 https://github.com/abseil/abseil-cpp $SRC/abseil-cpp && \
    cd $SRC/abseil-cpp && \
    mkdir build && cd build && \
    cmake -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_POSITION_INDEPENDENT_CODE=ON .. && \
    cmake --build . --target install && \
    cd $SRC && rm -rf $SRC/abseil-cpp

RUN git clone --depth 1 --branch 2024-07-02 https://github.com/google/re2 $SRC/re2 && \
    cd $SRC/re2 && \
    mkdir build && cd build && \
    cmake -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_POSITION_INDEPENDENT_CODE=ON .. && \
    cmake --build . --target install && \
    cd $SRC && rm -rf $SRC/re2

COPY . $SRC/scnlib
WORKDIR $SRC/scnlib
COPY ./.clusterfuzzlite/build.sh $SRC/
