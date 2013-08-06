#include "exc.h"
#include "consts.h"
#include "builtin.h"
#include "model.h"
#include "exc.h"
#include <cstdio>
#include <sstream>
#include <cctype>
#include <cstdlib>

using std::stringstream;

extern EmptyList *empty_list;
static const int NUM_LVL_COMP = 0;
static const int NUM_LVL_REAL = 1;
static const int NUM_LVL_RAT = 2;
static const int NUM_LVL_INT = 3;


string double_to_str(double val, bool force_sign = false) {
    stringstream ss;
    if (force_sign) ss << std::showpos;
    ss << val;
    return ss.str();
}

string int_to_str(int val) {
    stringstream ss;
    ss << val;
    return ss.str();
}

double str_to_double(string repr, bool &flag) {
    const char *nptr = repr.c_str();
    char *endptr; 
    double val = strtod(nptr, &endptr);
    if (endptr != nptr + repr.length())
    {
        flag = false;
        return 0;
    }
    flag = true;
    return val;
}

int str_to_int(string repr, bool &flag) {
    const char *nptr = repr.c_str();
    char *endptr; 
    int val = strtol(nptr, &endptr, 10);
    if (endptr != nptr + repr.length())
    {
        flag = false;
        return 0;
    }
    flag = true;
    return val;
}


int gcd(int a, int b) {
    int t;
    while (b) t = b, b = a % b, a = t;
    return abs(a);
}


InexactNumObj::InexactNumObj(NumLvl level) : NumObj(level, false) {}

CompNumObj::CompNumObj(double _real, double _imag) :
    InexactNumObj(NUM_LVL_COMP), real(_real), imag(_imag) {}

CompNumObj *CompNumObj::from_string(string repr) {
    // spos: the position of the last sign
    // ipos: the position of i
    int spos = -1, ipos = -1, 
        len = repr.length();
    bool sign;
    for (int i = 0; i < len; i++)
        if (repr[i] == '+' || repr[i] == '-')
        {
            spos = i;
            sign = repr[i] == '-';
        }
        else if (repr[i] == 'i' || repr[i] == 'I') 
            ipos = i;

    if (spos == -1 || ipos == -1 || !(spos < ipos)) 
        return NULL;

    double real = 0, imag = 1;
    IntNumObj *int_ptr;
    RatNumObj *rat_ptr;
    RealNumObj *real_ptr;
    if (spos > 0)
    {
        string real_str = repr.substr(0, spos);
        if (int_ptr = IntNumObj::from_string(real_str))
            real = int_ptr->val;
        else if ((rat_ptr = RatNumObj::from_string(real_str)))
            real = rat_ptr->a / double(rat_ptr->b);
        else if ((real_ptr = RealNumObj::from_string(real_str)))
            real = real_ptr->real;
        else return NULL;
    }
    if (ipos > spos + 1)
    {
        string imag_str = repr.substr(spos + 1, ipos - spos - 1);
        if (int_ptr = IntNumObj::from_string(imag_str))
            imag = int_ptr->val;
        else if ((rat_ptr = RatNumObj::from_string(imag_str)))
            imag = rat_ptr->a / double(rat_ptr->b);
        else if ((real_ptr = RealNumObj::from_string(imag_str)))
            imag = real_ptr->real;
        else return NULL;
    }
    if (sign) imag = -imag;
    return new CompNumObj(real, imag);
}

CompNumObj *CompNumObj::convert(NumObj *obj) {
    switch (obj->level) 
    {
        case NUM_LVL_COMP : 
            return static_cast<CompNumObj*>(obj); break;
        case NUM_LVL_REAL : 
            return new CompNumObj(static_cast<RealNumObj*>(obj)->real, 0);
            break;
        case NUM_LVL_RAT :
            {
            RatNumObj *rat = static_cast<RatNumObj*>(obj);
            return new CompNumObj(rat->a / double(rat->b), 0);
            break;
            }
        case NUM_LVL_INT :
            return new CompNumObj(static_cast<IntNumObj*>(obj)->val, 0);
    }
    throw NormalError(INT_ERR);
}

#define A (real)
#define B (imag)
#define C (r->real)
#define D (r->imag)

NumObj *CompNumObj::plus(NumObj *_r) {
    CompNumObj *r = static_cast<CompNumObj*>(_r);
    return new CompNumObj(A + C, B + D);
}

NumObj *CompNumObj::minus(NumObj *_r) {
    CompNumObj *r = static_cast<CompNumObj*>(_r);
    return new CompNumObj(A - C, B - D);
}

NumObj *CompNumObj::multi(NumObj *_r) {
    CompNumObj *r = static_cast<CompNumObj*>(_r);
    return new CompNumObj(A * C - B * D,
                         B * C + A * D);
}

NumObj *CompNumObj::div(NumObj *_r) {
    CompNumObj *r = static_cast<CompNumObj*>(_r);
    double f = 1.0 / (C * C + D * D);
    return new CompNumObj((A * C + B * D) * f,
                            (B * C - A * D) * f);
}

bool CompNumObj::lt(NumObj *_r) {
    throw TokenError("a comparable number", RUN_ERR_WRONG_TYPE);
}

bool CompNumObj::gt(NumObj *_r) {
    throw TokenError("a comparable number", RUN_ERR_WRONG_TYPE);
}

bool CompNumObj::eq(NumObj *_r) {
    CompNumObj *r = static_cast<CompNumObj*>(_r);
    return A == C && B == D; // TODO: more proper judgement
}


string CompNumObj::ext_repr() {
    return double_to_str(real) + double_to_str(imag, true) + "i";
}

#undef A
#undef B
#undef C
#undef D

RealNumObj::RealNumObj(double _real) : InexactNumObj(NUM_LVL_REAL), real(_real) {}

RealNumObj *RealNumObj::from_string(string repr) {
    bool flag;
    double real = str_to_double(repr, flag);
    if (!flag) return NULL;
    return new RealNumObj(real);
}

RealNumObj *RealNumObj::convert(NumObj *obj) {
    switch (obj->level)
    {
        case NUM_LVL_REAL: 
            return static_cast<RealNumObj*>(obj); break;
        case NUM_LVL_RAT:
            {
            RatNumObj *rat = static_cast<RatNumObj*>(obj);
            return new RealNumObj(rat->a / double(rat->b));
            break;
            }
        case NUM_LVL_INT:
            return new RealNumObj(static_cast<IntNumObj*>(obj)->val);
    }
    throw NormalError(INT_ERR);
}

NumObj *RealNumObj::plus(NumObj *_r) {
    return new RealNumObj(real + static_cast<RealNumObj*>(_r)->real);
}

NumObj *RealNumObj::minus(NumObj *_r) {
    return new RealNumObj(real - static_cast<RealNumObj*>(_r)->real);
}

NumObj *RealNumObj::multi(NumObj *_r) {
    return new RealNumObj(real * static_cast<RealNumObj*>(_r)->real);
}

NumObj *RealNumObj::div(NumObj *_r) {
    return new RealNumObj(real / static_cast<RealNumObj*>(_r)->real);
}

bool RealNumObj::eq(NumObj *_r) {
    return real == static_cast<RealNumObj*>(_r)->real;
}

bool RealNumObj::lt(NumObj *_r) {
    return real < static_cast<RealNumObj*>(_r)->real;
}

bool RealNumObj::gt(NumObj *_r) {
    return real > static_cast<RealNumObj*>(_r)->real;
}

string RealNumObj::ext_repr() {
    return double_to_str(real);
}

ExactNumObj::ExactNumObj(NumLvl level) : NumObj(level, true) {}

RatNumObj::RatNumObj(int _a, int _b) : 
    ExactNumObj(NUM_LVL_RAT), a(_a), b(_b) {
    int g = gcd(a, b);
    a /= g;
    b /= g;
}

RatNumObj *RatNumObj::from_string(string repr) {
    int a, b;
    int len = repr.length(), pos = -1;
    for (int i = 0; i < len; i++)
        if (repr[i] == '/') { pos = i; break; }
    bool flag;
    a = str_to_int(repr.substr(0, pos), flag);
    if (!flag) return NULL;
    b = str_to_int(repr.substr(pos + 1, len - pos - 1), flag);
    if (!flag) return NULL;

    return new RatNumObj(a, b);
}

RatNumObj *RatNumObj::convert(NumObj *obj) {
    switch (obj->level)
    {
        case NUM_LVL_RAT:
            return static_cast<RatNumObj*>(obj); break;
        case NUM_LVL_INT:
            return new RatNumObj(static_cast<IntNumObj*>(obj)->val, 1);
    }
    throw NormalError(INT_ERR);
}

#define A (a)
#define B (b)
#define C (r->a)
#define D (r->b)

NumObj *RatNumObj::plus(NumObj *_r) {
    RatNumObj *r = static_cast<RatNumObj*>(_r);
    int na = A * D + B * C, nb = B * D;
    int g = gcd(na, nb);
    na /= g;
    nb /= g;
    return new RatNumObj(na, nb);
}

NumObj *RatNumObj::minus(NumObj *_r) {
    RatNumObj *r = static_cast<RatNumObj*>(_r);
    int na = A * D - B * C, nb = B * D;
    int g = gcd(na, nb);
    na /= g;
    nb /= g;
    return new RatNumObj(na, nb);
}

NumObj *RatNumObj::multi(NumObj *_r) {
    RatNumObj *r = static_cast<RatNumObj*>(_r);
    int na = A * C, nb = B * D;
    int g = gcd(na, nb);
    na /= g;
    nb /= g;
    return new RatNumObj(na, nb);
}

NumObj *RatNumObj::div(NumObj *_r) {
    RatNumObj *r = static_cast<RatNumObj*>(_r);
    int na = A * D, nb = B * C;
    int g = gcd(na, nb);
    na /= g;
    nb /= g;
    return new RatNumObj(na, nb);
}

bool RatNumObj::lt(NumObj *_r) {
    RatNumObj *r = static_cast<RatNumObj*>(_r);
    return A * D < C * B;
}

bool RatNumObj::gt(NumObj *_r) {
    RatNumObj *r = static_cast<RatNumObj*>(_r);
    return A * D > C * B;
}

bool RatNumObj::eq(NumObj *_r) {
    RatNumObj *r = static_cast<RatNumObj*>(_r);
    return A * D == C * B;
}

string RatNumObj::ext_repr() {
    return int_to_str(A) + "/" + int_to_str(B);
}

IntNumObj::IntNumObj(int _val) : ExactNumObj(NUM_LVL_INT), val(_val) {}

IntNumObj *IntNumObj::from_string(string repr) {
    int val = 0;
    for (int i = 0; i < repr.length(); i++)
    {
        if (!('0' <= repr[i] && repr[i] <= '9'))
            return NULL;
        val = val * 10 + repr[i] - '0';
    }
    return new IntNumObj(val);
}

IntNumObj *IntNumObj::convert(NumObj *obj) {
    switch (obj->level)
    {
        case NUM_LVL_INT :
            return static_cast<IntNumObj*>(obj);
        default:
            throw NormalError(INT_ERR);
    }
}

NumObj *IntNumObj::plus(NumObj *_r) {

    return new IntNumObj(val + static_cast<IntNumObj*>(_r)->val);
}

NumObj *IntNumObj::minus(NumObj *_r) {
    return new IntNumObj(val - static_cast<IntNumObj*>(_r)->val);
}

NumObj *IntNumObj::multi(NumObj *_r) {
    return new IntNumObj(val * static_cast<IntNumObj*>(_r)->val);
}

NumObj *IntNumObj::div(NumObj *_r) {
    return new IntNumObj(val / static_cast<IntNumObj*>(_r)->val);
}

bool IntNumObj::lt(NumObj *_r) {
    return val < static_cast<IntNumObj*>(_r)->val;
}

bool IntNumObj::gt(NumObj *_r) {
    return val > static_cast<IntNumObj*>(_r)->val;
}

bool IntNumObj::eq(NumObj *_r) {
    return val == static_cast<IntNumObj*>(_r)->val;
}

string IntNumObj::ext_repr() {
    return int_to_str(val);
}

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
    if (args->car == empty_list || !args->car->is_cons_obj())
        throw TokenError("pair", RUN_ERR_WRONG_TYPE);

    return TO_CONS(args->car)->car;
}

EvalObj *builtin_cdr(ArgList *args) {
    if (args == empty_list ||
        args->cdr != empty_list)
        throw TokenError("cdr", RUN_ERR_WRONG_NUM_OF_ARGS);
    if (args->car == empty_list || !args->car->is_cons_obj())
        throw TokenError("pair", RUN_ERR_WRONG_TYPE);

    return TO_CONS(args->car)->cdr;
}

EvalObj *builtin_list(ArgList *args) {
    return args;
}

EvalObj *builtin_plus(ArgList *args) {
    NumObj *res = new IntNumObj(0), *opr; // the most accurate type
    for (Cons *ptr = args; ptr != empty_list; ptr = TO_CONS(ptr->cdr))
    {
        if (!ptr->car->is_num_obj())        // not a number
            throw TokenError("a number", RUN_ERR_WRONG_TYPE);
        opr = static_cast<NumObj*>(ptr->car);
        NumObj *_res = res;
        if (_res->level < opr->level)
            opr = _res->convert(opr);
        else
            _res =  opr->convert(_res);
        res = _res->plus(opr);
    }
    return res; 
}

EvalObj *builtin_minus(ArgList *args) {
    if (args == empty_list)
        throw TokenError("-", RUN_ERR_WRONG_NUM_OF_ARGS);
    if (!args->car->is_num_obj())
        throw TokenError("a number", RUN_ERR_WRONG_TYPE);

    NumObj *res = static_cast<NumObj*>(args->car), *opr; 
    for (Cons *ptr = TO_CONS(args->cdr); 
            ptr != empty_list; ptr = TO_CONS(ptr->cdr))
    {
        if (!ptr->car->is_num_obj())        // not a number
            throw TokenError("a number", RUN_ERR_WRONG_TYPE);
        opr = static_cast<NumObj*>(ptr->car);
        // upper type conversion
        NumObj *_res = res;
        if (_res->level < opr->level)
            opr = _res->convert(opr);
        else
            _res =  opr->convert(_res);
        res = _res->minus(opr);
    }
    return res; 
}

EvalObj *builtin_multi(ArgList *args) {
    NumObj *res = new IntNumObj(1), *opr; // the most accurate type
    for (Cons *ptr = args; ptr != empty_list; ptr = TO_CONS(ptr->cdr))
    {
        if (!ptr->car->is_num_obj())        // not a number
            throw TokenError("a number", RUN_ERR_WRONG_TYPE);
        opr = static_cast<NumObj*>(ptr->car);
        NumObj *_res = res;
        if (_res->level < opr->level)
            opr = _res->convert(opr);
        else
            _res =  opr->convert(_res);
        res = _res->multi(opr);
    }
    return res; 
}

EvalObj *builtin_div(ArgList *args) {
    if (args == empty_list)
        throw TokenError("/", RUN_ERR_WRONG_NUM_OF_ARGS);
    if (!args->car->is_num_obj())
        throw TokenError("a number", RUN_ERR_WRONG_TYPE);

    NumObj *res = static_cast<NumObj*>(args->car), *opr; 
    for (Cons *ptr = TO_CONS(args->cdr); 
            ptr != empty_list; ptr = TO_CONS(ptr->cdr))
    {
        if (!ptr->car->is_num_obj())        // not a number
            throw TokenError("a number", RUN_ERR_WRONG_TYPE);
        opr = static_cast<NumObj*>(ptr->car);
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

EvalObj *builtin_lt(ArgList *args) {
    if (args == empty_list)
        throw TokenError("<", RUN_ERR_WRONG_NUM_OF_ARGS);
    if (!args->car->is_num_obj())
        throw TokenError("a number", RUN_ERR_WRONG_TYPE);
       
    NumObj *last = static_cast<NumObj*>(args->car), *opr; 

    for (Cons *ptr = TO_CONS(args->cdr); 
            ptr != empty_list; ptr = TO_CONS(ptr->cdr), last = opr)
    {
        if (!ptr->car->is_num_obj())        // not a number
            throw TokenError("a number", RUN_ERR_WRONG_TYPE);
        opr = static_cast<NumObj*>(ptr->car);
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

EvalObj *builtin_gt(ArgList *args) {
    if (args == empty_list)
        throw TokenError(">", RUN_ERR_WRONG_NUM_OF_ARGS);
    if (!args->car->is_num_obj())
        throw TokenError("a number", RUN_ERR_WRONG_TYPE);
       
    NumObj *last = static_cast<NumObj*>(args->car), *opr; 

    for (Cons *ptr = TO_CONS(args->cdr); 
            ptr != empty_list; ptr = TO_CONS(ptr->cdr), last = opr)
    {
        if (!ptr->car->is_num_obj())        // not a number
            throw TokenError("a number", RUN_ERR_WRONG_TYPE);
        opr = static_cast<NumObj*>(ptr->car);
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

EvalObj *builtin_arithmetic_eq(ArgList *args) {
    if (args == empty_list)
        throw TokenError("=", RUN_ERR_WRONG_NUM_OF_ARGS);
    if (!args->car->is_num_obj())
        throw TokenError("a number", RUN_ERR_WRONG_TYPE);
       
    NumObj *last = static_cast<NumObj*>(args->car), *opr; 

    for (Cons *ptr = TO_CONS(args->cdr); 
            ptr != empty_list; ptr = TO_CONS(ptr->cdr), last = opr)
    {
        if (!ptr->car->is_num_obj())        // not a number
            throw TokenError("a number", RUN_ERR_WRONG_TYPE);
        opr = static_cast<NumObj*>(ptr->car);
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


EvalObj *builtin_exact(ArgList *args) {
    if (args == empty_list ||
        args->cdr != empty_list)
        throw TokenError("(in)exact?", RUN_ERR_WRONG_NUM_OF_ARGS);
    if (!args->car->is_num_obj())
        throw TokenError("a number", RUN_ERR_WRONG_TYPE);
    return new BoolObj(static_cast<NumObj*>(args->car)->is_exact()); 
}

EvalObj *builtin_inexact(ArgList *args) {
    BoolObj *ret = static_cast<BoolObj*>(builtin_exact(args));
    ret->val = !ret->val;
    return ret;
}


EvalObj *builtin_display(ArgList *args) {
    printf("%s\n", args->car->ext_repr().c_str());
    return new UnspecObj();
}
