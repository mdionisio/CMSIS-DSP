#include "DECIMF32.h"
#include <stdio.h>
#include "Error.h"

/*

The FIR decimators and interpolators specialized at compile time
(dsp/filtering_functions_ct.h) are tested with the same patterns and the
same checks as arm_fir_decimate_f32 and arm_fir_interpolate_f32.
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
  ARM_FIR_DECIMATE_CT_DECLARE(f32, NT, M) \
  ARM_FIR_DECIMATE_CT_DEFINE(f32, NT, M)

DECIM_CT_CONFIGS(DECIM_CT_DEFINE)

static void fir_decimate_ct_f32(const arm_fir_decimate_instance_f32 *S,
                                const float32_t *pSrc,
                                float32_t *pDst,
                                uint32_t blockSize)
{
    switch (FILTER_CT_KEY(S->numTaps, S->M))
    {
#define DECIM_CT_CASE(NT, M)                                                   \
        case FILTER_CT_KEY(NT, M):                                             \
          ARM_FIR_DECIMATE_CT_FUNC(f32, NT, M)(S, pSrc, pDst, blockSize); \
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
  ARM_FIR_INTERPOLATE_CT_DECLARE(f32, NT, L) \
  ARM_FIR_INTERPOLATE_CT_DEFINE(f32, NT, L)

INTERP_CT_CONFIGS(INTERP_CT_DEFINE)

static void fir_interpolate_ct_f32(const arm_fir_interpolate_instance_f32 *S,
                                   const float32_t *pSrc,
                                   float32_t *pDst,
                                   uint32_t blockSize)
{
    switch (FILTER_CT_KEY(S->phaseLength * S->L, S->L))
    {
#define INTERP_CT_CASE(NT, L)                                                     \
        case FILTER_CT_KEY(NT, L):                                                \
          ARM_FIR_INTERPOLATE_CT_FUNC(f32, NT, L)(S, pSrc, pDst, blockSize); \
        break;

        INTERP_CT_CONFIGS(INTERP_CT_CASE)

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


    /* Test of arm_fir_decimate_f32 or of a decimator specialized at compile time */
    void DECIMF32::run_fir_decimate(decimate_t decimate)
    {
        int nbTests;
        int nb;
        uint32_t *pConfig = config.ptr();

        const float32_t * pSrc = input.ptr();
        float32_t * pDst = output.ptr();
        float32_t * pCoefs = coefs.ptr();

        nbTests=config.nbSamples() / 4;

        for(nb=0;nb < nbTests; nb++)
        {

            this->q = pConfig[0];
            this->numTaps = pConfig[1];
            this->blocksize = pConfig[2];
            this->refsize = pConfig[3];


            pConfig += 4;

            this->status=arm_fir_decimate_init_f32(&(this->S),
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

        ASSERT_REL_ERROR(output,ref,REL_ERROR);

    } 

    /* Test of arm_fir_interpolate_f32 or of an interpolator specialized at compile time */
    void DECIMF32::run_fir_interpolate(interpolate_t interpolate)
    {
        int nbTests;
        int nb;
        uint32_t *pConfig = config.ptr();

        const float32_t * pSrc = input.ptr();
        float32_t * pDst = output.ptr();
        float32_t * pCoefs = coefs.ptr();

        nbTests=config.nbSamples() / 4;

        for(nb=0;nb < nbTests; nb++)
        {

            this->q = pConfig[0];
            this->numTaps = pConfig[1];
            this->blocksize = pConfig[2];
            this->refsize = pConfig[3];



            pConfig += 4;

            this->status=arm_fir_interpolate_init_f32(&(this->SI),
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

        ASSERT_REL_ERROR(output,ref,REL_ERROR);

    } 

   
    void DECIMF32::test_fir_decimate_f32()
    {
        run_fir_decimate(arm_fir_decimate_f32);
    }

    void DECIMF32::test_fir_decimate_ct_f32()
    {
        run_fir_decimate(fir_decimate_ct_f32);
    }

    void DECIMF32::test_fir_interpolate_f32()
    {
        run_fir_interpolate(arm_fir_interpolate_f32);
    }

    void DECIMF32::test_fir_interpolate_ct_f32()
    {
        run_fir_interpolate(fir_interpolate_ct_f32);
    }

    void DECIMF32::setUp(Testing::testID_t id,std::vector<Testing::param_t>& params,Client::PatternMgr *mgr)
    {
      
       (void)params;
       
       switch(id)
       {
        case DECIMF32::TEST_FIR_DECIMATE_F32_1:
        case DECIMF32::TEST_FIR_DECIMATE_CT_F32_3:
          config.reload(DECIMF32::CONFIGSDECIMF32_ID,mgr);
         
          input.reload(DECIMF32::INPUT1_F32_ID,mgr);
          coefs.reload(DECIMF32::COEFS1_F32_ID,mgr);

          ref.reload(DECIMF32::REF1_DECIM_F32_ID,mgr);
          state.create(16 + 768 - 1,DECIMF32::STATE_F32_ID,mgr);

          break;

        case DECIMF32::TEST_FIR_INTERPOLATE_F32_2:
        case DECIMF32::TEST_FIR_INTERPOLATE_CT_F32_4:
          config.reload(DECIMF32::CONFIGSINTERPF32_ID,mgr);
         
          input.reload(DECIMF32::INPUT2_F32_ID,mgr);
          coefs.reload(DECIMF32::COEFS2_F32_ID,mgr);

          ref.reload(DECIMF32::REF2_INTERP_F32_ID,mgr);
          state.create(16 + 768 - 1,DECIMF32::STATE_F32_ID,mgr);

          break;


       }
      

       

       output.create(ref.nbSamples(),DECIMF32::OUT_F32_ID,mgr);
    }

    void DECIMF32::tearDown(Testing::testID_t id,Client::PatternMgr *mgr)
    {
        (void)id;
        output.dump(mgr);
    }
