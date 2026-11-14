#include "DECIMF64.h"
#include <stdio.h>
#include "Error.h"

/*

The FIR decimators specialized at compile time
(dsp/filtering_functions_ct.h) are tested with the same patterns and the
same checks as arm_fir_decimate_f64.
A function is instantiated for each configuration of the patterns and it
is selected at run-time from the configuration of the instance.

*/

#if defined(ARM_MATH_CT_CHECK) && !defined(ARM_CT_ASSERT)
/* A failed run-time check of the CT functions is a test failure
   (assert() is removed by NDEBUG) */
#define ARM_CT_ASSERT(COND) ASSERT_TRUE(COND)
#endif

#include "dsp/filtering_functions_ct_impl.h"

#define FILTER_CT_KEY(NUMTAPS, FACTOR) ((((uint32_t)(NUMTAPS)) << 8) | ((uint32_t)(FACTOR)))

/* (numTaps, M) of the decimator patterns (PatternGeneration/Decimate.py) */
#define DECIM_CT_CONFIGS(X) \
  X(2,1) X(2,2) X(2,4) X(2,8) X(3,1) X(3,2) X(3,4) X(3,8) \
  X(4,1) X(4,2) X(4,4) X(4,8) X(5,1) X(5,2) X(5,4) X(5,8) \
  X(6,1) X(6,2) X(6,4) X(6,8)

#define DECIM_CT_DEFINE(NT, M)                \
  ARM_FIR_DECIMATE_CT_DECLARE(f64, NT, M) \
  ARM_FIR_DECIMATE_CT_DEFINE(f64, NT, M)

DECIM_CT_CONFIGS(DECIM_CT_DEFINE)

static void fir_decimate_ct_f64(const arm_fir_decimate_instance_f64 *S,
                                const float64_t *pSrc,
                                float64_t *pDst,
                                uint32_t blockSize)
{
    switch (FILTER_CT_KEY(S->numTaps, S->M))
    {
#define DECIM_CT_CASE(NT, M)                                                   \
        case FILTER_CT_KEY(NT, M):                                             \
          ARM_FIR_DECIMATE_CT_FUNC(f64, NT, M)(S, pSrc, pDst, blockSize); \
        break;

        DECIM_CT_CONFIGS(DECIM_CT_CASE)

        default:
          /* No function specialized for this configuration */
          ASSERT_TRUE(false);
        break;
    }
}

#define SNR_THRESHOLD 120

/* 

Reference patterns are generated with
a double precision computation.

*/
#define REL_ERROR (8.0e-4)


    /* Test of arm_fir_decimate_f64 or of a decimator specialized at compile time */
    void DECIMF64::run_fir_decimate(decimate_t decimate)
    {
        int nbTests;
        int nb;
        uint32_t *pConfig = config.ptr();

        const float64_t * pSrc = input.ptr();
        float64_t * pDst = output.ptr();
        float64_t * pCoefs = coefs.ptr();

        nbTests=config.nbSamples() / 4;

        for(nb=0;nb < nbTests; nb++)
        {

            this->q = pConfig[0];
            this->numTaps = pConfig[1];
            this->blocksize = pConfig[2];
            this->refsize = pConfig[3];


            pConfig += 4;

            this->status=arm_fir_decimate_init_f64(&(this->S),
               this->numTaps,
               this->q,
               pCoefs,
               state.ptr(),
               this->blocksize);



            ASSERT_TRUE(this->status == ARM_MATH_SUCCESS);

            decimate(
                 &(this->S),
                 pSrc,
                 pDst,
                 this->blocksize);

            pSrc += this->blocksize;
            pDst += this->refsize;

            pCoefs += this->numTaps;
        }


        ASSERT_EMPTY_TAIL(output);

        ASSERT_SNR(output,ref,(float64_t)SNR_THRESHOLD);

        ASSERT_REL_ERROR(output,ref,REL_ERROR);

    } 

#if 0
    void DECIMF64::test_fir_interpolate_f64()
    {
        int nbTests;
        int nb;
        uint64_t *pConfig = config.ptr();

        const float64_t * pSrc = input.ptr();
        float64_t * pDst = output.ptr();
        float64_t * pCoefs = coefs.ptr();

        nbTests=config.nbSamples() / 4;

        for(nb=0;nb < nbTests; nb++)
        {

            this->q = pConfig[0];
            this->numTaps = pConfig[1];
            this->blocksize = pConfig[2];
            this->refsize = pConfig[3];



            pConfig += 4;

            this->status=arm_fir_interpolate_init_f64(&(this->SI),
               this->q,
               this->numTaps,
               pCoefs,
               state.ptr(),
               this->blocksize);



            ASSERT_TRUE(this->status == ARM_MATH_SUCCESS);

            arm_fir_interpolate_f64(
                 &(this->SI),
                 pSrc,
                 pDst,
                 this->blocksize);

            pSrc += this->blocksize;
            pDst += this->refsize;

            pCoefs += this->numTaps;
        }


        ASSERT_EMPTY_TAIL(output);

        ASSERT_SNR(output,ref,(float64_t)SNR_THRESHOLD);

        ASSERT_REL_ERROR(output,ref,REL_ERROR);

    } 

#endif
    void DECIMF64::test_fir_decimate_f64()
    {
        run_fir_decimate(arm_fir_decimate_f64);
    }

    void DECIMF64::test_fir_decimate_ct_f64()
    {
        run_fir_decimate(fir_decimate_ct_f64);
    }

    void DECIMF64::setUp(Testing::testID_t id,std::vector<Testing::param_t>& params,Client::PatternMgr *mgr)
    {
      
       (void)params;
       
       switch(id)
       {
        case DECIMF64::TEST_FIR_DECIMATE_F64_1:
        case DECIMF64::TEST_FIR_DECIMATE_CT_F64_2:
          config.reload(DECIMF64::CONFIGSDECIMF64_ID,mgr);
         
          input.reload(DECIMF64::INPUT1_F64_ID,mgr);
          coefs.reload(DECIMF64::COEFS1_F64_ID,mgr);

          ref.reload(DECIMF64::REF1_DECIM_F64_ID,mgr);
          state.create(16 + 768 - 1,DECIMF64::STATE_F64_ID,mgr);

          break;

#if 0
        case DECIMF64::TEST_FIR_INTERPOLATE_F64_2:
          config.reload(DECIMF64::CONFIGSINTERPF64_ID,mgr);
         
          input.reload(DECIMF64::INPUT2_F64_ID,mgr);
          coefs.reload(DECIMF64::COEFS2_F64_ID,mgr);

          ref.reload(DECIMF64::REF2_INTERP_F64_ID,mgr);
          state.create(16 + 768 - 1,DECIMF64::STATE_F64_ID,mgr);

          break;
#endif

       }
      

       

       output.create(ref.nbSamples(),DECIMF64::OUT_F64_ID,mgr);
    }

    void DECIMF64::tearDown(Testing::testID_t id,Client::PatternMgr *mgr)
    {
        (void)id;
        output.dump(mgr);
    }
