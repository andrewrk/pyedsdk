#ifndef ERROR_MAP
#define ERROR_MAP

#include "EDSDK.h"
#include "EDSDKErrors.h"
#include "EDSDKTypes.h"

#include <string>
#include <map>
using namespace std;

class ErrorMap {
    public:
        static string errorMsg(EdsError err);

    private:
        static bool s_initialized;
        static map<EdsError, string> s_messages;

        static void initialize();
};

#endif
