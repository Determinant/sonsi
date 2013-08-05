#include "exc.h"
#include <cstdio>

GeneralError::GeneralError(ErrCode _code) : code(_code) {}

string GeneralError::get_msg() { return this->msg; }

TokenError::TokenError(string token, ErrCode code) : GeneralError(code) {
    static char buffer[1024];   // should be enough
    sprintf(buffer, ERR_MSG[code], token.c_str());
    msg = buffer;
}

NormalError::NormalError(ErrCode code) : GeneralError(code) {
    msg = ERR_MSG[code];
}
