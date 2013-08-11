#include <cstdio>
#include <cctype>
#include <cstdlib>

#include "consts.h"
#include "builtin.h"
#include "model.h"
#include "exc.h"
#include "types.h"

using std::stringstream;
extern EmptyList *empty_list;

#define ARGS_EXACTLY_TWO \
    if (args == empty_list ||  \
        args->cdr == empty_list || \
        TO_PAIR(args->cdr)->cdr != empty_list) \
        throw TokenError(name, RUN_ERR_WRONG_NUM_OF_ARGS)

#define ARGS_EXACTLY_ONE \
    if (args == empty_list || \
        args->cdr != empty_list ) \
        throw TokenError(name, RUN_ERR_WRONG_NUM_OF_ARGS)

#define ARGS_AT_LEAST_ONE \
    if (args == empty_list) \
        throw TokenError(name, RUN_ERR_WRONG_NUM_OF_ARGS)

SpecialOptIf::SpecialOptIf() : SpecialOptObj("if") {}

void SpecialOptIf::prepare(Pair *pc) {
#define IF_EXP_ERR \
        throw TokenError(name, RUN_ERR_WRONG_NUM_OF_ARGS)
    state = 0;  // Prepared

    Pair *first, *second, *third;

    if (pc->cdr->is_pair_obj())
        first = TO_PAIR(pc->cdr);
    else
        IF_EXP_ERR;

    if (first->cdr->is_pair_obj())
        second = TO_PAIR(first->cdr);
    else
        IF_EXP_ERR;

    if (second->cdr != empty_list)
    {
        if (second->cdr->is_pair_obj())
        {
            third = TO_PAIR(second->cdr);
            if (third->cdr != empty_list)
                IF_EXP_ERR;
        }
        else
            IF_EXP_ERR;
    }
    pc->next = first;
    first->next = NULL; // skip <consequence> and <alternative>
}

void SpecialOptIf::pre_call(Pair *args, Pair *pc,
        Environment *envt) {
    // prepare has guaranteed ...
    pc = TO_PAIR(pc->car);
    Pair *first = TO_PAIR(pc->cdr);
    Pair *second = TO_PAIR(first->cdr);
    Pair *third = TO_PAIR(second->cdr);

    // Condition evaluated and the decision is made
    state = 1;

    if (TO_PAIR(args->cdr)->car->is_true())
    {
        pc->next = second;
        second->next = NULL;
    }
    else
    {
        pc->next = third;
        third->next = NULL;
    }
}

EvalObj *SpecialOptIf::post_call(Pair *args, Pair *pc,
        Environment *envt) {
    // Value already evaluated, so just return it
    return TO_PAIR(args->cdr)->car;
}

Pair *SpecialOptIf::call(Pair *args, Environment * &envt,
        Continuation * &cont, FrameObj ** &top_ptr) {
    Pair *ret_addr = static_cast<RetAddr*>(*top_ptr)->addr;
    if (state)
    {
        *top_ptr++ = post_call(args, ret_addr, envt);
        return ret_addr->next;          // Move to the next instruction
    }
    else
    {
        pre_call(args, ret_addr, envt);
        top_ptr += 2;
        // Undo pop and invoke again
        // static_cast because it's a call invocation
        return TO_PAIR(ret_addr->car)->next;
    }
}

ReprCons *SpecialOptIf::get_repr_cons() {
    return new ReprStr("#<Builtin Macro: if>");
}

SpecialOptLambda::SpecialOptLambda() : SpecialOptObj("lambda") {}
#define CHECK_COM(pc)  \
do  \
{ \
    EvalObj *nptr; \
    Pair *ptr; \
    for (ptr = pc;;)  \
    { \
        if ((nptr = ptr->cdr)->is_pair_obj()) \
            ptr = TO_PAIR(nptr); \
        else break; \
    } \
    if (ptr->cdr != empty_list) \
        throw TokenError(name, RUN_ERR_WRONG_NUM_OF_ARGS); \
} \
while (0)

#define CHECK_SYMBOL(ptr) \
do \
{ \
    if (!(ptr)->is_sym_obj()) \
        throw TokenError("a symbol", RUN_ERR_WRONG_TYPE); \
} while (0)

#define CHECK_PARA_LIST(p) \
do  \
{ \
    if (p == empty_list) break; \
    EvalObj *nptr; \
    Pair *ptr; \
    for (ptr = TO_PAIR(p);;)  \
    { \
        if ((nptr = ptr->cdr)->is_pair_obj()) \
            ptr = TO_PAIR(nptr); \
        else break; \
        CHECK_SYMBOL(ptr->car); \
    } \
    if (ptr->cdr != empty_list) \
       CHECK_SYMBOL(ptr->cdr); \
} \
while (0)


void SpecialOptLambda::prepare(Pair *pc) {
    // Do not evaluate anything
    CHECK_COM(pc);
    pc->next = NULL;
}

Pair *SpecialOptLambda::call(Pair *args, Environment * &envt,
        Continuation * &cont, FrameObj ** &top_ptr) {

    Pair *ret_addr = static_cast<RetAddr*>(*top_ptr)->addr;
    Pair *pc = static_cast<Pair*>(ret_addr->car);
    // TODO: remove the following two lines?
    if (pc->cdr == empty_list)
        throw TokenError(name, SYN_ERR_EMPTY_PARA_LIST);
    Pair *first = TO_PAIR(pc->cdr);
    // <body> is expected
    if (first->cdr == empty_list)
        throw TokenError(name, SYN_ERR_MISS_OR_EXTRA_EXP);

    // Restore the next pointer
    pc->next = TO_PAIR(pc->cdr);            // CHECK_COM made it always okay

    if (first->car->is_simple_obj())
        CHECK_SYMBOL(first->car);
    else
        CHECK_PARA_LIST(first->car);
    EvalObj *params = first->car;

    // store a list of expressions inside <body>
    Pair *body = TO_PAIR(first->cdr);       // Truncate the expression list
    for (Pair *ptr = body; ptr != empty_list; ptr = TO_PAIR(ptr->cdr))
        ptr->next = NULL;    // Make each expression an orphan

    *top_ptr++ = new ProcObj(body, envt, params);
    return ret_addr->next;  // Move to the next instruction
}

ReprCons *SpecialOptLambda::get_repr_cons() {
    return new ReprStr("#<Builtin Macro: lambda>");
}

SpecialOptDefine::SpecialOptDefine() : SpecialOptObj("define") {}

void SpecialOptDefine::prepare(Pair *pc) {
    if (!pc->cdr->is_pair_obj())
        throw TokenError(name, RUN_ERR_WRONG_NUM_OF_ARGS);
    Pair *first = TO_PAIR(pc->cdr), *second;
    if (first->car->is_simple_obj())  // Simple value assignment
    {
        if (!first->cdr->is_pair_obj())
            throw TokenError(name, RUN_ERR_WRONG_NUM_OF_ARGS);
        second = TO_PAIR(first->cdr);
        if (second->cdr != empty_list)
            throw TokenError(name, RUN_ERR_WRONG_NUM_OF_ARGS);
        pc->next = second;                      // Skip the identifier
        second->next = NULL;
    }                                           // Procedure definition
    else
    {
        CHECK_COM(pc);
        pc->next = NULL; // Skip all parts
    }
}

Pair *SpecialOptDefine::call(Pair *args, Environment * &envt,
        Continuation * &cont, FrameObj ** &top_ptr) {
    Pair *ret_addr = static_cast<RetAddr*>(*top_ptr)->addr;
    Pair *pc = static_cast<Pair*>(ret_addr->car);
    EvalObj *obj;
    SymObj *id;
    EvalObj *first = TO_PAIR(pc->cdr)->car;
    if (first->is_simple_obj())
    {
        if (!first->is_sym_obj())
            throw TokenError(first->ext_repr(), SYN_ERR_NOT_AN_ID);
        id = static_cast<SymObj*>(first);
        obj = TO_PAIR(args->cdr)->car;
    }
    else
    {
        // static_cast because of is_simple_obj() is false
        Pair *plst = static_cast<Pair*>(first);

        if (plst == empty_list)
            throw TokenError(name, SYN_ERR_EMPTY_PARA_LIST);
        CHECK_SYMBOL(plst->car);
        if (plst->cdr->is_simple_obj())
            CHECK_SYMBOL(plst->cdr);
        else
            CHECK_PARA_LIST(plst->cdr);

        id = static_cast<SymObj*>(plst->car);
        EvalObj *params = plst->cdr;

        // Restore the next pointer
        pc->next = TO_PAIR(pc->cdr);

        Pair *body = TO_PAIR(TO_PAIR(pc->cdr)->cdr);   // Truncate the expression list

        if (body == empty_list)
            throw TokenError(name, SYN_ERR_MISS_OR_EXTRA_EXP);

        for (Pair *ptr = body; ptr != empty_list; ptr = TO_PAIR(ptr->cdr))
            ptr->next = NULL;           // Make each expression a orphan

        obj = new ProcObj(body, envt, params);
    }
    envt->add_binding(id, obj);
    *top_ptr++ = new UnspecObj();
    return ret_addr->next;
}

ReprCons *SpecialOptDefine::get_repr_cons() {
    return new ReprStr("#<Builtin Macro: define>");
}

void SpecialOptSet::prepare(Pair *pc) {
    if (!pc->cdr->is_pair_obj())
        throw TokenError(name, RUN_ERR_WRONG_NUM_OF_ARGS);

    Pair *first = TO_PAIR(pc->cdr), *second;

    if (!first->is_pair_obj())
        throw TokenError(name, RUN_ERR_WRONG_NUM_OF_ARGS);

    second = TO_PAIR(pc->cdr);
    if (second->cdr != empty_list)
        throw TokenError(name, RUN_ERR_WRONG_NUM_OF_ARGS);

    pc->next = second;
    second->next = NULL;
}

Pair *SpecialOptSet::call(Pair *args, Environment * &envt,
        Continuation * &cont, FrameObj ** &top_ptr) {
    Pair *ret_addr = static_cast<RetAddr*>(*top_ptr)->addr;
    Pair *pc = static_cast<Pair*>(ret_addr->car);
    EvalObj *first = TO_PAIR(pc->cdr)->car;

    if (!first->is_sym_obj())
        throw TokenError(first->ext_repr(), SYN_ERR_NOT_AN_ID);

    SymObj *id = static_cast<SymObj*>(first);

    bool flag = envt->add_binding(id, TO_PAIR(args->cdr)->car, false);
    if (!flag) throw TokenError(id->ext_repr(), RUN_ERR_UNBOUND_VAR);
    *top_ptr++ = new UnspecObj();
    return ret_addr->next;
}

SpecialOptSet::SpecialOptSet() : SpecialOptObj("set!") {}

ReprCons *SpecialOptSet::get_repr_cons() {
    return new ReprStr("#<Builtin Macro: set!>");
}

SpecialOptQuote::SpecialOptQuote() : SpecialOptObj("quote") {}

void SpecialOptQuote::prepare(Pair *pc) {
    // Do not evaluate anything
    CHECK_COM(pc);
    pc->next = NULL;
}

Pair *SpecialOptQuote::call(Pair *args, Environment * &envt,
        Continuation * &cont, FrameObj ** &top_ptr) {
    Pair *ret_addr = static_cast<RetAddr*>(*top_ptr)->addr;
    Pair *pc = static_cast<Pair*>(ret_addr->car);
    // revert
    pc->next = TO_PAIR(pc->cdr);
    *top_ptr++ = TO_PAIR(pc->cdr)->car;
    return ret_addr->next;
}

ReprCons *SpecialOptQuote::get_repr_cons() {
    return new ReprStr("#<Builtin Macro: quote>");
}

SpecialOptEval::SpecialOptEval() : SpecialOptObj("eval") {}

void SpecialOptEval::prepare(Pair *pc) {
    state = 0;
}

Pair *SpecialOptEval::call(Pair *args, Environment * &envt,
        Continuation * &cont, FrameObj ** &top_ptr) {
    if (args->cdr == empty_list ||
        TO_PAIR(args->cdr)->cdr != empty_list)
        throw TokenError(name, RUN_ERR_WRONG_NUM_OF_ARGS);
    Pair *ret_addr = static_cast<RetAddr*>(*top_ptr)->addr;
    if (state)
    {
        *top_ptr++ = TO_PAIR(args->cdr)->car;
        return ret_addr->next;          // Move to the next instruction
    }
    else
    {
        state = 1;
        top_ptr += 2;
        return TO_PAIR(args->cdr);
    }
}

ReprCons *SpecialOptEval::get_repr_cons() {
    return new ReprStr("#<Builtin Macro: eval>");
}

SpecialOptAnd::SpecialOptAnd() : SpecialOptObj("and") {}

void SpecialOptAnd::prepare(Pair *pc) {
    CHECK_COM(pc);
    if (pc->cdr != empty_list)
    {
        pc->next = TO_PAIR(pc->cdr);
        pc->next->next = NULL;
    }
}

Pair *SpecialOptAnd::call(Pair *args, Environment * &envt,
        Continuation * &cont, FrameObj ** &top_ptr) {
    Pair *ret_addr = static_cast<RetAddr*>(*top_ptr)->addr;
    Pair *pc = static_cast<Pair*>(ret_addr->car);
    if (args->cdr == empty_list)
    {
        *top_ptr++ = new BoolObj(true);
        return ret_addr->next;
    }
    EvalObj *ret = TO_PAIR(args->cdr)->car;
    if (ret->is_true())
    {
        if (pc->next->cdr == empty_list) // the last member
        {
            *top_ptr++ = ret;
            return ret_addr->next;
        }
        else
        {
            top_ptr += 2;
            pc->next = TO_PAIR(pc->next->cdr);
            pc->next->next = NULL;
            return pc->next;
        }
    }
    else
    {
        *top_ptr++ = ret;
        return ret_addr->next;
    }
    throw NormalError(INT_ERR);
}

ReprCons *SpecialOptAnd::get_repr_cons() {
    return new ReprStr("#<Builtin Macro: and>");
}

SpecialOptOr::SpecialOptOr() : SpecialOptObj("or") {}

void SpecialOptOr::prepare(Pair *pc) {
    CHECK_COM(pc);
    if (pc->cdr != empty_list)
    {
        pc->next = TO_PAIR(pc->cdr);
        pc->next->next = NULL;
    }
}

Pair *SpecialOptOr::call(Pair *args, Environment * &envt,
        Continuation * &cont, FrameObj ** &top_ptr) {
    Pair *ret_addr = static_cast<RetAddr*>(*top_ptr)->addr;
    Pair *pc = static_cast<Pair*>(ret_addr->car);
    if (args->cdr == empty_list)
    {
        *top_ptr++ = new BoolObj(false);
        return ret_addr->next;
    }
    EvalObj *ret = TO_PAIR(args->cdr)->car;
    if (!ret->is_true())
    {
        if (pc->next->cdr == empty_list) // the last member
        {
            *top_ptr++ = ret;
            return ret_addr->next;
        }
        else
        {
            top_ptr += 2;
            pc->next = TO_PAIR(pc->next->cdr);
            pc->next->next = NULL;
            return pc->next;
        }
    }
    else
    {
        *top_ptr++ = ret;
        return ret_addr->next;
    }
    throw NormalError(INT_ERR);
}

ReprCons *SpecialOptOr::get_repr_cons() {
    return new ReprStr("#<Builtin Macro: or>");
}

SpecialOptApply::SpecialOptApply() : SpecialOptObj("apply") {}

void SpecialOptApply::prepare(Pair *pc) {}

Pair *SpecialOptApply::call(Pair *args, Environment * &envt,
        Continuation * &cont, FrameObj ** &top_ptr) {
    top_ptr++;          // Recover the return address
    if (args->cdr == empty_list)
        throw TokenError(name, RUN_ERR_WRONG_NUM_OF_ARGS);

    args = TO_PAIR(args->cdr);
    if (!args->car->is_opt_obj())
        throw TokenError("an operator", RUN_ERR_WRONG_TYPE);

    *top_ptr++ = args->car;
    args = TO_PAIR(args->cdr);
    if (args == empty_list)
        throw TokenError(name, RUN_ERR_WRONG_NUM_OF_ARGS);

    for (; args->cdr != empty_list; args = TO_PAIR(args->cdr))
        *top_ptr++ = args->car;

    if (args->car != empty_list)
    {
        if (!args->car->is_pair_obj())
            throw TokenError("a list", RUN_ERR_WRONG_TYPE);

        args = TO_PAIR(args->car);
        EvalObj *nptr;
        for (;;)
        {
            *top_ptr++ = args->car;
            if ((nptr = args->cdr)->is_pair_obj())
                args = TO_PAIR(nptr);
            else break;
        }
        if (args->cdr != empty_list)
            throw TokenError("a list", RUN_ERR_WRONG_TYPE);
    }
    return NULL; // force the invocation
}

ReprCons *SpecialOptApply::get_repr_cons() {
    return new ReprStr("#<Builtin Macro: apply>");
}

SpecialOptForce::SpecialOptForce() : SpecialOptObj("force") {}

void SpecialOptForce::prepare(Pair *pc) {
    state = 0;
}

Pair *SpecialOptForce::call(Pair *args, Environment * &envt,
        Continuation * &cont, FrameObj ** &top_ptr) {
    if (args->cdr == empty_list ||
        TO_PAIR(args->cdr)->cdr != empty_list)
        throw TokenError(name, RUN_ERR_WRONG_NUM_OF_ARGS);
    args = TO_PAIR(args->cdr);

    Pair *ret_addr = static_cast<RetAddr*>(*top_ptr)->addr;
    if (state)
    {
        EvalObj *mem = args->car;
        prom->feed_mem(mem);
        *top_ptr++ = mem;
        return ret_addr->next;          // Move to the next instruction
    }
    else
    {
        if (!args->car->is_prom_obj())
            throw TokenError("a promise", RUN_ERR_WRONG_TYPE);
        prom = static_cast<PromObj*>(args->car);
        EvalObj *mem = prom->get_mem();
        if (mem)                        // fetch from memorized result
        {
            *top_ptr++ = mem;
            return ret_addr->next;      
        }
        else                            // force
        {
            state = 1;
            top_ptr += 2;
            return prom->get_entry();
        }
    }
}

ReprCons *SpecialOptForce::get_repr_cons() {
    return new ReprStr("#<Builtin Macro: force>");
}

SpecialOptDelay::SpecialOptDelay() : SpecialOptObj("delay") {}

void SpecialOptDelay::prepare(Pair *pc) {
    if (pc->cdr == empty_list ||
        TO_PAIR(pc->cdr)->cdr != empty_list)
        throw TokenError(name, RUN_ERR_WRONG_NUM_OF_ARGS);
    pc->next = NULL;
}

Pair *SpecialOptDelay::call(Pair *args, Environment * &envt,
        Continuation * &cont, FrameObj ** &top_ptr) {
    Pair *ret_addr = static_cast<RetAddr*>(*top_ptr)->addr;
    Pair *pc = static_cast<Pair*>(ret_addr->car);
    *top_ptr++ = new PromObj(TO_PAIR(pc->cdr)->car);
    return ret_addr->next;          // Move to the next instruction
}

ReprCons *SpecialOptDelay::get_repr_cons() {
    return new ReprStr("#<Builtin Macro: delay>");
}


BUILTIN_PROC_DEF(make_pair) {
    ARGS_EXACTLY_TWO;
    return new Pair(args->car, TO_PAIR(args->cdr)->car);
}

BUILTIN_PROC_DEF(pair_car) {
    ARGS_EXACTLY_ONE;
    if (!args->car->is_pair_obj())
        throw TokenError("pair", RUN_ERR_WRONG_TYPE);

    return TO_PAIR(args->car)->car;
}

BUILTIN_PROC_DEF(pair_cdr) {
    ARGS_EXACTLY_ONE;
    if (!args->car->is_pair_obj())
        throw TokenError("pair", RUN_ERR_WRONG_TYPE);

    return TO_PAIR(args->car)->cdr;
}


BUILTIN_PROC_DEF(make_list) {
    return args;
}

BUILTIN_PROC_DEF(num_add) {
//    ARGS_AT_LEAST_ONE;
    NumObj *res = new IntNumObj(0), *opr; // the most accurate type
    for (;args != empty_list; args = TO_PAIR(args->cdr))
    {
        if (!args->car->is_num_obj())        // not a number
            throw TokenError("a number", RUN_ERR_WRONG_TYPE);
        opr = static_cast<NumObj*>(args->car);
        NumObj *_res = res;
        if (_res->level < opr->level)
            opr = _res->convert(opr);
        else
            _res =  opr->convert(_res);
        res = _res->add(opr);
    }
    return res;
}

BUILTIN_PROC_DEF(num_sub) {
    ARGS_AT_LEAST_ONE;
    if (!args->car->is_num_obj())
        throw TokenError("a number", RUN_ERR_WRONG_TYPE);

    NumObj *res = static_cast<NumObj*>(args->car), *opr;
    args = TO_PAIR(args->cdr);
    if (args == empty_list)
    {
        IntNumObj _zero(0);
        NumObj *zero = res->convert(&_zero);
        return zero->sub(res);
    }
    for (; args != empty_list; args = TO_PAIR(args->cdr))
    {
        if (!args->car->is_num_obj())        // not a number
            throw TokenError("a number", RUN_ERR_WRONG_TYPE);
        opr = static_cast<NumObj*>(args->car);
        // upper type conversion
        NumObj *_res = res;
        if (_res->level < opr->level)
            opr = _res->convert(opr);
        else
            _res =  opr->convert(_res);
        res = _res->sub(opr);
    }
    return res;
}


BUILTIN_PROC_DEF(num_mul) {
//    ARGS_AT_LEAST_ONE;
    NumObj *res = new IntNumObj(1), *opr; // the most accurate type
    for (;args != empty_list; args = TO_PAIR(args->cdr))
    {
        if (!args->car->is_num_obj())        // not a number
            throw TokenError("a number", RUN_ERR_WRONG_TYPE);
        opr = static_cast<NumObj*>(args->car);
        NumObj *_res = res;
        if (_res->level < opr->level)
            opr = _res->convert(opr);
        else
            _res =  opr->convert(_res);
        res = _res->mul(opr);
    }
    return res;
}

BUILTIN_PROC_DEF(num_div) {
    ARGS_AT_LEAST_ONE;
    if (!args->car->is_num_obj())
        throw TokenError("a number", RUN_ERR_WRONG_TYPE);
    NumObj *res = static_cast<NumObj*>(args->car), *opr;
    args = TO_PAIR(args->cdr);
    if (args == empty_list)
    {
        IntNumObj _one(1);
        NumObj *one = res->convert(&_one);
        return one->div(res);
    }
    for (; args != empty_list; args = TO_PAIR(args->cdr))
    {
        if (!args->car->is_num_obj())        // not a number
            throw TokenError("a number", RUN_ERR_WRONG_TYPE);
        opr = static_cast<NumObj*>(args->car);
        // upper type conversion
        NumObj *_res = res;
        if (_res->level < opr->level)
            opr = _res->convert(opr);
        else
            _res =  opr->convert(_res);
        res = _res->div(opr);
    }
    return res;
}

BUILTIN_PROC_DEF(num_le) {
    if (args == empty_list)
        return new BoolObj(true);
    // zero arguments
    if (!args->car->is_num_obj())
        throw TokenError("a number", RUN_ERR_WRONG_TYPE);

    NumObj *last = static_cast<NumObj*>(args->car), *opr;
    args = TO_PAIR(args->cdr);
    for (; args != empty_list; args = TO_PAIR(args->cdr), last = opr)
    {
        if (!args->car->is_num_obj())        // not a number
            throw TokenError("a number", RUN_ERR_WRONG_TYPE);
        opr = static_cast<NumObj*>(args->car);
        // upper type conversion
        if (last->level < opr->level)
            opr = last->convert(opr);
        else
            last = opr->convert(last);
        if (!last->le(opr))
            return new BoolObj(false);
    }
    return new BoolObj(true);
}

BUILTIN_PROC_DEF(num_ge) {
    if (args == empty_list)
        return new BoolObj(true);
    // zero arguments
    if (!args->car->is_num_obj())
        throw TokenError("a number", RUN_ERR_WRONG_TYPE);

    NumObj *last = static_cast<NumObj*>(args->car), *opr;
    args = TO_PAIR(args->cdr);
    for (; args != empty_list; args = TO_PAIR(args->cdr), last = opr)
    {
        if (!args->car->is_num_obj())        // not a number
            throw TokenError("a number", RUN_ERR_WRONG_TYPE);
        opr = static_cast<NumObj*>(args->car);
        // upper type conversion
        if (last->level < opr->level)
            opr = last->convert(opr);
        else
            last = opr->convert(last);
        if (!last->ge(opr))
            return new BoolObj(false);
    }
    return new BoolObj(true);
}


BUILTIN_PROC_DEF(num_lt) {
    if (args == empty_list)
        return new BoolObj(true);
    // zero arguments
    if (!args->car->is_num_obj())
        throw TokenError("a number", RUN_ERR_WRONG_TYPE);

    NumObj *last = static_cast<NumObj*>(args->car), *opr;
    args = TO_PAIR(args->cdr);
    for (; args != empty_list; args = TO_PAIR(args->cdr), last = opr)
    {
        if (!args->car->is_num_obj())        // not a number
            throw TokenError("a number", RUN_ERR_WRONG_TYPE);
        opr = static_cast<NumObj*>(args->car);
        // upper type conversion
        if (last->level < opr->level)
            opr = last->convert(opr);
        else
            last = opr->convert(last);
        if (!last->lt(opr))
            return new BoolObj(false);
    }
    return new BoolObj(true);
}

BUILTIN_PROC_DEF(num_gt) {
    if (args == empty_list)
        return new BoolObj(true);
    // zero arguments
    if (!args->car->is_num_obj())
        throw TokenError("a number", RUN_ERR_WRONG_TYPE);

    NumObj *last = static_cast<NumObj*>(args->car), *opr;
    args = TO_PAIR(args->cdr);
    for (; args != empty_list; args = TO_PAIR(args->cdr), last = opr)
    {
        if (!args->car->is_num_obj())        // not a number
            throw TokenError("a number", RUN_ERR_WRONG_TYPE);
        opr = static_cast<NumObj*>(args->car);
        // upper type conversion
        if (last->level < opr->level)
            opr = last->convert(opr);
        else
            last = opr->convert(last);
        if (!last->gt(opr))
            return new BoolObj(false);
    }
    return new BoolObj(true);
}

BUILTIN_PROC_DEF(num_eq) {
    if (args == empty_list)
        return new BoolObj(true);
    // zero arguments
    if (!args->car->is_num_obj())
        throw TokenError("a number", RUN_ERR_WRONG_TYPE);

    NumObj *last = static_cast<NumObj*>(args->car), *opr;
    args = TO_PAIR(args->cdr);
    for (; args != empty_list; args = TO_PAIR(args->cdr), last = opr)
    {
        if (!args->car->is_num_obj())        // not a number
            throw TokenError("a number", RUN_ERR_WRONG_TYPE);
        opr = static_cast<NumObj*>(args->car);
        // upper type conversion
        if (last->level < opr->level)
            opr = last->convert(opr);
        else
            last = opr->convert(last);
        if (!last->eq(opr))
            return new BoolObj(false);
    }
    return new BoolObj(true);
}

BUILTIN_PROC_DEF(bool_not) {
    ARGS_EXACTLY_ONE;
    return new BoolObj(!args->car->is_true());
}

BUILTIN_PROC_DEF(is_boolean) {
    ARGS_EXACTLY_ONE;
    return new BoolObj(args->car->is_bool_obj());
}

BUILTIN_PROC_DEF(is_pair) {
    ARGS_EXACTLY_ONE;
    return new BoolObj(args->car->is_pair_obj());
}

BUILTIN_PROC_DEF(pair_set_car) {
    ARGS_EXACTLY_TWO;
    if (!args->car->is_pair_obj())
        throw TokenError("pair", RUN_ERR_WRONG_TYPE);
    TO_PAIR(args->car)->car = TO_PAIR(args->cdr)->car;
    return new UnspecObj();
}

BUILTIN_PROC_DEF(pair_set_cdr) {
    ARGS_EXACTLY_TWO;
    if (!args->car->is_pair_obj())
        throw TokenError("pair", RUN_ERR_WRONG_TYPE);
    TO_PAIR(args->car)->cdr = TO_PAIR(args->cdr)->car;
    return new UnspecObj();
}

BUILTIN_PROC_DEF(is_null) {
    ARGS_EXACTLY_ONE;
    return new BoolObj(args->car == empty_list);
}

BUILTIN_PROC_DEF(is_list) {
    ARGS_EXACTLY_ONE;
    if (args->car == empty_list)
        return new BoolObj(true);
    if (!args->car->is_pair_obj())
        return new BoolObj(false);
    args = TO_PAIR(args->car);
    EvalObj *nptr;
    for (;;)
    {
        if ((nptr = args->cdr)->is_pair_obj())
            args = TO_PAIR(nptr);
        else break;
    }
    return new BoolObj(args->cdr == empty_list);
}

BUILTIN_PROC_DEF(num_is_exact) {
    ARGS_EXACTLY_ONE;
    if (!args->car->is_num_obj())
        throw TokenError("a number", RUN_ERR_WRONG_TYPE);
    return new BoolObj(static_cast<NumObj*>(args->car)->is_exact());
}

BUILTIN_PROC_DEF(num_is_inexact) {
    ARGS_EXACTLY_ONE;
    if (!args->car->is_num_obj())
        throw TokenError("a number", RUN_ERR_WRONG_TYPE);
    return new BoolObj(!static_cast<NumObj*>(args->car)->is_exact());
}

BUILTIN_PROC_DEF(length) {
    ARGS_EXACTLY_ONE;
    if (args->car == empty_list)
        return new IntNumObj(mpz_class(0));
    if (!args->car->is_pair_obj())
        throw TokenError("a list", RUN_ERR_WRONG_TYPE);
    int num = 0;
    EvalObj *nptr;
    for (args = TO_PAIR(args->car);;)
    {
        num++;
        if ((nptr = args->cdr)->is_pair_obj())
            args = TO_PAIR(nptr);
        else
            break;
    }
    if (args->cdr != empty_list)
        throw TokenError("a list", RUN_ERR_WRONG_TYPE);
    return new IntNumObj(mpz_class(num));
}

Pair *copy_list(Pair *src, EvalObj * &tail) {
    if (src == empty_list)
        throw NormalError(INT_ERR);
    EvalObj* nptr;
    Pair head(NULL, NULL);
    tail = &head;
    for (;;)
    {
        TO_PAIR(tail)->cdr = new Pair(*src);
        tail = TO_PAIR(TO_PAIR(tail)->cdr);
        if ((nptr = src->cdr)->is_pair_obj())
            src = TO_PAIR(nptr);
        else break;
    }
    return TO_PAIR(head.cdr);
}

BUILTIN_PROC_DEF(append) {
    EvalObj *tail = empty_list, *head = tail;
    for (; args != empty_list; args = TO_PAIR(args->cdr))
    {
        if (tail == empty_list)
        {
            head = args->car;
            if (head->is_pair_obj())
                head = copy_list(TO_PAIR(head), tail);
            else tail = head;
        }
        else
        {
            if (tail->is_pair_obj())
            {
                Pair *prev = TO_PAIR(tail);
                if (prev->cdr != empty_list)
                    throw TokenError("empty list", RUN_ERR_WRONG_TYPE);
                if (args->car->is_pair_obj())
                    prev->cdr = copy_list(TO_PAIR(args->car), tail);
                else
                    prev->cdr = args->car;
            }
            else
                throw TokenError("a pair", RUN_ERR_WRONG_TYPE);
        }
    }
    return head;
}

BUILTIN_PROC_DEF(reverse) {
    ARGS_EXACTLY_ONE;
    Pair *tail = empty_list;
    EvalObj *ptr;
    for (ptr = args->car;
            ptr->is_pair_obj(); ptr = TO_PAIR(ptr)->cdr)
        tail = new Pair(TO_PAIR(ptr)->car, tail);
    if (ptr != empty_list)
        throw TokenError("a list", RUN_ERR_WRONG_TYPE);
    return tail;
}

BUILTIN_PROC_DEF(list_tail) {
    ARGS_EXACTLY_TWO;
    EvalObj *sec = TO_PAIR(args->cdr)->car;
    if (!sec->is_num_obj() ||
            static_cast<NumObj*>(sec)->level != NUM_LVL_INT)
        throw TokenError("an exact integer", RUN_ERR_WRONG_TYPE);
    int i, k = static_cast<IntNumObj*>(sec)->get_i();
    if (k < 0)
        throw TokenError("a non-negative integer", RUN_ERR_WRONG_TYPE);
    EvalObj *ptr;
    for (i = 0, ptr = args->car;
            ptr->is_pair_obj(); ptr = TO_PAIR(ptr)->cdr, i++)
        if (i == k) break;
    if (i != k)
        throw TokenError("a pair", RUN_ERR_WRONG_TYPE);
    EvalObj *tail;
    if (ptr->is_pair_obj())
        return copy_list(TO_PAIR(ptr), tail);
    else
        return ptr;
}

BUILTIN_PROC_DEF(is_eqv) {
    ARGS_EXACTLY_TWO;
    EvalObj *obj1 = args->car;
    EvalObj *obj2 = TO_PAIR(args->cdr)->car;
    int otype = obj1->get_otype();

    if (otype != obj2->get_otype()) return new BoolObj(false);
    if (otype & CLS_BOOL_OBJ)
        return new BoolObj(
            static_cast<BoolObj*>(obj1)->val ==
            static_cast<BoolObj*>(obj2)->val);
    if (otype & CLS_SYM_OBJ)
        return new BoolObj(
            static_cast<SymObj*>(obj1)->val ==
            static_cast<SymObj*>(obj2)->val);
    if (otype & CLS_NUM_OBJ)
    {
        NumObj *num1 = static_cast<NumObj*>(obj1);
        NumObj *num2 = static_cast<NumObj*>(obj2);
        if (num1->is_exact() != num2->is_exact())
            return new BoolObj(false);
        if (num1->level < num2->level)
            return new BoolObj(num1->eq(num1->convert(num2)));
        else
            return new BoolObj(num2->eq(num2->convert(num1)));
    }
    if (otype & CLS_CHAR_OBJ)
        return new BoolObj(
            static_cast<CharObj*>(obj1)->ch ==
            static_cast<CharObj*>(obj2)->ch);    // (char=?)
    return new BoolObj(obj1 == obj2);
}


BUILTIN_PROC_DEF(is_equal) {

//#define INC1(x) (++(x) == t1 ? (x) = q1:0)
//#define INC2(x) (++(x) == t2 ? (x) = q2:0)
#define INC1(x) (++(x))
#define INC2(x) (++(x))
#define CHK1 \
do { \
    if (r1 == q1 + EQUAL_QUEUE_SIZE)  \
        throw NormalError(RUN_ERR_QUEUE_OVERFLOW); \
} while (0)

#define CHK2 \
do { \
    if (r2 == q2 + EQUAL_QUEUE_SIZE)  \
        throw NormalError(RUN_ERR_QUEUE_OVERFLOW); \
} while (0)


    static EvalObj *q1[EQUAL_QUEUE_SIZE], *q2[EQUAL_QUEUE_SIZE];

    ARGS_EXACTLY_TWO;
    EvalObj **l1 = q1, **r1 = l1;
    EvalObj **l2 = q2, **r2 = l2;

    *r1++ = args->car;
    *r2++ = TO_PAIR(args->cdr)->car;

    EvalObj *a, *b;
    for (; l1 != r1; INC1(l1), INC2(l2))
    {
        // Different types
        int otype = (a = *l1)->get_otype();
        if (otype != (b = *l2)->get_otype())
            return new BoolObj(false);
        if (a != empty_list && b != empty_list &&
            otype & CLS_PAIR_OBJ)
        {
            *r1 = TO_PAIR(a)->car;
            INC1(r1);
            CHK1;
            *r1 = TO_PAIR(a)->cdr;
            INC1(r1);
            CHK1;

            *r2 = TO_PAIR(b)->car;
            INC2(r2);
            CHK2;
            *r2 = TO_PAIR(b)->cdr;
            INC2(r2);
            CHK2;
        }
        else if (otype & CLS_VECT_OBJ)
        {
            VecObj *va = static_cast<VecObj*>(a);
            VecObj *vb = static_cast<VecObj*>(b);
            if (va->get_size() != vb->get_size())
                return new BoolObj(false);
            for (EvalObjVec::iterator
                    it = va->vec.begin();
                    it != va->vec.end(); it++)
            {
                *r1 = *it;
                INC1(r1);
                CHK1;
            }

            for (EvalObjVec::iterator
                    it = vb->vec.begin();
                    it != vb->vec.end(); it++)
            {
                *r2 = *it;
                INC2(r2);
                CHK2;
            }
        }
        else if (otype & CLS_BOOL_OBJ)
        {
            if (static_cast<BoolObj*>(a)->val !=
                static_cast<BoolObj*>(b)->val)
                return new BoolObj(false);
        }
        else if (otype & CLS_SYM_OBJ)
        {
            if (static_cast<SymObj*>(a)->val !=
                static_cast<SymObj*>(b)->val)
                return new BoolObj(false);
        }
        else if (otype & CLS_NUM_OBJ)
        {
            NumObj *num1 = static_cast<NumObj*>(a);
            NumObj *num2 = static_cast<NumObj*>(b);
            if (num1->is_exact() != num2->is_exact())
            return new BoolObj(false);
            if (num1->level < num2->level)
            {
                if (!num1->eq(num1->convert(num2)))
                    return new BoolObj(false);
            }
            else
            {
                if (!num2->eq(num2->convert(num1)))
                    return new BoolObj(false);
            }
        }
        else if (otype & CLS_CHAR_OBJ)
        {
            if (static_cast<CharObj*>(a)->ch !=
                static_cast<CharObj*>(b)->ch)
                return new BoolObj(false); // (char=?)
        }
        else if (otype & CLS_STR_OBJ)
        {
            if (static_cast<StrObj*>(a)->str !=
                static_cast<StrObj*>(b)->str)
                return new BoolObj(false); // (string=?)
        }
        else if (a != b)
            return new BoolObj(false);
    }
    return new BoolObj(true);
}

BUILTIN_PROC_DEF(is_number) {
    ARGS_EXACTLY_ONE;
    return new BoolObj(args->car->is_num_obj());
}

BUILTIN_PROC_DEF(is_complex) {
    ARGS_EXACTLY_ONE;
    return new BoolObj(args->car->is_num_obj());
    // any numbers are complex 
}


BUILTIN_PROC_DEF(is_real) {
    ARGS_EXACTLY_ONE;
    if (!args->car->is_num_obj())
        return new BoolObj(false);
    NumObj *obj = static_cast<NumObj*>(args->car);
    if (obj->level >= NUM_LVL_REAL) 
        return new BoolObj(true);
    return new BoolObj(is_zero(static_cast<CompNumObj*>(obj)->imag));
}

BUILTIN_PROC_DEF(is_rational) {
    ARGS_EXACTLY_ONE;
    return new BoolObj(args->car->is_num_obj() && 
                    static_cast<NumObj*>(args->car)->level >= NUM_LVL_RAT);
}

BUILTIN_PROC_DEF(is_integer) {
    ARGS_EXACTLY_ONE;
    return new BoolObj(args->car->is_num_obj() && 
                    static_cast<NumObj*>(args->car)->level >= NUM_LVL_INT);
}

BUILTIN_PROC_DEF(num_abs) {
    ARGS_EXACTLY_ONE;
    if (!args->car->is_num_obj())
        throw TokenError("a number", RUN_ERR_WRONG_TYPE);
    return static_cast<NumObj*>(args->car)->abs();
}

BUILTIN_PROC_DEF(num_mod) {
    ARGS_EXACTLY_TWO;
    NumObj* a = static_cast<NumObj*>(args->car);
    NumObj* b = static_cast<NumObj*>(TO_PAIR(args->cdr)->car);
    if (a->level != NUM_LVL_INT || b->level != NUM_LVL_INT)
        throw TokenError("an integer", RUN_ERR_WRONG_TYPE);
    return static_cast<IntNumObj*>(a)->mod(b);
}

BUILTIN_PROC_DEF(num_rem) {
    ARGS_EXACTLY_TWO;
    NumObj* a = static_cast<NumObj*>(args->car);
    NumObj* b = static_cast<NumObj*>(TO_PAIR(args->cdr)->car);
    if (a->level != NUM_LVL_INT || b->level != NUM_LVL_INT)
        throw TokenError("an integer", RUN_ERR_WRONG_TYPE);
    return static_cast<IntNumObj*>(a)->rem(b);
}

BUILTIN_PROC_DEF(num_quo) {
    ARGS_EXACTLY_TWO;
    NumObj* a = static_cast<NumObj*>(args->car);
    NumObj* b = static_cast<NumObj*>(TO_PAIR(args->cdr)->car);
    if (a->level != NUM_LVL_INT || b->level != NUM_LVL_INT)
        throw TokenError("an integer", RUN_ERR_WRONG_TYPE);
    return static_cast<IntNumObj*>(a)->quo(b);
}

BUILTIN_PROC_DEF(num_gcd) {
//    ARGS_AT_LEAST_ONE;
    NumObj *res = new IntNumObj(0);
    IntNumObj *opr; 
    for (;args != empty_list; args = TO_PAIR(args->cdr))
    {
        if (!args->car->is_num_obj())        // not a number
            throw TokenError("a number", RUN_ERR_WRONG_TYPE);
        if (static_cast<NumObj*>(args->car)->level != NUM_LVL_INT)        // not a number
            throw TokenError("an integer", RUN_ERR_WRONG_TYPE);

        opr = static_cast<IntNumObj*>(args->car);
        res = opr->gcd(res);
    }
    return res;
}

BUILTIN_PROC_DEF(num_lcm) {
//    ARGS_AT_LEAST_ONE;
    NumObj *res = new IntNumObj(1);
    IntNumObj *opr; 
    for (;args != empty_list; args = TO_PAIR(args->cdr))
    {
        if (!args->car->is_num_obj())        // not a number
            throw TokenError("a number", RUN_ERR_WRONG_TYPE);
        if (static_cast<NumObj*>(args->car)->level != NUM_LVL_INT)        // not a number
            throw TokenError("an integer", RUN_ERR_WRONG_TYPE);

        opr = static_cast<IntNumObj*>(args->car);
        res = opr->lcm(res);
    }
    return res;
}

BUILTIN_PROC_DEF(is_string) {
    ARGS_AT_LEAST_ONE;
    return new BoolObj(args->car->is_str_obj());
}

BUILTIN_PROC_DEF(is_symbol) {
    ARGS_AT_LEAST_ONE;
    return new BoolObj(args->car->is_sym_obj());
}

BUILTIN_PROC_DEF(string_lt) {
    ARGS_EXACTLY_TWO;
    EvalObj *obj1 = args->car;
    EvalObj *obj2 = TO_PAIR(args->cdr)->car;
    if (!obj1->is_str_obj() || !obj2->is_str_obj())
        throw TokenError("a string", RUN_ERR_WRONG_TYPE);
    return new BoolObj(static_cast<StrObj*>(obj1)->lt(static_cast<StrObj*>(obj2)));
}

BUILTIN_PROC_DEF(string_le) {
    ARGS_EXACTLY_TWO;
    EvalObj *obj1 = args->car;
    EvalObj *obj2 = TO_PAIR(args->cdr)->car;
    if (!obj1->is_str_obj() || !obj2->is_str_obj())
        throw TokenError("a string", RUN_ERR_WRONG_TYPE);
    return new BoolObj(static_cast<StrObj*>(obj1)->le(static_cast<StrObj*>(obj2)));
}

BUILTIN_PROC_DEF(string_gt) {
    ARGS_EXACTLY_TWO;
    EvalObj *obj1 = args->car;
    EvalObj *obj2 = TO_PAIR(args->cdr)->car;
    if (!obj1->is_str_obj() || !obj2->is_str_obj())
        throw TokenError("a string", RUN_ERR_WRONG_TYPE);
    return new BoolObj(static_cast<StrObj*>(obj1)->lt(static_cast<StrObj*>(obj2)));
}

BUILTIN_PROC_DEF(string_ge) {
    ARGS_EXACTLY_TWO;
    EvalObj *obj1 = args->car;
    EvalObj *obj2 = TO_PAIR(args->cdr)->car;
    if (!obj1->is_str_obj() || !obj2->is_str_obj())
        throw TokenError("a string", RUN_ERR_WRONG_TYPE);
    return new BoolObj(static_cast<StrObj*>(obj1)->le(static_cast<StrObj*>(obj2)));
}

BUILTIN_PROC_DEF(string_eq) {
    ARGS_EXACTLY_TWO;
    EvalObj *obj1 = args->car;
    EvalObj *obj2 = TO_PAIR(args->cdr)->car;
    if (!obj1->is_str_obj() || !obj2->is_str_obj())
        throw TokenError("a string", RUN_ERR_WRONG_TYPE);
    return new BoolObj(static_cast<StrObj*>(obj1)->eq(static_cast<StrObj*>(obj2)));
}


BUILTIN_PROC_DEF(display) {
    ARGS_EXACTLY_ONE;
    printf("%s\n", args->car->ext_repr().c_str());
    return new UnspecObj();
}
