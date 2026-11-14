#include "Test.h"
#include "Pattern.h"

#include "dsp/filtering_functions.h"

class FIRF64:public Client::Suite
    {
        public:
            FIRF64(Testing::testID_t id);
            virtual void setUp(Testing::testID_t,std::vector<Testing::param_t>& params,Client::PatternMgr *mgr);
            virtual void tearDown(Testing::testID_t,Client::PatternMgr *mgr);
        private:
            #include "FIRF64_decl.h"

            /* Tested function : arm_fir_f64 or a FIR specialized at compile time */
            typedef void (*fir_t)(const arm_fir_instance_f64 *, const float64_t *, float64_t *, uint32_t);
            void run_fir(fir_t fir);
            
            Client::Pattern<float64_t> coefs;
            Client::Pattern<float64_t> inputs;
            Client::RefPattern<int16_t> configs;

            Client::LocalPattern<float64_t> output;
            Client::LocalPattern<float64_t> state;
            Client::LocalPattern<float64_t> tmp;
            // Reference patterns are not loaded when we are in dump mode
            Client::RefPattern<float64_t> ref;


            arm_fir_instance_f64 S;

    };
