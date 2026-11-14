#include "Test.h"
#include "Pattern.h"

#include "dsp/filtering_functions.h"

class FIRQ7:public Client::Suite
    {
        public:
            FIRQ7(Testing::testID_t id);
            virtual void setUp(Testing::testID_t,std::vector<Testing::param_t>& params,Client::PatternMgr *mgr);
            virtual void tearDown(Testing::testID_t,Client::PatternMgr *mgr);
        private:
            #include "FIRQ7_decl.h"

            /* Tested function : arm_fir_q7 or a FIR specialized at compile time */
            typedef void (*fir_t)(const arm_fir_instance_q7 *, const q7_t *, q7_t *, uint32_t);
            void run_fir(fir_t fir);
            
            Client::Pattern<q7_t> coefs;
            Client::Pattern<q7_t> inputs;
            Client::RefPattern<int16_t> configs;
            Client::LocalPattern<q7_t> output;
            Client::LocalPattern<q7_t> state;
            // Reference patterns are not loaded when we are in dump mode
            Client::RefPattern<q7_t> ref;


            arm_fir_instance_q7 S;
    };
