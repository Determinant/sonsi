#include "exc.h"
#include <cstdio>

SyntaxError::SyntaxError(ErrCode _code) : code(_code) {}

string SyntaxError::get_msg() { return this->msg; }

TokenError::TokenError(string token, ErrCode code) : SyntaxError(code) {
    static char buffer[1024];   // should be enough
    sprintf(buffer, SYN_ERR_MSG[code], token.c_str());
    msg = buffer;
}

NormalError::NormalError(ErrCode code) : SyntaxError(code) {
    msg = SYN_ERR_MSG[code];
}
