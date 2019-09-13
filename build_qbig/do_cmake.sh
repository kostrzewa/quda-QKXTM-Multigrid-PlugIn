#!/bin/sh

export PATH=/opt/gcc-5.4.0/bin:${PATH}

srcdir=~/code/quda-QKXTM-Multigrid-PlugIn/


CUDA_BIN_PATH=/opt/cuda-8.0 \
cmake  \
       -DMPI_HOME=/opt/openmpi-2.0.2a1-with-pmi \
       -DQUDA_ADDITIONAL_NVCC_FLAGS=-I/opt/openmpi-2.0.2a1-with-pmi/include \
       -DCMAKE_C_COMPILER=/opt/openmpi-2.0.2a1/bin/mpicc \
       -DCMAKE_CXX_COMPILER=/opt/openmpi-2.0.2a1/bin/mpicxx \
       -DCMAKE_CUDA_COMPILER=/opt/cuda-8.0/bin/nvcc \
       -DCUDA_TOOLKIT_ROOT_DIR=/opt/cuda-8.0 \
       -DCUDA_TOOLKIT_TARGET_DIR=/opt/cuda-8.0 \
       -DCMAKE_BUILD_TYPE=RELEASE \
       -DCMAKE_EXE_LINKER_FLAGS="-lmpi_mpifh -lgfortran" \
       -DQKXTM_HDF5_LIB="/usr/lib/x86_64-linux-gnu/hdf5/openmpi/libhdf5.so" \
       -DQKXTM_LIME_LIB="/qbigwork2/bartek/libs/lime/sandybridge/lib/liblime.a" \
       -DQKXTM_QUDA_HOME="/qbigwork2/pittler/code/quda-for_plugin/" \
       -DQKXTM_LIMEHOME="/qbigwork2/bartek/libs/lime/sandybridge" \
       -DQKXTM_HDF5HOME="/usr/lib/x86_64-linux-gnu/hdf5/openmpi/" \
       -DQKXTM_2FLAVMG=ON \
       -DQKXTM_ARPACK=ON \
       -DQKXTM_ARPACK_LIB=/usr/lib/libarpack.a \
       -DQKXTM_MKL=OFF \
       -DQUDA_ARPACK=OFF \
       -DQKXTM_OPENBLAS=ON \
       -DQKXTM_OPENBLASHOME=/usr \
       -DQKXTM_OPENBLAS_LIB=/usr/lib/libopenblas.a \
       -DQKXTM_OPENBLAS_LIBDIR=/usr/lib \
       -DQKXTM_OPENBLAS_GOMP=/opt/gcc-5.4.0/lib64/libgomp.so \
       -DQKXTM_PARPACK_LIB=/usr/lib/libparpack.a \
       -DQUDA_MPI=ON \
       -DQUDA_MULTIGRID=ON \
       -DQUDA_BLOCKSOLVER=OFF \
       -DQUDA_CONTRACT=ON \
       -DQUDA_DEFLATEDSOLVER=OFF \
       -DQUDA_DIRAC_CLOVER=ON \
       -DQUDA_DIRAC_DOMAIN_WALL=OFF \
       -DQUDA_DIRAC_NDEG_TWISTED_MASS=OFF \
       -DQUDA_DIRAC_STAGGERED=OFF \
       -DQUDA_DIRAC_TWISTED_CLOVER=ON \
       -DQUDA_DIRAC_TWISTED_MASS=ON \
       -DQUDA_DIRAC_WILSON=ON \
       -DQUDA_DOWNLOAD_EIGEN=ON \
       -DQUDA_DYNAMIC_CLOVER=OFF \
       -DQUDA_FORCE_ASQTAD=OFF \
       -DQUDA_FORCE_GAUGE=OFF \
       -DQUDA_FORCE_HISQ=OFF \
       -DQUDA_GAUGE_ALG=OFF \
       -DQUDA_GAUGE_TOOLS=OFF \
       -DQUDA_GITDIR="${srcdir}/.git" \
       -DQUDA_GPU_ARCH=sm_35 \
       -DQUDA_INTERFACE_BQCD=OFF \
       -DQUDA_INTERFACE_CPS=OFF \
       -DQUDA_INTERFACE_MILC=ON \
       -DQUDA_INTERFACE_QDP=ON \
       -DQUDA_INTERFACE_QDPJIT=OFF \
       -DQUDA_INTERFACE_TIFR=OFF \
       -DQUDA_LINK_ASQTAD=OFF \
       -DQUDA_LINK_HISQ=OFF \
       -DQUDA_MAGMA=OFF \
       -DQUDA_POSIX_THREADS=OFF \
       -DQUDA_QDPJIT=OFF \
       -DQUDA_QIO=OFF \
       -DQUDA_QMP=OFF \
${srcdir}
