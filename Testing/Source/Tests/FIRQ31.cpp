#include "FIRQ31.h"
#include <stdio.h>
#include "Error.h"

/*

The FIR filters specialized at compile time (dsp/filtering_functions_ct.h)
are tested with the same patterns and the same checks as arm_fir_q31.
A function is instantiated for each number of taps of the patterns and
it is selected at run-time from the number of taps of the instance.

*/

#if defined(ARM_MATH_CT_CHECK) && !defined(ARM_CT_ASSERT)
/* A failed run-time check of the CT functions is a test failure
   (assert() is removed by NDEBUG) */
#define ARM_CT_ASSERT(COND) ASSERT_TRUE(COND)
#endif

#include "dsp/filtering_functions_ct_impl.h"

/* Number of taps of the patterns (PatternGeneration/FIR.py) */
#define FIR_CT_TAPS(X) \
  X(1) X(2) X(3) X(4) X(5) X(6) X(7) X(8) \
  X(11) X(16) X(23) X(25)

#define FIR_CT_DEFINE(NT)       \
  ARM_FIR_CT_DECLARE(q31, NT) \
  ARM_FIR_CT_DEFINE(q31, NT)

FIR_CT_TAPS(FIR_CT_DEFINE)

static void fir_ct_q31(const arm_fir_instance_q31 *S,
                       const q31_t *pSrc,
                       q31_t *pDst,
                       uint32_t blockSize)
{
    switch (S->numTaps)
    {
#define FIR_CT_CASE(NT) \
        case NT: ARM_FIR_CT_FUNC(q31, NT)(S, pSrc, pDst, blockSize); break;

        FIR_CT_TAPS(FIR_CT_CASE)

        default:
          /* No function specialized for this number of taps */
          ASSERT_TRUE(false);
        break;
    }
}

#define SNR_THRESHOLD 100

#define ABS_ERROR_Q31 ((q31_t)2)

#if defined(ARM_MATH_MVEI) && !defined(ARM_MATH_AUTOVECTORIZE)
static __ALIGNED(8) q31_t coeffArray[32];
#endif

static void checkInnerTail(q31_t *b)
{
    ASSERT_TRUE(b[0] == 0);
    ASSERT_TRUE(b[1] == 0);
    ASSERT_TRUE(b[2] == 0);
    ASSERT_TRUE(b[3] == 0);
}

// Coef must be padded to a multiple of 4
#define FIRCOEFPADDING 2

    /* Test of arm_fir_q31 or of a FIR specialized at compile time */
    void FIRQ31::run_fir(fir_t fir)
    {
        

        const int16_t *configp = configs.ptr();
        q31_t *statep = state.ptr();
        const q31_t *orgcoefsp = coefs.ptr();

        const q31_t *coefsp;
        const q31_t *inputp = inputs.ptr();
        q31_t *outp = output.ptr();

        unsigned long i;
#if defined(ARM_MATH_MVEI) && !defined(ARM_MATH_AUTOVECTORIZE)
        int j;
        int round;
#endif
        int blockSize;
        int numTaps;

        /*

        Python script is generating different tests with
        different blockSize and numTaps.

        We loop on those configs.

        */
        for(i=0; i < configs.nbSamples() ; i += 2)
        {
           blockSize = configp[0];
           numTaps = configp[1];

#if defined(ARM_MATH_MVEI) && !defined(ARM_MATH_AUTOVECTORIZE)
           /* Copy coefficients and pad to zero 
           */
           memset(coeffArray,127,32*sizeof(q31_t));
           round = numTaps >> FIRCOEFPADDING;
           if ((round << FIRCOEFPADDING) < numTaps)
           {
             round ++;
           }
           round = round<<FIRCOEFPADDING;
           memset(coeffArray,0,round*sizeof(q31_t));

           //printf("blockSize=%d, numTaps=%d, round=%d (%d)\n",blockSize,numTaps,round,round - numTaps);


           for(j=0;j < numTaps; j++)
           {
              coeffArray[j] = orgcoefsp[j];
           }
   
           coefsp = coeffArray;
#else
           coefsp = orgcoefsp;
#endif

           /*

           The filter is initialized with the coefs, blockSize and numTaps.

           */
           arm_fir_init_q31(&this->S,numTaps,coefsp,statep,blockSize);

           /*

           Input pointer is reset since the same input pattern is used

           */
           inputp = inputs.ptr();

           /*
           
           Python script is filtering a 2*blockSize number of samples.
           We do the same filtering in two pass to check (indirectly that
           the state management of the fir is working.)

           */
           fir(&this->S,inputp,outp,blockSize);
           outp += blockSize;
           checkInnerTail(outp);

           inputp += blockSize;
           fir(&this->S,inputp,outp,blockSize);
           outp += blockSize;
           checkInnerTail(outp);

           configp += 2;
           orgcoefsp += numTaps;



        }

        ASSERT_EMPTY_TAIL(output);

        ASSERT_SNR(output,ref,(q31_t)SNR_THRESHOLD);

        ASSERT_NEAR_EQ(output,ref,ABS_ERROR_Q31);

    } 

 
    void FIRQ31::test_fir_q31()
    {
        run_fir(arm_fir_q31);
    }

    void FIRQ31::test_fir_ct_q31()
    {
        run_fir(fir_ct_q31);
    }

    void FIRQ31::setUp(Testing::testID_t id,std::vector<Testing::param_t>& params,Client::PatternMgr *mgr)
    {
      
       (void)params;
       switch(id)
       {
        case FIRQ31::TEST_FIR_Q31_1:
          
        break;

       }
      

       inputs.reload(FIRQ31::FIRINPUTS_Q31_ID,mgr);
       coefs.reload(FIRQ31::FIRCOEFS_Q31_ID,mgr);
       configs.reload(FIRQ31::FIRCONFIGS_S16_ID,mgr);
       ref.reload(FIRQ31::FIRREFS_Q31_ID,mgr);

       output.create(ref.nbSamples(),FIRQ31::OUT_Q31_ID,mgr);
       /* > Max blockSize + numTaps - 1 as generated by Python script */
       state.create(47 + 47+47,FIRQ31::OUT_Q31_ID,mgr);
    }

    void FIRQ31::tearDown(Testing::testID_t id,Client::PatternMgr *mgr)
    {
        (void)id;
        output.dump(mgr);
    }
