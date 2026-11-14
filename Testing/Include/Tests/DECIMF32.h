#include "Test.h"
#include "Pattern.h"

#include "dsp/filtering_functions.h"

class DECIMF32:public Client::Suite
    {
        public:
            DECIMF32(Testing::testID_t id);
            virtual void setUp(Testing::testID_t,std::vector<Testing::param_t>& params,Client::PatternMgr *mgr);
            virtual void tearDown(Testing::testID_t,Client::PatternMgr *mgr);
        private:
            #include "DECIMF32_decl.h"

            /* Tested function : arm_fir_decimate_f32 or a decimator specialized at compile time */
            typedef void (*decimate_t)(const arm_fir_decimate_instance_f32 *, const float32_t *, float32_t *, uint32_t);
            void run_fir_decimate(decimate_t decimate);

            /* Tested function : arm_fir_interpolate_f32 or an interpolator specialized at compile time */
            typedef void (*interpolate_t)(const arm_fir_interpolate_instance_f32 *, const float32_t *, float32_t *, uint32_t);
            void run_fir_interpolate(interpolate_t interpolate);
            
            Client::Pattern<float32_t> input;
            Client::Pattern<float32_t> coefs;
            Client::Pattern<uint32_t> config;

            Client::LocalPattern<float32_t> output;
            Client::LocalPattern<float32_t> state;
            // Reference patterns are not loaded when we are in dump mode
            Client::RefPattern<float32_t> ref;


            arm_fir_decimate_instance_f32 S;
            arm_fir_interpolate_instance_f32 SI;

            int q;
            int numTaps;
            int blocksize;
            int refsize;

            arm_status status;
    };
