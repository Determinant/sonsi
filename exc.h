#ifndef EXC_H
#define EXC_H

#include "consts.h"
#include <string>

using std::string;

/** @class GeneralError
 * The top-level exception
 */
class GeneralError {
    protected:
        string msg;                     /**< Error mesg */
        ErrCode code;                   /**< Error code */
    public:

        GeneralError(ErrCode code);         /**< Construct a General Error */ 
        string get_msg();                   /**< Get the error message */
};

class TokenError : public GeneralError {
    public:
        TokenError(string token, ErrCode code);     /**< Construct an TokenError */
};

class NormalError : public GeneralError {
    public:
        NormalError(ErrCode code);
};

#endif
