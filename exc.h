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

/** @class TokenError
 * Error with some hints
 */
class TokenError : public GeneralError {
    public:
        /** Construct an TokenError */
        TokenError(string token, ErrCode code);     
};

/** @class NormalError
 * Error with constant plain text 
 */
class NormalError : public GeneralError {
    public:
        /** Construct a NormalError */
        NormalError(ErrCode code);
};

#endif
