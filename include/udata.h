#include <nvector/nvector_serial.h>  /* defs. of serial NVECTOR fcts. and macros  */
#include <sundials/sundials_klu_impl.h> /* def. of type klu solver */
#include <sundials/sundials_sparse.h> /* def. of type sparse stuff */
#include <sundials/sundials_types.h> /* def. of type realtype */
#include <sundials/sundials_math.h>  /* definition of ABS */
#include <sundials/sundials_config.h>

#define plist udata->am_plist
#define np udata->am_np
#define ny udata->am_ny
#define nx udata->am_nx
#define nr udata->am_nr
#define nt udata->am_nt
#define ndisc udata->am_ndisc
#define nnz udata->am_nnz
#define nmaxroot udata->am_nmaxroot
#define nmaxdisc udata->am_nmaxdisc

#define p udata->am_p
#define k udata->am_k

#define tstart udata->am_tstart
#define ts udata->am_ts

#define pbar udata->am_pbar
#define xbar udata->am_xbar

#define idlist udata->am_idlist

#define sensi udata->am_sensi
#define atol udata->am_atol
#define rtol udata->am_rtol
#define maxsteps udata->am_maxsteps

#define ism udata->am_ism
#define sensi_meth udata->am_sensi_meth

#define linsol udata->am_linsol
#define interpType udata->am_interpType

#define lmm udata->am_lmm
#define iter udata->am_iter

#define stldet udata->am_stldet

#define ubw udata->am_ubw
#define lbw udata->am_lbw

#define b_sx0 udata->am_bsx0
#define sx0data udata->am_sx0data

#define event_model udata->am_event_model
#define data_model udata->am_data_model

#define ordering udata->am_ordering

#ifndef _MY_UDATA
#define _MY_UDATA

/** @brief struct that stores all user provided data */
typedef struct {
    
    /** parameter reordering */
    int    *am_plist;
    /** number of parameters */
    int    am_np;
    /** number of observables */
    int    am_ny;
    /** number of states */
    int    am_nx;
    /** number of roots */
    int    am_nr;
    /** number of timepoints */
    int    am_nt;
    /** number of discontinuities */
    int    am_ndisc;
    /** number of nonzero entries in jacobian */
    int    am_nnz;
    /** maximal number of roots to collect */
    int    am_nmaxroot;
    /** maximal number of discontinuities to track */
    int    am_nmaxdisc;
    
    /** parameter array */
    double *am_p;
    /** constants array */
    double *am_k;
    
    /** starting time */
    double am_tstart;
    /** timepoints */
    double *am_ts;
    
    /** scaling of parameters */
    double *am_pbar;
    /** scaling of states */
    double *am_xbar;
    
    /** flag array for DAE equations */
    double *am_idlist;
    
    /** flag indicating whether sensitivities are supposed to be computed */
    int am_sensi;
    /** absolute tolerances for integration */
    double am_atol;
    /** relative tolerances for integration */
    double am_rtol;
    /** maximum number of allowed integration steps */
    int am_maxsteps;
    
    /** internal sensitivity method */
    /*!
     * a flag used to select the sensitivity solution method. Its value can be CV SIMULTANEOUS or CV STAGGERED. Only applies for Forward Sensitivities.
     */
    int am_ism;
    
    /** method for sensitivity computation */
    /*!
     * CW_FSA for forward sensitivity analysis, CW_ASA for adjoint sensitivity analysis
     */
    int am_sensi_meth;
    /** linear solver specification */
    int am_linsol;
    /** interpolation type */
    /*!
     * specifies the interpolation type for the forward problem solution which is then used for the backwards problem. can be either CV_POLYNOMIAL or CV_HERMITE
     */
    int am_interpType;
    
    /** linear multistep method */
    /*!
     * specifies the linear multistep method and may be one of two possible values: CV ADAMS or CV BDF.
     */
    int am_lmm;
    
    /** nonlinear solver */
    /*!
     * specifies the type of nonlinear solver iteration and may be either CV NEWTON or CV FUNCTIONAL.
     */
    int am_iter;
    
    /** flag controlling stability limit detection */
    booleantype am_stldet;
    
    /** upper bandwith of the jacobian */
    int am_ubw;
    /** lower bandwith of the jacobian */
    int am_lbw;
    
    /** flag for sensitivity initialisation */
    /*!
     * flag which determines whether analytic sensitivities initialisation or provided initialisation should be used
     */
    booleantype am_bsx0;
    
    /** sensitivity initialisation */
    double *am_sx0data;
    
    /** error model for events */
    int am_event_model;
    /** error model for udata */
    int am_data_model;
    
    /** state ordering */
    int am_ordering;
    
} *UserData;
#endif /* _MY_UDATA */
