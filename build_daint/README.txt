Building on Piz Daint
 - A. Gasbarro, Feb 12 2019

1.) One will need to link to sources for 
     a.) hdf5
     b.) lime
     c.) gsl
     d.) arpack (optional)
   One also needs the source for QUDA, but this does not need to be built ahead of time.  This get compiled during the QKXTM build process.
   note: QKXTM requires an old version of QUDA 
   The built sources can be found in piz daint at 
     /users/gasbarro/adgBuildK/dependencies/
   and can be copied directly to wherever you are doing your build
2.) Edit cmakebuild.sh to point to the location of the dependences
3.) Make a build folder inside of QKXTM
     /path/to/quda-QKXTM-Multigrid-PlugIn/build
4.) Copy "cmakebuild.sh" to your build folder
5.) source PizDaint_load_modules_v2.sh
6.) From your build folder, run  "./cmakebuild.sh"
7.) Hopefully the configuration step went through and you now have a make file
8.) Build the Calc_Loops executable with "make Calc_Loops"  (note: trying to make everything with "make" fails for me).
