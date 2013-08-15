#include <cstdio>
#include <cctype>
#include <cstdlib>

#include "consts.h"
#include "builtin.h"
#include "model.h"
#include "exc.h"
#include "types.h"
#include "gc.h"

using std::stringstream;

extern EmptyList *empty_list;
extern UnspecObj *unspec_obj;



SpecialOptIf::SpecialOptIf() : SpecialOptObj("if") {}

void SpecialOptIf::prepare(Pair *pc) {
#define IF_EXP_ERR \
        throw TokenError(name, RUN_ERR_WRONG_NUM_OF_ARGS)
    
    Pair *first, *second, *third;

    if (pc->cdr == empty_list)
        IF_EXP_ERR;
    first = TO_PAIR(pc->cdr);

    if (first->cdr == empty_list)
        IF_EXP_ERR;
    second = TO_PAIR(first->cdr);

    if (second->cdr != empty_list)
    {
        third = TO_PAIR(second->cdr);
        if (third->cdr != empty_list) IF_EXP_ERR;
    }
    pc->next = NULL;
}

Pair *SpecialOptIf::call(Pair *args, Environment * &lenvt,
        Continuation * &cont, EvalObj ** &top_ptr, Pair *pc) {
    Pair *ret_addr = cont->pc;
    if (cont->state)
    {
        if (cont->state == empty_list)
        {
            gc.expose(*top_ptr);
            *top_ptr++ = gc.attach(TO_PAIR(args->cdr)->car);
            EXIT_CURRENT_EXEC(lenvt, cont);
            gc.expose(args);
            return ret_addr->next;          // Move to the next instruction
        }
        else 
        {
            Pair *first = TO_PAIR(pc->cdr);
            Pair *second = TO_PAIR(first->cdr);
            Pair *third = TO_PAIR(second->cdr);

            if (TO_PAIR(args->cdr)->car->is_true())
            {
                second->next = NULL;
                // Undo pop and invoke again
                gc.attach(static_cast<EvalObj*>(*(++top_ptr)));
                top_ptr++;
                cont->state = empty_list;
                gc.expose(args);
                return second;
            }
            else if (third != empty_list)
            {
                third->next = NULL;
                // Undo pop and invoke again
                gc.attach(static_cast<EvalObj*>(*(++top_ptr)));
                top_ptr++;
                cont->state = empty_list;
                gc.expose(args);
                return third;
            }
            else
            {
                gc.expose(*top_ptr);
                *top_ptr++ = gc.attach(unspec_obj);
                EXIT_CURRENT_EXEC(lenvt, cont);
                gc.expose(args);
                return ret_addr->next;
            }
        }
    }
    else
    {
        gc.attach(static_cast<EvalObj*>(*(++top_ptr)));
        top_ptr++;
        cont->state = TO_PAIR(pc->cdr);
        cont->state->next = NULL;
        gc.expose(args);
        return cont->state;
    }
    throw NormalError(INT_ERR);
}

#define CHECK_SYMBOL(ptr) \
do \
{ \
    if (!(ptr)->is_sym_obj()) \
        throw TokenError("a symbol", RUN_ERR_WRONG_TYPE); \
} while (0)

#define CHECK_NUMBER(ptr) \
do \
{ \
    if (!(ptr)->is_num_obj()) \
        throw TokenError("a number", RUN_ERR_WRONG_TYPE); \
} while (0)

#define CHECK_EXACT(ptr) \
do \
{ \
    if (!(static_cast<NumObj*>(ptr))->is_exact()) \
        throw TokenError("an integer", RUN_ERR_WRONG_TYPE); \
} while (0)

#define CHECK_PARA_LIST(p) \
do  \
{ \
    if (p == empty_list) break; \
    EvalObj *nptr; \
    Pair *ptr; \
    for (ptr = TO_PAIR(p);;)  \
    { \
        CHECK_SYMBOL(ptr->car); \
        if ((nptr = ptr->cdr)->is_pair_obj()) \
            ptr = TO_PAIR(nptr); \
        else break; \
    } \
    if (ptr->cdr != empty_list) \
       CHECK_SYMBOL(ptr->cdr); \
} \
while (0)

SpecialOptLambda::SpecialOptLambda() : SpecialOptObj("lambda") {}

void SpecialOptLambda::prepare(Pair *pc) {
    // Do not evaluate anything
    pc->next = NULL;
}

Pair *SpecialOptLambda::call(Pair *args, Environment * &lenvt,
        Continuation * &cont, EvalObj ** &top_ptr, Pair *pc) {

    Pair *ret_addr = cont->pc;

    if (pc->cdr == empty_list)
        throw TokenError(name, SYN_ERR_EMPTY_PARA_LIST);
    Pair *first = TO_PAIR(pc->cdr);

    EvalObj *params = first->car;
    // store a list of expressions inside <body>
    Pair *body = TO_PAIR(first->cdr);       // Truncate the expression list

    // Check <body>
    if (body == empty_list)
        throw TokenError(name, SYN_ERR_MISS_OR_EXTRA_EXP);
    // Check parameters
    if (params->is_simple_obj())
        CHECK_SYMBOL(first->car);
    else
        CHECK_PARA_LIST(first->car);

    for (Pair *ptr = body; ptr != empty_list; ptr = TO_PAIR(ptr->cdr))
        ptr->next = NULL;    // Make each expression isolated

    gc.expose(*top_ptr);
    *top_ptr++ = gc.attach(new ProcObj(body, lenvt, params));
    EXIT_CURRENT_EXEC(lenvt, cont);
    gc.expose(args);
    return ret_addr->next;  // Move to the next instruction
}

SpecialOptDefine::SpecialOptDefine() : SpecialOptObj("define") {}

void SpecialOptDefine::prepare(Pair *pc) {
    Pair *first, *second;
    if (pc->cdr == empty_list)
        throw TokenError(name, RUN_ERR_WRONG_NUM_OF_ARGS);
    first = TO_PAIR(pc->cdr);

    if (first->car->is_simple_obj())  // Simple value assignment
    {
        if (first->cdr == empty_list)
            throw TokenError(name, RUN_ERR_WRONG_NUM_OF_ARGS);
        second = TO_PAIR(first->cdr);
        if (second->cdr != empty_list)
            throw TokenError(name, RUN_ERR_WRONG_NUM_OF_ARGS);
    }                                           // Procedure definition
    pc->next = NULL;
}

Pair *SpecialOptDefine::call(Pair *args, Environment * &lenvt,
        Continuation * &cont, EvalObj ** &top_ptr, Pair *pc) {
    Pair *ret_addr = cont->pc;
    EvalObj *obj;
    SymObj *id;
    EvalObj *first = TO_PAIR(pc->cdr)->car;
    if (first->is_simple_obj())
    {
        if (!cont->state)
        {
            gc.attach(static_cast<EvalObj*>(*(++top_ptr)));
            top_ptr++;
            cont->state = TO_PAIR(TO_PAIR(pc->cdr)->cdr);
            cont->state->next = NULL;
            gc.expose(args);
            return cont->state;
        }
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
        id = static_cast<SymObj*>(plst->car);

        EvalObj *params = plst->cdr;
        Pair *body = TO_PAIR(TO_PAIR(pc->cdr)->cdr);

        // Check <body>
        if (body == empty_list)
            throw TokenError(name, SYN_ERR_MISS_OR_EXTRA_EXP);
        // Check parameters
        if (params->is_simple_obj())
            CHECK_SYMBOL(plst->cdr);
        else
            CHECK_PARA_LIST(plst->cdr);

        for (Pair *ptr = body; ptr != empty_list; ptr = TO_PAIR(ptr->cdr))
            ptr->next = NULL;           // Make each expression a orphan

        obj = new ProcObj(body, lenvt, params);
    }
    lenvt->add_binding(id, obj);
    gc.expose(*top_ptr);
    *top_ptr++ = gc.attach(unspec_obj);
    EXIT_CURRENT_EXEC(lenvt, cont);
    gc.expose(args);
    return ret_addr->next;
}

SpecialOptSet::SpecialOptSet() : SpecialOptObj("set!") {}

void SpecialOptSet::prepare(Pair *pc) {
    Pair *first, *second;
    if (pc->cdr == empty_list)
        throw TokenError(name, RUN_ERR_WRONG_NUM_OF_ARGS);
    first = TO_PAIR(pc->cdr);

    if (first->cdr == empty_list)
        throw TokenError(name, RUN_ERR_WRONG_NUM_OF_ARGS);
    second = TO_PAIR(first->cdr);

    if (second->cdr != empty_list)
        throw TokenError(name, RUN_ERR_WRONG_NUM_OF_ARGS);

    pc->next = NULL;
}

Pair *SpecialOptSet::call(Pair *args, Environment * &lenvt,
        Continuation * &cont, EvalObj ** &top_ptr, Pair *pc) {
    Pair *ret_addr = cont->pc;
    EvalObj *first = TO_PAIR(pc->cdr)->car;

    if (!cont->state)
    {
        gc.attach(static_cast<EvalObj*>(*(++top_ptr)));
        top_ptr++;
        cont->state = TO_PAIR(TO_PAIR(pc->cdr)->cdr);
        cont->state->next = NULL;
        gc.expose(args);
        return cont->state;
    }

    if (!first->is_sym_obj())
        throw TokenError(first->ext_repr(), SYN_ERR_NOT_AN_ID);

    SymObj *id = static_cast<SymObj*>(first);

    bool flag = lenvt->add_binding(id, TO_PAIR(args->cdr)->car, false);
    if (!flag) throw TokenError(id->ext_repr(), RUN_ERR_UNBOUND_VAR);
    gc.expose(*top_ptr);
    *top_ptr++ = gc.attach(unspec_obj);
    EXIT_CURRENT_EXEC(lenvt, cont);
    gc.expose(args);
    return ret_addr->next;
}

SpecialOptQuote::SpecialOptQuote() : SpecialOptObj("quote") {}

void SpecialOptQuote::prepare(Pair *pc) {
    // Do not evaluate anything
    pc->next = NULL;
}

Pair *SpecialOptQuote::call(Pair *args, Environment * &lenvt,
        Continuation * &cont, EvalObj ** &top_ptr, Pair *pc) {
    Pair *ret_addr = cont->pc;
    gc.expose(*top_ptr);
    *top_ptr++ = gc.attach(TO_PAIR(pc->cdr)->car);
    EXIT_CURRENT_EXEC(lenvt, cont);
    gc.expose(args);
    return ret_addr->next;
}

SpecialOptEval::SpecialOptEval() : SpecialOptObj("eval") {}

void SpecialOptEval::prepare(Pair *pc) {
    if (pc->cdr == empty_list ||
        TO_PAIR(pc->cdr)->cdr != empty_list)
        throw TokenError(name, RUN_ERR_WRONG_NUM_OF_ARGS);
}

Pair *SpecialOptEval::call(Pair *args, Environment * &lenvt,
        Continuation * &cont, EvalObj ** &top_ptr, Pair *pc) {
    Pair *ret_addr = cont->pc;
    if (cont->state)
    {
        gc.expose(cont->state);     // Exec done
        gc.expose(*top_ptr);
        *top_ptr++ = gc.attach(TO_PAIR(args->cdr)->car);
        EXIT_CURRENT_EXEC(lenvt, cont);
        gc.expose(args);
        return ret_addr->next;          // Move to the next instruction
    }
    else
    {
        gc.attach(static_cast<EvalObj*>(*(++top_ptr)));
        top_ptr++;
        cont->state = TO_PAIR(args->cdr);
        gc.attach(cont->state);     // Or it will be released
        cont->state->next = NULL;
        gc.expose(args);
        return cont->state;
    }
    throw NormalError(INT_ERR);
}

SpecialOptAnd::SpecialOptAnd() : SpecialOptObj("and") {}

void SpecialOptAnd::prepare(Pair *pc) {
    pc->next = NULL;
}

Pair *SpecialOptAnd::call(Pair *args, Environment * &lenvt,
        Continuation * &cont, EvalObj ** &top_ptr, Pair *pc) {
    Pair *ret_addr = cont->pc;
    if (pc->cdr == empty_list)
    {
        gc.expose(*top_ptr);
        *top_ptr++ = gc.attach(new BoolObj(true));
        EXIT_CURRENT_EXEC(lenvt, cont);
        gc.expose(args);
        return ret_addr->next;
    }
    if (!cont->state)
    {
        gc.attach(static_cast<EvalObj*>(*(++top_ptr)));
        top_ptr++;
        cont->state = TO_PAIR(pc->cdr);
        cont->state->next = NULL;
        gc.expose(args);
        return cont->state;
    }
    EvalObj *ret = TO_PAIR(args->cdr)->car;
    if (ret->is_true())
    {
        if (cont->state->cdr == empty_list) // the last member
        {
            gc.expose(*top_ptr);
            *top_ptr++ = gc.attach(ret);
            EXIT_CURRENT_EXEC(lenvt, cont);
            gc.expose(args);
            return ret_addr->next;
        }
        else
        {
            gc.attach(static_cast<EvalObj*>(*(++top_ptr)));
            top_ptr++;
            cont->state = TO_PAIR(cont->state->cdr);
            cont->state->next = NULL;
            gc.expose(args);
            return cont->state;
        }
    }
    else
    {
        gc.expose(*top_ptr);
        *top_ptr++ = gc.attach(ret);
        EXIT_CURRENT_EXEC(lenvt, cont);
        gc.expose(args);
        return ret_addr->next;
    }
    throw NormalError(INT_ERR);
}

SpecialOptOr::SpecialOptOr() : SpecialOptObj("or") {}

void SpecialOptOr::prepare(Pair *pc) {
    pc->next = NULL;
}

Pair *SpecialOptOr::call(Pair *args, Environment * &lenvt,
        Continuation * &cont, EvalObj ** &top_ptr, Pair *pc) {
    Pair *ret_addr = cont->pc;
    if (pc->cdr == empty_list)
    {
        gc.expose(*top_ptr);
        *top_ptr++ = gc.attach(new BoolObj(false));
        EXIT_CURRENT_EXEC(lenvt, cont);
        gc.expose(args);
        return ret_addr->next;
    }
    if (!cont->state)
    {
        gc.attach(static_cast<EvalObj*>(*(++top_ptr)));
        top_ptr++;
        cont->state = TO_PAIR(pc->cdr);
        cont->state->next = NULL;
        gc.expose(args);
        return cont->state;
    }
    EvalObj *ret = TO_PAIR(args->cdr)->car;
    if (!ret->is_true())
    {
        if (cont->state->cdr == empty_list) // the last member
        {
            gc.expose(*top_ptr);
            *top_ptr++ = gc.attach(ret);
            EXIT_CURRENT_EXEC(lenvt, cont);
            gc.expose(args);
            return ret_addr->next;
        }
        else
        {
            gc.attach(static_cast<EvalObj*>(*(++top_ptr)));
            top_ptr++;
            cont->state = TO_PAIR(cont->state->cdr);
            cont->state->next = NULL;
            gc.expose(args);
            return cont->state;
        }
    }
    else
    {
        gc.expose(*top_ptr);
        *top_ptr++ = gc.attach(ret);
        EXIT_CURRENT_EXEC(lenvt, cont);
        gc.expose(args);
        return ret_addr->next;
    }
    throw NormalError(INT_ERR);
}

SpecialOptApply::SpecialOptApply() : SpecialOptObj("apply") {}

void SpecialOptApply::prepare(Pair *pc) {}

Pair *SpecialOptApply::call(Pair *_args, Environment * &lenvt,
        Continuation * &cont, EvalObj ** &top_ptr, Pair *pc) {
    Pair *args = _args;
    top_ptr++;          // Recover the return address
    if (args->cdr == empty_list)
        throw TokenError(name, RUN_ERR_WRONG_NUM_OF_ARGS);

    args = TO_PAIR(args->cdr);
    if (!args->car->is_opt_obj())
        throw TokenError("an operator", RUN_ERR_WRONG_TYPE);

    *top_ptr++ = gc.attach(args->car);          // Push the operator into the stack
    args = TO_PAIR(args->cdr);                  // Examine arguments
    if (args == empty_list)
        throw TokenError(name, RUN_ERR_WRONG_NUM_OF_ARGS);

    for (; args->cdr != empty_list; args = TO_PAIR(args->cdr))
        *top_ptr++ = gc.attach(args->car);      // Add leading arguments: arg_1 ...

    if (args->car != empty_list)    // args->car is the trailing args
    {
        if (!args->car->is_pair_obj())
            throw TokenError("a list", RUN_ERR_WRONG_TYPE);

        args = TO_PAIR(args->car);
        EvalObj *nptr;
        for (;;)
        {
            *top_ptr++ = gc.attach(args->car);
            if ((nptr = args->cdr)->is_pair_obj())
                args = TO_PAIR(nptr);
            else break;
        }
        if (args->cdr != empty_list)
            throw TokenError("a list", RUN_ERR_WRONG_TYPE);
    }
    // force the invocation, so that the desired operator will take over
    gc.expose(_args);
    return NULL;
}

SpecialOptForce::SpecialOptForce() : SpecialOptObj("force") {}

void SpecialOptForce::prepare(Pair *pc) {
    if (pc->cdr == empty_list ||
        TO_PAIR(pc->cdr)->cdr != empty_list)
        throw TokenError(name, RUN_ERR_WRONG_NUM_OF_ARGS);
}

Pair *SpecialOptForce::call(Pair *_args, Environment * &lenvt,
        Continuation * &cont, EvalObj ** &top_ptr, Pair *pc) {
    Pair *args = _args;
    args = TO_PAIR(args->cdr);
    Pair *ret_addr = cont->pc;
    if (cont->state)
    {
        EvalObj *mem = args->car;
        prom->feed_mem(mem);
        gc.expose(*top_ptr);
        *top_ptr++ = gc.attach(mem);
        EXIT_CURRENT_EXEC(lenvt, cont);
        gc.expose(_args);
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
            gc.expose(*top_ptr);
            *top_ptr++ = gc.attach(mem);
            EXIT_CURRENT_EXEC(lenvt, cont);
            gc.expose(_args);
            return ret_addr->next;      
        }
        else                            // force
        {
            gc.attach(static_cast<EvalObj*>(*(++top_ptr)));
            top_ptr++;
            cont->state = prom->get_entry();
            cont->state->next = NULL;
            gc.expose(_args);
            return cont->state;
        }
    }
}

SpecialOptDelay::SpecialOptDelay() : SpecialOptObj("delay") {}

void SpecialOptDelay::prepare(Pair *pc) {
    if (pc->cdr == empty_list ||
        TO_PAIR(pc->cdr)->cdr != empty_list)
        throw TokenError(name, RUN_ERR_WRONG_NUM_OF_ARGS);
    pc->next = NULL;
}

Pair *SpecialOptDelay::call(Pair *args, Environment * &lenvt,
        Continuation * &cont, EvalObj ** &top_ptr, Pair *pc) {
    Pair *ret_addr = cont->pc;
    gc.expose(*top_ptr);
    *top_ptr++ = gc.attach(new PromObj(TO_PAIR(pc->cdr)->car));
    EXIT_CURRENT_EXEC(lenvt, cont);
    gc.expose(args);
    return ret_addr->next;          // Move to the next instruction
}

/*************************************************************************/

/* The following lines are the implementation of various simple built-in
 * procedures. Some library procdures are implemented here for the sake of
 * efficiency. */

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
    return gc.attach(args);     // Or it will be GCed
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
        if (res->level < opr->level)
        {
            res->add(opr = res->convert(opr));
            delete opr;
        }
        else
        {
            (res = opr->convert(res))->add(opr);
            delete _res;
        }
    }
    return res;
}

BUILTIN_PROC_DEF(num_sub) {
    ARGS_AT_LEAST_ONE;
    if (!args->car->is_num_obj())
        throw TokenError("a number", RUN_ERR_WRONG_TYPE);

    NumObj *res = static_cast<NumObj*>(args->car), *opr;
    res = res->clone();
    args = TO_PAIR(args->cdr);
    if (args == empty_list)
    {
        IntNumObj *_zero = new IntNumObj(0);
        NumObj *zero = res->convert(_zero);
        if (zero != _zero) delete _zero;
        zero->sub(res);
        return zero;
    }
    for (; args != empty_list; args = TO_PAIR(args->cdr))
    {
        if (!args->car->is_num_obj())        // not a number
            throw TokenError("a number", RUN_ERR_WRONG_TYPE);
        opr = static_cast<NumObj*>(args->car);
        // upper type conversion
        NumObj *_res = res;
        if (res->level < opr->level)
        {
            res->sub(opr = res->convert(opr));
            delete opr;
        }
        else
        {
            (res = opr->convert(res))->sub(opr);
            delete _res;
        }
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
        if (res->level < opr->level)
        {
            res->mul(opr = res->convert(opr));
            delete opr;
        }
        else
        {
            (res = opr->convert(res))->mul(opr);
            delete _res;
        }
    }
    return res;
}

BUILTIN_PROC_DEF(num_div) {
    ARGS_AT_LEAST_ONE;
    if (!args->car->is_num_obj())
        throw TokenError("a number", RUN_ERR_WRONG_TYPE);

    NumObj *res = static_cast<NumObj*>(args->car), *opr;
    if (res->level > NUM_LVL_RAT)
        res = new RatNumObj(static_cast<IntNumObj*>(res)->val);
    else res = res->clone();

    args = TO_PAIR(args->cdr);
    if (args == empty_list)
    {
        IntNumObj *_one = new IntNumObj(1);
        NumObj *one = res->convert(_one);
        if (one != _one) delete _one;
        one->div(res);
        return one;
    }
    for (; args != empty_list; args = TO_PAIR(args->cdr))
    {
        if (!args->car->is_num_obj())        // not a number
            throw TokenError("a number", RUN_ERR_WRONG_TYPE);
        opr = static_cast<NumObj*>(args->car);
        // upper type conversion
        NumObj *_res = res;
        if (res->level < opr->level)
        {
            res->div(opr = res->convert(opr));
            delete opr;
        }
        else
        {
            (res = opr->convert(res))->div(opr);
            delete _res;
        }
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
        {
            if (!last->le(opr = last->convert(opr)))
            {
                delete opr;
                return new BoolObj(false);
            }
            else delete opr;
        }
        else
        {
            if (!(last = opr->convert(last))->le(opr))
            {
                delete last;
                return new BoolObj(false);
            }
            else delete last;
        }
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
        {
            if (!last->lt(opr = last->convert(opr)))
            {
                delete opr;
                return new BoolObj(false);
            }
            else delete opr;
        }
        else
        {
            if (!(last = opr->convert(last))->lt(opr))
            {
                delete last;
                return new BoolObj(false);
            }
            else delete last;
        }
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
        {
            if (!last->ge(opr = last->convert(opr)))
            {
                delete opr;
                return new BoolObj(false);
            }
            else delete opr;
        }
        else
        {
            if (!(last = opr->convert(last))->ge(opr))
            {
                delete last;
                return new BoolObj(false);
            }
            else delete last;
        }
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
        {
            if (!last->gt(opr = last->convert(opr)))
            {
                delete opr;
                return new BoolObj(false);
            }
            else delete opr;
        }
        else
        {
            if (!(last = opr->convert(last))->gt(opr))
            {
                delete last;
                return new BoolObj(false);
            }
            else delete last;
        }
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
        {
            if (!last->eq(opr = last->convert(opr)))
            {
                delete opr;
                return new BoolObj(false);
            }
            else delete opr;
        }
        else
        {
            if (!(last = opr->convert(last))->eq(opr))
            {
                delete last;
                return new BoolObj(false);
            }
            else delete last;
        }
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
    Pair *p = TO_PAIR(args->car);
    gc.expose(p->car);
    p->car = TO_PAIR(args->cdr)->car;
    gc.attach(p->car);
    return unspec_obj;
}

BUILTIN_PROC_DEF(pair_set_cdr) {
    ARGS_EXACTLY_TWO;
    if (!args->car->is_pair_obj())
        throw TokenError("pair", RUN_ERR_WRONG_TYPE);
    Pair *p = TO_PAIR(args->car);
    gc.expose(p->cdr);
    p->cdr = TO_PAIR(args->cdr)->car;
    gc.attach(p->cdr);
    return unspec_obj;
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
    {
        puts("oops");
        throw NormalError(INT_ERR);
    }
    EvalObj* nptr;
    Pair head(NULL, NULL);
    tail = &head;
    for (;;)
    {
        gc.attach(TO_PAIR(tail)->cdr = new Pair(*src));
        tail = TO_PAIR(TO_PAIR(tail)->cdr);
        gc.attach(TO_PAIR(tail)->car);
        if ((nptr = src->cdr)->is_pair_obj())
            src = TO_PAIR(nptr);
        else break;
    }
    gc.attach(TO_PAIR(tail)->cdr);
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
                    gc.attach(prev->cdr = copy_list(TO_PAIR(args->car), tail));
                else
                    gc.attach(prev->cdr = args->car);
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
    CHECK_NUMBER(sec);
    CHECK_EXACT(sec);
    IntNumObj *val = static_cast<ExactNumObj*>(sec)->to_int();
    int i, k = val->get_i();
    delete val;
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
    CHECK_NUMBER(args->car);
    NumObj* num = static_cast<NumObj*>(args->car)->clone();
    num->abs();
    return num;
}

BUILTIN_PROC_DEF(num_mod) {
    ARGS_EXACTLY_TWO;
    EvalObj *first = args->car, *second = TO_PAIR(args->cdr)->car;
    CHECK_NUMBER(first);
    CHECK_NUMBER(second);

    CHECK_EXACT(first);
    CHECK_EXACT(second);

    IntNumObj *a = static_cast<ExactNumObj*>(first)->to_int();
    IntNumObj *b = static_cast<ExactNumObj*>(second)->to_int();

    a->mod(b);

    delete b;
    return a;
}

BUILTIN_PROC_DEF(num_quo) {
    ARGS_EXACTLY_TWO;
    EvalObj *first = args->car, *second = TO_PAIR(args->cdr)->car;
    CHECK_NUMBER(first);
    CHECK_NUMBER(second);

    CHECK_EXACT(first);
    CHECK_EXACT(second);

    IntNumObj *a = static_cast<ExactNumObj*>(first)->to_int();
    IntNumObj *b = static_cast<ExactNumObj*>(second)->to_int();

    a->div(b);

    delete b;
    return a;
}

BUILTIN_PROC_DEF(num_rem) {
    ARGS_EXACTLY_TWO;
    EvalObj *first = args->car, *second = TO_PAIR(args->cdr)->car;
    CHECK_NUMBER(first);
    CHECK_NUMBER(second);

    CHECK_EXACT(first);
    CHECK_EXACT(second);

    IntNumObj *a = static_cast<ExactNumObj*>(first)->to_int();
    IntNumObj *b = static_cast<ExactNumObj*>(second)->to_int();

    a->rem(b);

    delete b;
    return a;
}

BUILTIN_PROC_DEF(num_gcd) {
//    ARGS_AT_LEAST_ONE;
    IntNumObj *res = new IntNumObj(0);
    IntNumObj *opr; 
    for (;args != empty_list; args = TO_PAIR(args->cdr))
    {
        EvalObj *obj = args->car;
        CHECK_NUMBER(obj);
        CHECK_EXACT(obj);

        opr = static_cast<ExactNumObj*>(obj)->to_int();
        res->gcd(opr);
        delete opr;
    }
    return res;
}

BUILTIN_PROC_DEF(num_lcm) {
//    ARGS_AT_LEAST_ONE;
    IntNumObj *res = new IntNumObj(1);
    IntNumObj *opr; 
    for (;args != empty_list; args = TO_PAIR(args->cdr))
    {
        EvalObj *obj = args->car;
        CHECK_NUMBER(obj);
        CHECK_EXACT(obj);

        opr = static_cast<ExactNumObj*>(obj)->to_int();
        res->lcm(opr);
        delete opr;
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
    return new BoolObj(static_cast<StrObj*>(obj1)->gt(static_cast<StrObj*>(obj2)));
}

BUILTIN_PROC_DEF(string_ge) {
    ARGS_EXACTLY_TWO;
    EvalObj *obj1 = args->car;
    EvalObj *obj2 = TO_PAIR(args->cdr)->car;
    if (!obj1->is_str_obj() || !obj2->is_str_obj())
        throw TokenError("a string", RUN_ERR_WRONG_TYPE);
    return new BoolObj(static_cast<StrObj*>(obj1)->ge(static_cast<StrObj*>(obj2)));
}

BUILTIN_PROC_DEF(string_eq) {
    ARGS_EXACTLY_TWO;
    EvalObj *obj1 = args->car;
    EvalObj *obj2 = TO_PAIR(args->cdr)->car;
    if (!obj1->is_str_obj() || !obj2->is_str_obj())
        throw TokenError("a string", RUN_ERR_WRONG_TYPE);
    return new BoolObj(static_cast<StrObj*>(obj1)->eq(static_cast<StrObj*>(obj2)));
}

BUILTIN_PROC_DEF(make_vector) {
    ARGS_AT_LEAST_ONE;
    EvalObj *first = args->car;
    CHECK_NUMBER(first);
    CHECK_EXACT(first);
    IntNumObj *val = static_cast<ExactNumObj*>(first)->to_int();
    ssize_t len = val->get_i();
    delete val;
    if (len < 0)
        throw TokenError("a non-negative integer", RUN_ERR_WRONG_TYPE);

    EvalObj *fill; 

    args = TO_PAIR(args->cdr);
    if (args == empty_list)
        fill = unspec_obj;
    else if (args->cdr == empty_list)
        fill = args->car;
    else 
        throw TokenError(name, RUN_ERR_WRONG_NUM_OF_ARGS);

    VecObj *res = new VecObj(size_t(len), fill);
    return res;
}

BUILTIN_PROC_DEF(vector_set) {
    if (args == empty_list)
        throw TokenError(name, RUN_ERR_WRONG_NUM_OF_ARGS);
    if (!args->car->is_vect_obj())
        throw TokenError("a vector", RUN_ERR_WRONG_TYPE);
    VecObj *vect = static_cast<VecObj*>(args->car);

    args = TO_PAIR(args->cdr);
    if (args == empty_list)
        throw TokenError(name, RUN_ERR_WRONG_NUM_OF_ARGS);
    EvalObj *second = args->car;
    CHECK_NUMBER(second);
    CHECK_EXACT(second);
    IntNumObj *val = static_cast<ExactNumObj*>(second)->to_int();
    ssize_t k = val->get_i();
    delete val;
    if (k < 0)
        throw TokenError("a non-negative integer", RUN_ERR_WRONG_TYPE);

    args = TO_PAIR(args->cdr);
    if (args == empty_list)
        throw TokenError(name, RUN_ERR_WRONG_NUM_OF_ARGS);
    if (args->cdr != empty_list)
        throw TokenError(name, RUN_ERR_WRONG_NUM_OF_ARGS);

    vect->set(k, args->car);
    return unspec_obj;
}

BUILTIN_PROC_DEF(vector_ref) {
    if (args == empty_list)
        throw TokenError(name, RUN_ERR_WRONG_NUM_OF_ARGS);
    if (!args->car->is_vect_obj())
        throw TokenError("a vector", RUN_ERR_WRONG_TYPE);
    VecObj *vect = static_cast<VecObj*>(args->car);

    args = TO_PAIR(args->cdr);
    if (args == empty_list)
        throw TokenError(name, RUN_ERR_WRONG_NUM_OF_ARGS);
    if (args->cdr != empty_list)
        throw TokenError(name, RUN_ERR_WRONG_NUM_OF_ARGS);

    EvalObj *second = args->car;
    CHECK_NUMBER(second);
    CHECK_EXACT(second);
    IntNumObj *val = static_cast<ExactNumObj*>(second)->to_int();
    ssize_t k = val->get_i();
    delete val;
    if (k < 0)
        throw TokenError("a non-negative integer", RUN_ERR_WRONG_TYPE);
    return vect->get(k);
}

BUILTIN_PROC_DEF(vector_length) {
    if (args == empty_list)
        throw TokenError(name, RUN_ERR_WRONG_NUM_OF_ARGS);
    if (!args->car->is_vect_obj())
        throw TokenError("a vector", RUN_ERR_WRONG_TYPE);
    if (args->cdr != empty_list)
        throw TokenError(name, RUN_ERR_WRONG_NUM_OF_ARGS);
    VecObj *vect = static_cast<VecObj*>(args->car);
    return new IntNumObj(vect->get_size());
}

BUILTIN_PROC_DEF(gc_status) {
    if (args != empty_list)
        throw TokenError(name, RUN_ERR_WRONG_NUM_OF_ARGS);
    return new IntNumObj(gc.get_remaining());
}

BUILTIN_PROC_DEF(set_gc_resolve_threshold) {
    ARGS_EXACTLY_ONE;
    EvalObj *first = args->car;
    CHECK_NUMBER(first);
    CHECK_EXACT(first);
    IntNumObj *val = static_cast<ExactNumObj*>(first)->to_int();
    ssize_t s = val->get_i();
    delete val;
    if (s < 0)
        throw TokenError("a non-negative integer", RUN_ERR_WRONG_TYPE);
    gc.set_resolve_threshold(size_t(s));
    return new UnspecObj();
}

BUILTIN_PROC_DEF(display) {
    ARGS_EXACTLY_ONE;
    printf("%s", args->car->ext_repr().c_str());
    return unspec_obj;
}
