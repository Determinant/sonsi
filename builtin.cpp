#include "exc.h"
#include "consts.h"
#include "builtin.h"
#include <cstdio>
#include <sstream>

using std::stringstream;

extern EmptyList *empty_list;

BoolObj::BoolObj(bool _val) : EvalObj(), val(_val) {}

bool BoolObj::is_true() { return val; }

string BoolObj::ext_repr() { return string(val ? "#t" : "#f"); }

#ifdef DEBUG
string BoolObj::_debug_repr() { return ext_repr(); }
#endif

IntObj::IntObj(int _val) : NumberObj(), val(_val) {}

string IntObj::ext_repr() {
    stringstream ss;
    ss << val;
    return ss.str();
}

#ifdef DEBUG
string IntObj::_debug_repr() { return ext_repr(); }
#endif

FloatObj::FloatObj(double _val) : NumberObj(), val(_val) {}

string FloatObj::ext_repr() { 
    stringstream ss;
    ss << val;
    return ss.str();
}

#ifdef DEBUG
string FloatObj::_debug_repr() { return ext_repr(); }
#endif

SpecialOptIf::SpecialOptIf() : SpecialOptObj() {}

void SpecialOptIf::prepare(Cons *pc) {
    state = 0;  // Prepared

    pc = TO_CONS(pc->cdr);
    if (pc == empty_list)
        throw TokenError("if", SYN_ERR_MISS_OR_EXTRA_EXP);
    pc->skip = false;

    pc = TO_CONS(pc->cdr);
    if (pc == empty_list)
        throw TokenError("if", SYN_ERR_MISS_OR_EXTRA_EXP);

    pc->skip = true;
    if (pc->cdr != empty_list)
        TO_CONS(pc->cdr)->skip = true;
}

void SpecialOptIf::pre_call(ArgList *args, Cons *pc,
        Environment *envt) {
    // static_cast because it's a call invocation
    pc = TO_CONS(TO_CONS(pc->car)->cdr); 

    // Condition evaluated and the decision is made
    state = 1;

    if (TO_CONS(args->cdr)->car->is_true())
    {
        pc->skip = true;
        pc = TO_CONS(pc->cdr);
        pc->skip = false;
        if (pc->cdr != empty_list)
            TO_CONS(pc->cdr)->skip = true; // Eval the former
    }
    else
    {
        pc->skip = true;
        pc = TO_CONS(pc->cdr);
        TO_CONS(pc->cdr)->skip = true;
        if (pc->cdr != empty_list)
            TO_CONS(pc->cdr)->skip = false; //Eval the latter
    }
}

EvalObj *SpecialOptIf::post_call(ArgList *args, Cons *pc,
                                Environment *envt) {
    // Value already evaluated, so just return it
    return TO_CONS(args->cdr)->car;
}

Cons *SpecialOptIf::call(ArgList *args, Environment * &envt, 
                        Continuation * &cont, FrameObj ** &top_ptr) {
    Cons *ret_addr = static_cast<RetAddr*>(*top_ptr)->addr;
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
        return static_cast<Cons*>(ret_addr->car)->next;
    }
}

string SpecialOptIf::ext_repr() { return string("#<Builtin Macro: if>"); }

#ifdef DEBUG
string SpecialOptIf::_debug_repr() { return ext_repr(); }
#endif

SpecialOptLambda::SpecialOptLambda() : SpecialOptObj() {}
#define FILL_MARKS(pc, flag) \
    for (Cons *ptr = TO_CONS(pc->cdr); \
            ptr != empty_list; ptr = TO_CONS(ptr->cdr)) \
        ptr->skip = flag

void SpecialOptLambda::prepare(Cons *pc) {
    //TODO check number of arguments
    // Do not evaluate anything
    FILL_MARKS(pc, true);
}

Cons *SpecialOptLambda::call(ArgList *args, Environment * &envt, 
                            Continuation * &cont, FrameObj ** &top_ptr) {

    Cons *ret_addr = static_cast<RetAddr*>(*top_ptr)->addr;
    Cons *pc = static_cast<Cons*>(ret_addr->car);

    if (pc->cdr == empty_list)
        throw TokenError("lambda", SYN_ERR_EMPTY_PARA_LIST);
    if (TO_CONS(pc->cdr)->cdr == empty_list)
        throw TokenError("lambda", SYN_ERR_MISS_OR_EXTRA_EXP);

    SymbolList *para_list = static_cast<SymbolList*>(TO_CONS(pc->cdr)->car); 
    // Clear the flag to avoid side-effects (e.g. proc calling)
    FILL_MARKS(pc, false);

    // store a list of expressions inside <body>
    ASTList *body = TO_CONS(TO_CONS(pc->cdr)->cdr);       // Truncate the expression list
    for (Cons *ptr = body; ptr != empty_list; ptr = TO_CONS(ptr->cdr))
        ptr->next = NULL;    // Make each expression an orphan

    *top_ptr++ = new ProcObj(body, envt, para_list);
    return ret_addr->next;  // Move to the next instruction
}

string SpecialOptLambda::ext_repr() { return string("#<Builtin Macro: lambda>"); }

#ifdef DEBUG
string SpecialOptLambda::_debug_repr() { return ext_repr(); }
#endif

SpecialOptDefine::SpecialOptDefine() : SpecialOptObj() {}

void SpecialOptDefine::prepare(Cons *pc) {
    if (pc->cdr == empty_list)
        throw TokenError("define", SYN_ERR_MISS_OR_EXTRA_EXP);

    if (TO_CONS(pc->cdr)->car->is_simple_obj())  // Simple value assignment
    {
        pc = TO_CONS(pc->cdr);
        if (pc->cdr == empty_list)
            throw TokenError("define", SYN_ERR_MISS_OR_EXTRA_EXP);
        pc->skip = true;           // Skip the identifier
        TO_CONS(pc->cdr)->skip = false; 
    }                                   // Procedure definition
    else FILL_MARKS(pc, true);          // Skip all parts
}

Cons *SpecialOptDefine::call(ArgList *args, Environment * &envt, 
        Continuation * &cont, FrameObj ** &top_ptr) {
    Cons *ret_addr = static_cast<RetAddr*>(*top_ptr)->addr;
    Cons *pc = static_cast<Cons*>(ret_addr->car);
    EvalObj *obj;
    SymObj *id;
    // TODO: check identifier
    EvalObj *first = TO_CONS(pc->cdr)->car;
    if (first->is_simple_obj())
    {
        if (!first->is_sym_obj())
            throw TokenError(first->ext_repr(), SYN_ERR_NOT_AN_ID);

        id = static_cast<SymObj*>(first);
        obj = TO_CONS(args->cdr)->car;
    }
    else
    {
        // static_cast because of is_simple_obj() is false
        Cons *plst = static_cast<Cons*>(TO_CONS(pc->cdr)->car);

        if (plst == empty_list)
            throw TokenError("if", SYN_ERR_EMPTY_PARA_LIST);
        if (!plst->car->is_sym_obj())
            throw TokenError(first->ext_repr(), SYN_ERR_NOT_AN_ID);

        id = static_cast<SymObj*>(plst->car);
        ArgList *para_list = TO_CONS(plst->cdr);
        // Clear the flag to avoid side-effects (e.g. proc calling)
        FILL_MARKS(pc, false);

        ASTList *body = TO_CONS(TO_CONS(pc->cdr)->cdr);   // Truncate the expression list

        if (body == empty_list)
            throw TokenError("define", SYN_ERR_MISS_OR_EXTRA_EXP);

        for (Cons *ptr = body; ptr != empty_list; ptr = TO_CONS(ptr->cdr))
            ptr->next = NULL;           // Make each expression a orphan

        obj = new ProcObj(body, envt, para_list);
    }
    envt->add_binding(id, obj);
    *top_ptr++ = new UnspecObj();
    return ret_addr->next;
}

string SpecialOptDefine::ext_repr() { return string("#<Builtin Macro: define>"); }

#ifdef DEBUG
string SpecialOptDefine::_debug_repr() { return ext_repr(); }
#endif

void SpecialOptSet::prepare(Cons *pc) {
    pc = TO_CONS(pc->cdr);
    if (pc == empty_list)
        throw TokenError("set!", SYN_ERR_MISS_OR_EXTRA_EXP);

    pc->skip = true;       // Skip the identifier

    pc = TO_CONS(pc->cdr);
    if (pc == empty_list)
        throw TokenError("set!", SYN_ERR_MISS_OR_EXTRA_EXP);

    pc->skip = false; 
}

Cons *SpecialOptSet::call(ArgList *args, Environment * &envt, 
                            Continuation * &cont, FrameObj ** &top_ptr) {
    Cons *ret_addr = static_cast<RetAddr*>(*top_ptr)->addr;
    Cons *pc = static_cast<Cons*>(ret_addr->car);
    EvalObj *first = TO_CONS(pc->cdr)->car;

    if (!first->is_sym_obj())
        throw TokenError(first->ext_repr(), SYN_ERR_NOT_AN_ID);

    SymObj *id = static_cast<SymObj*>(first);

    bool flag = envt->add_binding(id, TO_CONS(args->cdr)->car, false);
    if (!flag) throw TokenError(id->ext_repr(), RUN_ERR_UNBOUND_VAR);
    *top_ptr++ = new UnspecObj();
    return ret_addr->next;
}

SpecialOptSet::SpecialOptSet() {}

string SpecialOptSet::ext_repr() { return string("#<Builtin Macro: set!>"); }

#ifdef DEBUG
string SpecialOptSet::_debug_repr() { return ext_repr(); }
#endif

EvalObj *builtin_cons(ArgList *args) {
    if (args == empty_list ||
        args->cdr == empty_list ||
        TO_CONS(args->cdr)->cdr != empty_list)
        throw TokenError("cons", RUN_ERR_WRONG_NUM_OF_ARGS);

    return new Cons(args->car, TO_CONS(args->cdr)->car);
}

EvalObj *builtin_car(ArgList *args) {
    if (args == empty_list ||
        args->cdr != empty_list)
        throw TokenError("car", RUN_ERR_WRONG_NUM_OF_ARGS);
    if (!args->car->is_cons_obj())
        throw TokenError("pair", RUN_ERR_WRONG_TYPE);

    return TO_CONS(args->car)->car;
}

EvalObj *builtin_cdr(ArgList *args) {
    if (args == empty_list ||
        args->cdr != empty_list)
        throw TokenError("cdr", RUN_ERR_WRONG_NUM_OF_ARGS);
    if (!args->car->is_cons_obj())
        throw TokenError("pair", RUN_ERR_WRONG_TYPE);

    return TO_CONS(args->car)->cdr;
}

EvalObj *builtin_list(ArgList *args) {
    return args;
}

EvalObj *builtin_plus(ArgList *args) {
    // TODO: type conversion and proper arithmetic
    int res = 0;
    for (Cons *ptr = args; ptr != empty_list; ptr = TO_CONS(ptr->cdr))
        res += dynamic_cast<IntObj*>(ptr->car)->val;
    return new IntObj(res);
}

EvalObj *builtin_minus(ArgList *args) {
    // TODO: type conversion and proper arithmetic
    int res = dynamic_cast<IntObj*>(args->car)->val;
    for (Cons *ptr = TO_CONS(args->cdr); 
            ptr != empty_list; ptr = TO_CONS(ptr->cdr))
        res -= dynamic_cast<IntObj*>(ptr->car)->val;
    return new IntObj(res);
}

EvalObj *builtin_times(ArgList *args) {
    // TODO: type conversion and proper arithmetic
    int res = 1;
    for (Cons *ptr = args; ptr != empty_list; ptr = TO_CONS(ptr->cdr))
        res *= dynamic_cast<IntObj*>(ptr->car)->val;
    return new IntObj(res);
}

EvalObj *builtin_div(ArgList *args) {
    // TODO: type conversion and proper arithmetic
    int res = dynamic_cast<IntObj*>(args->car)->val;
    for (Cons *ptr = TO_CONS(args->cdr); ptr != empty_list; ptr = TO_CONS(ptr->cdr))
        res /= dynamic_cast<IntObj*>(ptr->car)->val;
    return new IntObj(res);
}

EvalObj *builtin_lt(ArgList *args) {
    return new BoolObj(dynamic_cast<IntObj*>(args->car)->val <
                    dynamic_cast<IntObj*>(TO_CONS(args->cdr)->car)->val);
}

EvalObj *builtin_gt(ArgList *args) {
    return new BoolObj(dynamic_cast<IntObj*>(args->car)->val >
                    dynamic_cast<IntObj*>(TO_CONS(args->cdr)->car)->val);
}

EvalObj *builtin_arithmetic_eq(ArgList *args) {
    return new BoolObj(dynamic_cast<IntObj*>(args->car)->val ==
                    dynamic_cast<IntObj*>(TO_CONS(args->cdr)->car)->val);
}

EvalObj *builtin_display(ArgList *args) {
    printf("%s\n", args->car->ext_repr().c_str());
    return new UnspecObj();
}
