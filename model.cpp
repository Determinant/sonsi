#include <cstdio>

#include "model.h"
#include "exc.h"
#include "consts.h"
#include "types.h"

const int REPR_STACK_SIZE = 262144;
extern EmptyList *empty_list;

static EvalObjAddrHash hash;
static ReprCons *repr_stack[REPR_STACK_SIZE];

FrameObj::FrameObj(FrameType _ftype) : ftype(_ftype) {}

EmptyList::EmptyList() : Pair(NULL, NULL) {}

ReprCons *EmptyList::get_repr_cons() { return new ReprStr("()"); }

bool FrameObj::is_ret_addr() {
    return ftype & CLS_RET_ADDR;
}

bool FrameObj::is_parse_bracket() {
    return ftype & CLS_PAR_BRA;
}

EvalObj::EvalObj(int _otype) : FrameObj(CLS_EVAL_OBJ), otype(_otype) {}

void EvalObj::prepare(Pair *pc) {}

bool EvalObj::is_simple_obj() {
    return otype & CLS_SIM_OBJ;
}

bool EvalObj::is_sym_obj() {
    return otype & CLS_SYM_OBJ;
}

bool EvalObj::is_opt_obj() {
    return otype & CLS_OPT_OBJ;
}

bool EvalObj::is_pair_obj() {
    return this != empty_list && (otype & CLS_PAIR_OBJ);
}


bool EvalObj::is_num_obj() {
    return otype & CLS_NUM_OBJ;
}

bool EvalObj::is_bool_obj() {
    return otype & CLS_BOOL_OBJ;
}

bool EvalObj::is_str_obj() {
    return otype & CLS_STR_OBJ;
}

bool EvalObj::is_prom_obj() {
    return otype & CLS_PROM_OBJ;
}

int EvalObj::get_otype() {
    return otype;
}

bool EvalObj::is_true() {
    return true;
}

string EvalObj::ext_repr() {
    hash.clear();
    // TODO: Performance improvement
    // (from possibly O(n^2logn) to strictly O(nlogn))
    ReprCons **top_ptr = repr_stack;
    *top_ptr++ = this->get_repr_cons();
    EvalObj *obj;
    hash.insert(this);
    while (!(*repr_stack)->done)
    {
        if ((*(top_ptr - 1))->done)
        {
            top_ptr -= 2;
            obj = (*top_ptr)->next((*(top_ptr + 1))->repr);
            if (obj)
            {
                *(++top_ptr) = obj->get_repr_cons();
                EvalObj *ptr = (*top_ptr)->ori;
                if (ptr)
                {
                    if (hash.count(ptr))
                        *top_ptr = new ReprStr("#inf#");
                    else hash.insert(ptr);
                }
            }
            else
            {
                hash.erase((*top_ptr)->ori);
                *top_ptr = new ReprStr((*top_ptr)->repr);
            }
        }
        else
        {
            top_ptr--;
            obj = (*top_ptr)->next("");
            if (obj)
            {
                *(++top_ptr) = obj->get_repr_cons();
                EvalObj *ptr = (*top_ptr)->ori;
                if (ptr)
                {
                    if (hash.count(ptr))
                        *top_ptr = new ReprStr("#inf#");
                    else hash.insert(ptr);
                }
            }
            else *top_ptr = new ReprStr((*top_ptr)->repr);
        }
        top_ptr++;
    }
    string &res = (*repr_stack)->repr;
    if (this->is_pair_obj())
        res = "(" + res + ")";
    return res;
}



bool make_exec(Pair *ptr) {
    if (ptr == empty_list) return true;
    EvalObj *nptr;
    for (;;)
        if ((nptr = ptr->cdr)->is_pair_obj())
        {
            ptr->next = TO_PAIR(nptr);
            ptr = ptr->next;
        }
        else break;
    ptr->next = NULL;
    return ptr->cdr == empty_list;
}

