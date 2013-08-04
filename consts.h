#ifndef CONSTS_H
#define CONSTS_H

enum ErrCode {
    SYN_ERR_NOT_AN_ID,
    SYN_ERR_CAN_NOT_APPLY,
    SYN_ERR_ID_EXPECTED,
    SYN_ERR_UNBOUND_VAR
};

extern const char *SYN_ERR_MSG[];

#endif
