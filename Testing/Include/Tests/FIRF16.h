#include "Test.h"
#include "Pattern.h"

#include "dsp/filtering_functions_f16.h"

class FIRF16:public Client::Suite
    {
        public:
            FIRF16(Testing::testID_t id);
            virtual void setUp(Testing::testID_t,std::vector<Testing::param_t>& params,Client::PatternMgr *mgr);
            virtual void tearDown(Testing::testID_t,Client::PatternMgr *mgr);
        private:
            #include "FIRF16_decl.h"

            /* Tested function : arm_fir_f16 or a FIR specialized at compile time */
            typedef void (*fir_t)(const arm_fir_instance_f16 *, const float16_t *, float16_t *, uint32_t);
            void run_fir(fir_t fir);
            
            Client::Pattern<float16_t> coefs;
            Client::Pattern<float16_t> inputs;
            Client::RefPattern<int16_t> configs;
            Client::LocalPattern<float16_t> output;
            Client::LocalPattern<float16_t> state;
            // Reference patterns are not loaded when we are in dump mode
            Client::RefPattern<float16_t> ref;


            arm_fir_instance_f16 S;

    };
