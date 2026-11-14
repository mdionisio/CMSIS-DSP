#include "DECIMQ31.h"
#include <stdio.h>
#include "Error.h"

/*

The FIR decimators and interpolators specialized at compile time
(dsp/filtering_functions_ct.h) are tested with the same patterns and the
same checks as arm_fir_decimate_q31 and arm_fir_interpolate_q31.
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
  X(3,1) X(3,2) X(3,4) X(3,8) X(4,1) X(4,2) X(4,4) X(4,8) \
  X(8,1) X(8,2) X(8,4) X(8,8) X(9,1) X(9,2) X(9,4) X(9,8) \
  X(11,1) X(11,2) X(11,4) X(11,8) X(12,1) X(12,2) X(12,4) X(12,8)

#define DECIM_CT_DEFINE(NT, M)                \
  ARM_FIR_DECIMATE_CT_DECLARE(q31, NT, M) \
  ARM_FIR_DECIMATE_CT_DEFINE(q31, NT, M)

DECIM_CT_CONFIGS(DECIM_CT_DEFINE)

static void fir_decimate_ct_q31(const arm_fir_decimate_instance_q31 *S,
                                const q31_t *pSrc,
                                q31_t *pDst,
                                uint32_t blockSize)
{
    switch (FILTER_CT_KEY(S->numTaps, S->M))
    {
#define DECIM_CT_CASE(NT, M)                                                   \
        case FILTER_CT_KEY(NT, M):                                             \
          ARM_FIR_DECIMATE_CT_FUNC(q31, NT, M)(S, pSrc, pDst, blockSize); \
        break;

        DECIM_CT_CONFIGS(DECIM_CT_CASE)

        default:
          /* No function specialized for this configuration */
          ASSERT_TRUE(false);
        break;
    }
}

/* (numTaps, L) of the interpolator patterns (PatternGeneration/Decimate.py) */
#define INTERP_CT_CONFIGS(X) \
  X(4,1) X(8,1) X(8,2) X(16,2) X(16,4) X(32,4) X(20,5) X(40,5) \
  X(32,8) X(64,8) X(36,9) X(72,9)

#define INTERP_CT_DEFINE(NT, L)                  \
  ARM_FIR_INTERPOLATE_CT_DECLARE(q31, NT, L) \
  ARM_FIR_INTERPOLATE_CT_DEFINE(q31, NT, L)

INTERP_CT_CONFIGS(INTERP_CT_DEFINE)

static void fir_interpolate_ct_q31(const arm_fir_interpolate_instance_q31 *S,
                                   const q31_t *pSrc,
                                   q31_t *pDst,
                                   uint32_t blockSize)
{
    switch (FILTER_CT_KEY(S->phaseLength * S->L, S->L))
    {
#define INTERP_CT_CASE(NT, L)                                                     \
        case FILTER_CT_KEY(NT, L):                                                \
          ARM_FIR_INTERPOLATE_CT_FUNC(q31, NT, L)(S, pSrc, pDst, blockSize); \
        break;

        INTERP_CT_CONFIGS(INTERP_CT_CASE)

        default:
          /* No function specialized for this configuration */
          ASSERT_TRUE(false);
        break;
    }
}

#define SNR_THRESHOLD 100

/* 

Reference patterns are generated with
a double precision computation.

*/
#define ABS_ERROR_Q31 ((q31_t)2)
#define ABS_ERROR_Q63 ((q63_t)(1<<17))

#define ONEHALF 0x40000000


    /* Test of arm_fir_decimate_q31 or of a decimator specialized at compile time */
    void DECIMQ31::run_fir_decimate(decimate_t decimate)
    {
        int nbTests;
        int nb;
        uint32_t *pConfig = config.ptr();

        const q31_t * pSrc = input.ptr();
        q31_t * pDst = output.ptr();
        q31_t * pCoefs = coefs.ptr();

        nbTests=config.nbSamples() / 4;

        for(nb=0;nb < nbTests; nb++)
        {

            this->q = pConfig[0];
            this->numTaps = pConfig[1];
            this->blocksize = pConfig[2];
            this->refsize = pConfig[3];


            pConfig += 4;

            this->status=arm_fir_decimate_init_q31(&(this->S),
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

        ASSERT_SNR(output,ref,(float32_t)SNR_THRESHOLD);

        ASSERT_NEAR_EQ(output,ref,ABS_ERROR_Q31);

    } 

    /* Test of arm_fir_interpolate_q31 or of an interpolator specialized at compile time */
    void DECIMQ31::run_fir_interpolate(interpolate_t interpolate)
    {
        int nbTests;
        int nb;
        uint32_t *pConfig = config.ptr();

        const q31_t * pSrc = input.ptr();
        q31_t * pDst = output.ptr();
        q31_t * pCoefs = coefs.ptr();

        nbTests=config.nbSamples() / 4;

        for(nb=0;nb < nbTests; nb++)
        {

            this->q = pConfig[0];
            this->numTaps = pConfig[1];
            this->blocksize = pConfig[2];
            this->refsize = pConfig[3];


            pConfig += 4;

            this->status=arm_fir_interpolate_init_q31(&(this->SI),
               this->q,
               this->numTaps,
               pCoefs,
               state.ptr(),
               this->blocksize);



            ASSERT_TRUE(this->status == ARM_MATH_SUCCESS);

            interpolate(
                 &(this->SI),
                 pSrc,
                 pDst,
                 this->blocksize);

            pSrc += this->blocksize;
            pDst += this->refsize;

            pCoefs += this->numTaps;
        }


        ASSERT_EMPTY_TAIL(output);

        ASSERT_SNR(output,ref,(float32_t)SNR_THRESHOLD);

        ASSERT_NEAR_EQ(output,ref,ABS_ERROR_Q31);

    } 

   
    void DECIMQ31::test_fir_decimate_q31()
    {
        run_fir_decimate(arm_fir_decimate_q31);
    }

    void DECIMQ31::test_fir_decimate_ct_q31()
    {
        run_fir_decimate(fir_decimate_ct_q31);
    }

    void DECIMQ31::test_fir_interpolate_q31()
    {
        run_fir_interpolate(arm_fir_interpolate_q31);
    }

    void DECIMQ31::test_fir_interpolate_ct_q31()
    {
        run_fir_interpolate(fir_interpolate_ct_q31);
    }

    void DECIMQ31::setUp(Testing::testID_t id,std::vector<Testing::param_t>& params,Client::PatternMgr *mgr)
    {
      
       
       (void)params;
       switch(id)
       {
        case DECIMQ31::TEST_FIR_DECIMATE_Q31_1:
        case DECIMQ31::TEST_FIR_DECIMATE_CT_Q31_3:
          config.reload(DECIMQ31::CONFIGSDECIMQ31_ID,mgr);

          input.reload(DECIMQ31::INPUT1_Q31_ID,mgr);
          coefs.reload(DECIMQ31::COEFS1_Q31_ID,mgr);

          ref.reload(DECIMQ31::REF1_DECIM_Q31_ID,mgr);
          state.create(16 + 768 - 1,DECIMQ31::STATE_Q31_ID,mgr);

          break;

        case DECIMQ31::TEST_FIR_INTERPOLATE_Q31_2:
        case DECIMQ31::TEST_FIR_INTERPOLATE_CT_Q31_4:
          config.reload(DECIMQ31::CONFIGSINTERPQ31_ID,mgr);

          input.reload(DECIMQ31::INPUT2_Q31_ID,mgr);
          coefs.reload(DECIMQ31::COEFS2_Q31_ID,mgr);

          ref.reload(DECIMQ31::REF2_INTERP_Q31_ID,mgr);
          state.create(16 + 768 - 1,DECIMQ31::STATE_Q31_ID,mgr);

          break;

       }
      

       

       output.create(ref.nbSamples(),DECIMQ31::OUT_Q31_ID,mgr);
    }

    void DECIMQ31::tearDown(Testing::testID_t id,Client::PatternMgr *mgr)
    {
        (void)id;
        output.dump(mgr);
    }
