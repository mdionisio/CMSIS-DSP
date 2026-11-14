#include "FIRF32.h"
#include <stdio.h>
#include "Error.h"

/*

The FIR filters specialized at compile time (dsp/filtering_functions_ct.h)
are tested with the same patterns and the same checks as arm_fir_f32.
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
  ARM_FIR_CT_DECLARE(f32, NT) \
  ARM_FIR_CT_DEFINE(f32, NT)

FIR_CT_TAPS(FIR_CT_DEFINE)

static void fir_ct_f32(const arm_fir_instance_f32 *S,
                       const float32_t *pSrc,
                       float32_t *pDst,
                       uint32_t blockSize)
{
    switch (S->numTaps)
    {
#define FIR_CT_CASE(NT) \
        case NT: ARM_FIR_CT_FUNC(f32, NT)(S, pSrc, pDst, blockSize); break;

        FIR_CT_TAPS(FIR_CT_CASE)

        default:
          /* No function specialized for this number of taps */
          ASSERT_TRUE(false);
        break;
    }
}

#define SNR_THRESHOLD 120

/* 

Reference patterns are generated with
a double precision computation.

*/
#define REL_ERROR (3.0e-5)

#if defined(ARM_MATH_MVEF) && !defined(ARM_MATH_AUTOVECTORIZE)
static __ALIGNED(8) float32_t coeffArray[32];
#endif 

static void checkInnerTail(float32_t *b)
{
    ASSERT_TRUE(b[0] == 0.0f);
    ASSERT_TRUE(b[1] == 0.0f);
    ASSERT_TRUE(b[2] == 0.0f);
    ASSERT_TRUE(b[3] == 0.0f);
}

// Coef must be padded to a multiple of 4
#define FIRCOEFPADDING 2

    /* Test of arm_fir_f32 or of a FIR specialized at compile time */
    void FIRF32::run_fir(fir_t fir)
    {
        

        const int16_t *configp = configs.ptr();
        float32_t *statep = state.ptr();
        const float32_t *orgcoefsp = coefs.ptr();
        
        const float32_t *coefsp;
        const float32_t *inputp = inputs.ptr();

        float32_t *outp = output.ptr();

        unsigned long i;
#if defined(ARM_MATH_MVEF) && !defined(ARM_MATH_AUTOVECTORIZE)
        int j;
#endif
        int blockSize;
        int numTaps;
#if defined(ARM_MATH_MVEF) && !defined(ARM_MATH_AUTOVECTORIZE)
        int round;
#endif


        /*

        Python script is generating different tests with
        different blockSize and numTaps.

        We loop on those configs.

        */
        for(i=0; i < configs.nbSamples() ; i += 2)
        {
           blockSize = configp[0];
           numTaps = configp[1];

#if defined(ARM_MATH_MVEF) && !defined(ARM_MATH_AUTOVECTORIZE)
           /* Copy coefficients and pad to zero 
           */
           memset(coeffArray,127,32*sizeof(float32_t));
           round = numTaps >> FIRCOEFPADDING;
           if ((round << FIRCOEFPADDING) < numTaps)
           {
             round ++;
           }
           round = round<<FIRCOEFPADDING;
           memset(coeffArray,0,round*sizeof(float32_t));

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
           arm_fir_init_f32(&this->S,numTaps,coefsp,statep,blockSize);

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

        ASSERT_SNR(output,ref,(float32_t)SNR_THRESHOLD);

        ASSERT_REL_ERROR(output,ref,REL_ERROR);

    } 

 
    void FIRF32::test_fir_f32()
    {
        run_fir(arm_fir_f32);
    }

    void FIRF32::test_fir_ct_f32()
    {
        run_fir(fir_ct_f32);
    }

    void FIRF32::setUp(Testing::testID_t id,std::vector<Testing::param_t>& params,Client::PatternMgr *mgr)
    {
      
       (void)params;
       
       switch(id)
       {
        case FIRF32::TEST_FIR_F32_1:
        break;

       }
      

       inputs.reload(FIRF32::FIRINPUTS_F32_ID,mgr);
       coefs.reload(FIRF32::FIRCOEFS_F32_ID,mgr);
       configs.reload(FIRF32::FIRCONFIGS_S16_ID,mgr);
       ref.reload(FIRF32::FIRREFS_F32_ID,mgr);

       output.create(ref.nbSamples(),FIRF32::OUT_F32_ID,mgr);
       /* Max 2*blockSize + numTaps - 1 as generated by Python script 
          A temp buffer blockSize is used by Helium implementation.
          It is at beginning of state buffer and is NOT the state
          of the FIR which is in the following part.
       */
       state.create(47+47,FIRF32::OUT_F32_ID,mgr);



    }

    void FIRF32::tearDown(Testing::testID_t id,Client::PatternMgr *mgr)
    {
        (void)id;
        output.dump(mgr);
    }
