# important: daint-gpu must be loaded to get access to many modules!

#module swap PrgEnv-cray/6.0.4 PrgEnv-gnu/6.0.4
#module load cray-hdf5

module load daint-gpu
module swap PrgEnv-cray PrgEnv-gnu

#module load GSL/2.4-CrayGNU-17.08
module load cray-hdf5-parallel/1.10.0
module load cray-libsci
module load CMake/3.6.2
#module load cudatoolkit/8.0.61_2.4.3-6.0.4.0_3.1__gb475d12
module load cudatoolkit/8.0.61_2.4.9-6.0.7.0_17.1__g899857c
module load Boost
module swap gcc/6.2.0 gcc/4.9.3
module load GSL/2.5-CrayGNU-18.08
