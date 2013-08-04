#ifndef EXC_H
#define EXC_H

#include "consts.h"
#include <string>

using std::string;

/** @class GeneralError
 * The top-level exception
 */
class GeneralError {
    public:
        virtual string get_msg() = 0;   /**< Extract error message */
};

class SyntaxError : public GeneralError {
    protected:
        string msg;                     /**< Error mesg */
        ErrCode code;                   /**< Error code */
    public:

        SyntaxError(ErrCode code);          /**< Construct an SyntaxError */ 
        string get_msg();                   /**< Get the error message */
};

class TokenError : public SyntaxError {
    public:
        TokenError(string token, ErrCode code);     /**< Construct an TokenError */
};

class NormalError : public SyntaxError {
    public:
        NormalError(ErrCode code);
};

#endif
