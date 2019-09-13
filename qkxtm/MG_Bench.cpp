#include <stdlib.h>
#include <stdio.h>
#include <time.h>
#include <math.h>
#include <string.h>

#include <util_quda.h>
#include <QKXTM_util.h>
#include <QKXTM_mapping_parity.h>
#include <dslash_util.h>
#include <blas_reference.h>
#include <wilson_dslash_reference.h>
#include <domain_wall_dslash_reference.h>
#include "misc.h"

#include "face_quda.h"

#if defined(QMP_COMMS)
#include <qmp.h>
#elif defined(MPI_COMMS)
#include <mpi.h>
#endif

#include <qio_field.h>

#define MAX(a,b) ((a)>(b)?(a):(b))

// In a typical application, quda.h is the only QUDA header required.
#include <quda.h>
#include <qudaQKXTM.h>

//========================================================================//
//====== P A R A M E T E R   S E T T I N G S   A N D   C H E C K S =======//
//========================================================================//

//-----------------//
// QUDA Parameters //
//-----------------//
extern QudaDslashType dslash_type;
extern int device;
extern int xdim;
extern int ydim;
extern int zdim;
extern int tdim;
extern int Lsdim;
extern int gridsize_from_cmdline[];
extern QudaReconstructType link_recon;
extern QudaPrecision prec;
extern QudaPrecision  prec_sloppy;
extern QudaPrecision  prec_precondition;
extern QudaPrecision prec_null;
extern QudaReconstructType link_recon_sloppy;
extern QudaReconstructType link_recon_precondition;
extern double mass;  // mass of Dirac operator
extern double kappa; // kappa of Dirac operator
extern double mu;
extern double anisotropy;
extern double tol; // tolerance for inverter
extern double tol_hq; // heavy-quark tolerance for inverter
extern char latfile[];
extern int Nsrc; // number of spinors to apply to simultaneously
extern int niter;
extern int gcrNkrylov; // number of inner iterations for GCR, or l for BiCGstab-l
extern int pipeline; // length of pipeline for fused operations in GCR or BiCGstab-l
extern int nvec[];
extern int mg_levels;

extern bool generate_nullspace;
extern bool generate_all_levels;
extern int nu_pre;
extern int nu_post;
extern int geo_block_size[QUDA_MAX_MG_LEVEL][QUDA_MAX_DIM];
extern double mu_factor[QUDA_MAX_MG_LEVEL];

extern QudaVerbosity mg_verbosity[QUDA_MAX_MG_LEVEL];

extern QudaInverterType setup_inv[QUDA_MAX_MG_LEVEL];
extern int num_setup_iter[QUDA_MAX_MG_LEVEL];
extern double setup_tol;
extern QudaSetupType setup_type;
extern bool pre_orthonormalize;
extern bool post_orthonormalize;
extern double omega;
extern QudaInverterType coarse_solver[QUDA_MAX_MG_LEVEL];
extern QudaInverterType smoother_type[QUDA_MAX_MG_LEVEL];
extern double coarse_solver_tol[QUDA_MAX_MG_LEVEL];
extern double smoother_tol[QUDA_MAX_MG_LEVEL];
extern int coarse_solver_maxiter[QUDA_MAX_MG_LEVEL];

extern QudaSchwarzType schwarz_type[QUDA_MAX_MG_LEVEL];
extern int schwarz_cycle[QUDA_MAX_MG_LEVEL];

extern QudaMatPCType matpc_type;
extern QudaSolveType solve_type;

extern char vec_infile[];
extern char vec_outfile[];

extern QudaTwistFlavorType twist_flavor;

extern void usage(char** );

extern double clover_coeff;
extern bool compute_clover;
extern QudaMassNormalization normalization; // mass normalization of Dirac operators
extern bool verify_results;

//------------------//
// QKXTM Parameters //
//------------------//
extern char latfile_smeared[];
extern char verbosity_level[];
extern int traj;
extern bool isEven;

extern int src[];
extern int Ntsink;
extern char pathList_tsink[];
extern int Q_sq;
extern int nsmearAPE;
extern int nsmearGauss;
extern double alphaAPE;
extern double alphaGauss;
extern char twop_filename[];
extern char threep_filename[];

extern char prop_path[];
extern double csw;

extern int numSourcePositions;
extern char pathListSourcePositions[];
extern char pathListRun3pt[];
extern char run3pt[];
extern char *corr_file_format;
extern char check_file_exist[];

extern int Nproj;
extern char proj_list_file[];

extern char *corr_write_space;

namespace quda {
  extern void setTransferGPU(bool);
}

void
display_test_info()
{
  printfQuda("running the following test:\n");
    
  printfQuda("prec    sloppy_prec    link_recon  sloppy_link_recon S_dimension T_dimension Ls_dimension\n");
  printfQuda("%s   %s             %s            %s            %d/%d/%d          %d         %d\n",
	     get_prec_str(prec),get_prec_str(prec_sloppy),
	     get_recon_str(link_recon), 
	     get_recon_str(link_recon_sloppy),  xdim, ydim, zdim, tdim, Lsdim);     

  printfQuda("MG parameters\n");
  printfQuda(" - number of levels %d\n", mg_levels);
  for (int i=0; i<mg_levels-1; i++) printfQuda(" - level %d number of null-space vectors %d\n", i+1, nvec[i]);
  printfQuda(" - number of pre-smoother applications %d\n", nu_pre);
  printfQuda(" - number of post-smoother applications %d\n", nu_post);

  printfQuda("Outer solver paramers\n");
  printfQuda(" - pipeline = %d\n", pipeline);

  printfQuda("Grid partition info:     X  Y  Z  T\n"); 
  printfQuda("                         %d  %d  %d  %d\n", 
	     dimPartitioned(0),
	     dimPartitioned(1),
	     dimPartitioned(2),
	     dimPartitioned(3)); 
  return ;  
}

QudaPrecision &cpu_prec = prec;
QudaPrecision &cuda_prec = prec;
QudaPrecision &cuda_prec_sloppy = prec_sloppy;
QudaPrecision &cuda_prec_precondition = prec_precondition;

void setGaugeParam(QudaGaugeParam &gauge_param) {
  gauge_param.X[0] = xdim;
  gauge_param.X[1] = ydim;
  gauge_param.X[2] = zdim;
  gauge_param.X[3] = tdim;

  gauge_param.anisotropy = anisotropy;
  gauge_param.type = QUDA_WILSON_LINKS;
  gauge_param.gauge_order = QUDA_QDP_GAUGE_ORDER;
  gauge_param.t_boundary = QUDA_ANTI_PERIODIC_T;
  
  gauge_param.cpu_prec = cpu_prec;

  gauge_param.cuda_prec = cuda_prec;
  gauge_param.reconstruct = link_recon;

  gauge_param.cuda_prec_sloppy = cuda_prec_sloppy;
  gauge_param.reconstruct_sloppy = link_recon_sloppy;

  gauge_param.cuda_prec_precondition = cuda_prec_precondition;
  gauge_param.reconstruct_precondition = link_recon_precondition;

  gauge_param.gauge_fix = QUDA_GAUGE_FIXED_NO;

  gauge_param.ga_pad = 0;
  // For multi-GPU, ga_pad must be large enough to store a time-slice
#ifdef MULTI_GPU
  int x_face_size = gauge_param.X[1]*gauge_param.X[2]*gauge_param.X[3]/2;
  int y_face_size = gauge_param.X[0]*gauge_param.X[2]*gauge_param.X[3]/2;
  int z_face_size = gauge_param.X[0]*gauge_param.X[1]*gauge_param.X[3]/2;
  int t_face_size = gauge_param.X[0]*gauge_param.X[1]*gauge_param.X[2]/2;
  int pad_size =MAX(x_face_size, y_face_size);
  pad_size = MAX(pad_size, z_face_size);
  pad_size = MAX(pad_size, t_face_size);
  gauge_param.ga_pad = pad_size;    
#endif
}

void setMultigridParam(QudaMultigridParam &mg_param) {
  QudaInvertParam &inv_param = *mg_param.invert_param;

  if (kappa == -1.0) {
    inv_param.mass = mass;
    inv_param.kappa = 1.0 / (2.0 * (1 + 3/anisotropy + mass));
  } else {
    inv_param.kappa = kappa;
    inv_param.mass = 0.5/kappa - (1.0 + 3.0/anisotropy);
  }

  printfQuda("Kappa = %.8f Mass = %.8f\n", inv_param.kappa, inv_param.mass);

  inv_param.Ls = 1;

  inv_param.sp_pad = 0;
  inv_param.cl_pad = 0;

  inv_param.cpu_prec = cpu_prec;
  inv_param.cuda_prec = cuda_prec;
  inv_param.cuda_prec_sloppy = cuda_prec_sloppy;
  inv_param.cuda_prec_precondition = cuda_prec_precondition;
  inv_param.preserve_source = QUDA_PRESERVE_SOURCE_NO;
  inv_param.gamma_basis = QUDA_DEGRAND_ROSSI_GAMMA_BASIS;
  inv_param.dirac_order = QUDA_DIRAC_ORDER;

  if (dslash_type == QUDA_CLOVER_WILSON_DSLASH || 
      dslash_type == QUDA_TWISTED_CLOVER_DSLASH) {
    inv_param.clover_cpu_prec = cpu_prec;
    inv_param.clover_cuda_prec = cuda_prec;
    inv_param.clover_cuda_prec_sloppy = cuda_prec_sloppy;
    inv_param.clover_cuda_prec_precondition = cuda_prec_precondition;
    inv_param.clover_order = QUDA_PACKED_CLOVER_ORDER;
    inv_param.clover_coeff = csw*inv_param.kappa;
  }

  inv_param.input_location = QUDA_CPU_FIELD_LOCATION;
  inv_param.output_location = QUDA_CPU_FIELD_LOCATION;

  inv_param.dslash_type = dslash_type;

  if (dslash_type == QUDA_TWISTED_MASS_DSLASH || 
      dslash_type == QUDA_TWISTED_CLOVER_DSLASH) {
    inv_param.mu = mu;
    inv_param.twist_flavor = twist_flavor;
    inv_param.Ls = (inv_param.twist_flavor == QUDA_TWIST_NONDEG_DOUBLET) ? 
      2 : 1;
    
    if (twist_flavor == QUDA_TWIST_NONDEG_DOUBLET) {
      printfQuda("Twisted-mass doublet non supported (yet)\n");
      exit(0);
    }
  }
  
  inv_param.dagger = QUDA_DAG_NO;
  inv_param.mass_normalization = normalization;

  // do we want to use an even-odd preconditioned solve or not
  if(isEven) inv_param.matpc_type = QUDA_MATPC_EVEN_EVEN;
  else inv_param.matpc_type = QUDA_MATPC_ODD_ODD;

  inv_param.solution_type = QUDA_MAT_SOLUTION;

  inv_param.solve_type = QUDA_DIRECT_SOLVE;

  mg_param.invert_param = &inv_param;
  mg_param.n_level = mg_levels;
  for (int i=0; i<mg_param.n_level; i++) {
    for (int j=0; j<QUDA_MAX_DIM; j++) {
	// if not defined use 4
      mg_param.geo_block_size[i][j] = geo_block_size[i][j] ? 
	geo_block_size[i][j] : 4;      
    }
    mg_param.verbosity[i] = mg_verbosity[i];
    mg_param.setup_inv_type[i] = setup_inv[i];
    mg_param.num_setup_iter[i] = num_setup_iter[i];
    mg_param.setup_tol[i] = setup_tol;
    mg_param.spin_block_size[i] = 1;
    mg_param.n_vec[i] = nvec[i] == 0 ? 24 : nvec[i]; // default to 24 vectors if not set
    mg_param.precision_null[i] = prec_null; // precision to store the null-space basis
    mg_param.nu_pre[i] = nu_pre;
    mg_param.nu_post[i] = nu_post;
    mg_param.mu_factor[i] = mu_factor[i];
    
    mg_param.cycle_type[i] = QUDA_MG_CYCLE_RECURSIVE;
 
    // set the coarse solver wrappers including bottom solver
    mg_param.coarse_solver[i] = coarse_solver[i];
    mg_param.coarse_solver_tol[i] = coarse_solver_tol[i];
    mg_param.coarse_solver_maxiter[i] = coarse_solver_maxiter[i];
   
    mg_param.smoother[i] = smoother_type[i];

    // set the smoother / bottom solver tolerance (for MR smoothing this will be ignored)
    mg_param.smoother_tol[i] = smoother_tol[i];

    // set to QUDA_DIRECT_PC_SOLVE for to enable even/odd 
    // preconditioning on the smoother
    mg_param.smoother_solve_type[i] = QUDA_DIRECT_PC_SOLVE; // EVEN-ODD

    // set to QUDA_ADDITIVE_SCHWARZ for Additive Schwarz precondioned smoother 
    // (presently only impelemented for MR)
    mg_param.smoother_schwarz_type[i] = schwarz_type[i];

    // if using Schwarz preconditioning then use local reductions only
    mg_param.global_reduction[i] = 
      (schwarz_type[i] == QUDA_INVALID_SCHWARZ) ? QUDA_BOOLEAN_YES : QUDA_BOOLEAN_NO;

    // set number of Schwarz cycles to apply
    mg_param.smoother_schwarz_cycle[i] = schwarz_cycle[i];
    
    // set to QUDA_MAT_SOLUTION to inject a full field into coarse grid
    // set to QUDA_MATPC_SOLUTION to inject single parity field into coarse grid

    // if we are using an outer even-odd preconditioned solve, then we
    // use single parity injection into the coarse grid
    mg_param.coarse_grid_solution_type[i] = 
      solve_type == QUDA_DIRECT_PC_SOLVE ? QUDA_MATPC_SOLUTION : QUDA_MAT_SOLUTION;

    mg_param.omega[i] = omega; // over/under relaxation factor

    mg_param.location[i] = QUDA_CUDA_FIELD_LOCATION;
  }

  // only coarsen the spin on the first restriction
  mg_param.spin_block_size[0] = 2;

  mg_param.setup_type = setup_type;
  mg_param.pre_orthonormalize = pre_orthonormalize ? QUDA_BOOLEAN_YES :  QUDA_BOOLEAN_NO;
  mg_param.post_orthonormalize = post_orthonormalize ? QUDA_BOOLEAN_YES :  QUDA_BOOLEAN_NO;

  mg_param.compute_null_vector = generate_nullspace ? 
    QUDA_COMPUTE_NULL_VECTOR_YES : QUDA_COMPUTE_NULL_VECTOR_NO;
  mg_param.generate_all_levels = generate_all_levels ? 
    QUDA_BOOLEAN_YES :  QUDA_BOOLEAN_NO;

  mg_param.run_verify = verify_results ? QUDA_BOOLEAN_YES : QUDA_BOOLEAN_NO;

  // set file i/o parameters
  strcpy(mg_param.vec_infile, vec_infile);
  strcpy(mg_param.vec_outfile, vec_outfile);

  // these need to be set for now but are actually ignored by the MG setup
  // needed to make it pass the initialization test
  inv_param.inv_type = QUDA_GCR_INVERTER;
  inv_param.tol = tol;
  inv_param.maxiter = niter;
  inv_param.reliable_delta = 1e-10;
  inv_param.gcrNkrylov = 10;

  inv_param.verbosity = QUDA_SUMMARIZE;
  inv_param.verbosity_precondition = QUDA_SUMMARIZE;
}

void setInvertParam(QudaInvertParam &inv_param) {

  if (kappa == -1.0) {
    inv_param.mass = mass;
    inv_param.kappa = 1.0 / (2.0 * (1 + 3/anisotropy + mass));
  } else {
    inv_param.kappa = kappa;
    inv_param.mass = 0.5/kappa - (1.0 + 3.0/anisotropy);
  }
  
  printfQuda("Kappa = %.8f Mass = %.8f\n", inv_param.kappa, inv_param.mass);


  inv_param.Ls = 1;

  inv_param.sp_pad = 0;
  inv_param.cl_pad = 0;

  inv_param.cpu_prec = cpu_prec;
  inv_param.cuda_prec = cuda_prec;
  inv_param.cuda_prec_sloppy = cuda_prec_sloppy;

  inv_param.cuda_prec_precondition = cuda_prec_precondition;
  inv_param.preserve_source = QUDA_PRESERVE_SOURCE_NO;
  inv_param.gamma_basis = QUDA_UKQCD_GAMMA_BASIS;
  inv_param.dirac_order = QUDA_DIRAC_ORDER;

  if (dslash_type == QUDA_CLOVER_WILSON_DSLASH || 
      dslash_type == QUDA_TWISTED_CLOVER_DSLASH) {
    inv_param.clover_cpu_prec = cpu_prec;
    inv_param.clover_cuda_prec = cuda_prec;
    inv_param.clover_cuda_prec_sloppy = cuda_prec_sloppy;
    inv_param.clover_cuda_prec_precondition = cuda_prec_precondition;
    inv_param.clover_order = QUDA_PACKED_CLOVER_ORDER;
    inv_param.clover_coeff = csw*inv_param.kappa;
  }

  inv_param.input_location = QUDA_CPU_FIELD_LOCATION;
  inv_param.output_location = QUDA_CPU_FIELD_LOCATION;

  inv_param.dslash_type = dslash_type;

  if (dslash_type == QUDA_TWISTED_MASS_DSLASH || 
      dslash_type == QUDA_TWISTED_CLOVER_DSLASH) {
    inv_param.mu = mu;
    inv_param.twist_flavor = twist_flavor;
    inv_param.Ls = (inv_param.twist_flavor == QUDA_TWIST_NONDEG_DOUBLET) ? 
      2 : 1;

    if (twist_flavor == QUDA_TWIST_NONDEG_DOUBLET) {
      printfQuda("Twisted-mass doublet non supported (yet)\n");
      exit(0);
    }
  }

  inv_param.dagger = QUDA_DAG_NO;
  inv_param.mass_normalization = normalization;

  // do we want full solution or single-parity solution
  inv_param.solution_type = QUDA_MAT_SOLUTION;

  // do we want to use an even-odd preconditioned solve or not
  inv_param.solve_type = solve_type;
  if(isEven) {
    inv_param.matpc_type = QUDA_MATPC_EVEN_EVEN;
    printfQuda("### Running for the Even-Even Operator\n");
  }
  else {
    printfQuda("### Running for the Odd-Odd Operator\n");
    inv_param.matpc_type = QUDA_MATPC_ODD_ODD;
  }

  inv_param.inv_type = QUDA_GCR_INVERTER;

  inv_param.verbosity = QUDA_VERBOSE;
  inv_param.verbosity_precondition = mg_verbosity[0];

  inv_param.inv_type_precondition = QUDA_MG_INVERTER;
  inv_param.pipeline = pipeline;
  inv_param.gcrNkrylov = gcrNkrylov;
  inv_param.tol = tol;

  // require both L2 relative and heavy quark residual to determine 
  // convergence
  inv_param.residual_type = 
    static_cast<QudaResidualType>(QUDA_L2_RELATIVE_RESIDUAL);
  // specify a tolerance for the residual for heavy quark residual
  inv_param.tol_hq = tol_hq; 
  
  // these can be set individually
  for (int i=0; i<inv_param.num_offset; i++) {
    inv_param.tol_offset[i] = inv_param.tol;
    inv_param.tol_hq_offset[i] = inv_param.tol_hq;
  }
  inv_param.maxiter = niter;
  inv_param.reliable_delta = 1e-4;

  // domain decomposition preconditioner parameters
  inv_param.schwarz_type = QUDA_ADDITIVE_SCHWARZ;
  inv_param.precondition_cycle = 1;
  inv_param.tol_precondition = 1e-1;
  inv_param.maxiter_precondition = 1;
  inv_param.omega = 1.0;


  if(strcmp(verbosity_level,"verbose")==0) 
    inv_param.verbosity = QUDA_VERBOSE;
  else if(strcmp(verbosity_level,"summarize")==0) 
    inv_param.verbosity = QUDA_SUMMARIZE;
  else if(strcmp(verbosity_level,"silent")==0) 
    inv_param.verbosity = QUDA_SILENT;
  else{
    warningQuda("Unknown verbosity level %s. Proceeding with QUDA_SUMMARIZE verbosity level\n",verbosity_level);
    inv_param.verbosity = QUDA_SUMMARIZE;
  }
}


//=======================================================================//
//== C O N T A I N E R   A N D   Q U D A   I N I T I A L I S A T I O N  =//
//=======================================================================//

int main(int argc, char **argv)
{

  using namespace quda;

  // We give here the default value to some of the array
  for(int i =0; i<QUDA_MAX_MG_LEVEL; i++) {
    mg_verbosity[i] = QUDA_SILENT;
    setup_inv[i] = QUDA_BICGSTAB_INVERTER;
    num_setup_iter[i] = 1;
    mu_factor[i] = 1.;
    schwarz_type[i] = QUDA_INVALID_SCHWARZ;
    schwarz_cycle[i] = 1;
    smoother_type[i] = QUDA_MR_INVERTER;
    smoother_tol[i] = 0.25;
    coarse_solver[i] = QUDA_GCR_INVERTER;
    coarse_solver_tol[i] = 0.25;
    coarse_solver_maxiter[i] = 10;
  }

  for (int i = 1; i < argc; i++){
    if(process_command_line_option(argc, argv, &i) == 0){
      continue;
    }
    printf("ERROR: Invalid option:%s\n", argv[i]);
    usage(argv);
  }

  if (prec_sloppy == QUDA_INVALID_PRECISION) prec_sloppy = prec;
  if (prec_precondition == QUDA_INVALID_PRECISION) prec_precondition = prec_sloppy;
  if (link_recon_sloppy == QUDA_RECONSTRUCT_INVALID) link_recon_sloppy = link_recon;
  if (prec_null == QUDA_INVALID_PRECISION) prec_null = prec_precondition;
  if (link_recon_precondition == QUDA_RECONSTRUCT_INVALID) link_recon_precondition = link_recon_sloppy;

  // initialize QMP/MPI, QUDA comms grid and RNG (test_util.cpp)
  initComms(argc, argv, gridsize_from_cmdline);

  // call srand() with a rank-dependent seed
  initRand();

  display_test_info();

  //QKXTM: qkxtm specific inputs
  //--------------------------------------------------------------------
  qudaQKXTMinfo info;  
  info.nsmearGauss = nsmearGauss;
  info.alphaGauss = alphaGauss;
  info.isEven = isEven;
  info.lL[0] = xdim;
  info.lL[1] = ydim;
  info.lL[2] = zdim;
  info.lL[3] = tdim;

  // *** QUDA parameters begin here.

  if (dslash_type != QUDA_TWISTED_MASS_DSLASH && 
      dslash_type != QUDA_TWISTED_CLOVER_DSLASH){
    printfQuda("This test is only for twisted mass or twisted clover operator\n");
    exit(-1);
  }

  QudaGaugeParam gauge_param = newQudaGaugeParam();
  setGaugeParam(gauge_param);

  QudaInvertParam mg_inv_param = newQudaInvertParam();
  QudaMultigridParam mg_param = newQudaMultigridParam();
  mg_param.invert_param = &mg_inv_param;
  setMultigridParam(mg_param);

  QudaInvertParam inv_param = newQudaInvertParam();
  setInvertParam(inv_param);

  setDims(gauge_param.X);
  setSpinorSiteSize(24);

  size_t gSize = (gauge_param.cpu_prec == QUDA_DOUBLE_PRECISION) ? sizeof(double) : sizeof(float);
  size_t sSize = (inv_param.cpu_prec == QUDA_DOUBLE_PRECISION) ? sizeof(double) : sizeof(float);

  //-Read the gauge field in lime format
  void *gauge[4];
  void *gauge_APE[4];

  for (int dir = 0; dir < 4; dir++) {
    gauge[dir] = malloc(V*gaugeSiteSize*gSize);
    gauge_APE[dir] = malloc(V*gaugeSiteSize*gSize);
  }

  // load in the command line supplied gauge field
  readLimeGauge(gauge, latfile, &gauge_param, &inv_param, gridsize_from_cmdline);
  applyBoundaryCondition(gauge, V/2 ,&gauge_param);

  // load in the command line supplied smeared gauge field
  // first read gauge field without apply BC
  readLimeGaugeSmeared(gauge_APE, latfile_smeared, &gauge_param, &inv_param,
		       gridsize_from_cmdline);        
  mapEvenOddToNormalGauge(gauge_APE,gauge_param,xdim,ydim,zdim,tdim);

  // initialize the QUDA library
  initQuda(device);

  // initialize QKXTM info
  init_qudaQKXTM(&info);
  printf_qudaQKXTM();

  // load the gauge field
  loadGaugeQuda((void*)gauge, &gauge_param);

  // this line ensures that if we need to construct the clover inverse 
  // (in either the smoother or the solver) we do so
  if (mg_param.smoother_solve_type[0] == QUDA_DIRECT_PC_SOLVE || 
      solve_type == QUDA_DIRECT_PC_SOLVE) inv_param.solve_type = QUDA_DIRECT_PC_SOLVE;
  
  printfQuda("Constructing clover field\n");
  if (dslash_type == QUDA_TWISTED_CLOVER_DSLASH) 
    loadCloverQuda(NULL, NULL, &inv_param);
  printfQuda("Clover field done\n");
  
  // restore actual solve_type we want to do
  inv_param.solve_type = solve_type; 

  // setup the multigrid solver
  void *mg_preconditioner = newMultigridQuda(&mg_param);
  inv_param.preconditioner = mg_preconditioner;

  printfQuda("\n\n");
  printfQuda("**************************\n");  
  printfQuda("* Begin MG bench routine *\n"); 
  printfQuda("**************************\n\n");  

  MG_bench(gauge_APE, gauge, &gauge_param, &inv_param, info);

  // free the multigrid solver
  destroyMultigridQuda(mg_preconditioner);
  
  freeGaugeQuda();
  if (dslash_type == QUDA_TWISTED_CLOVER_DSLASH) freeCloverQuda();

  for(int i = 0 ; i < 4 ; i++){
    free(gauge_APE[i]);
    free(gauge[i]);
  }

  // finalize the QUDA library
  endQuda();
    
  // finalize the communications layer
  finalizeComms();

  return 0;
}
