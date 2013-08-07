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

#define ARGS_EXACTLY_TWO \
    if (args == empty_list || !args->cdr->is_cons_obj() || \
            TO_CONS(args->cdr)->cdr != empty_list) \
        throw TokenError(name, RUN_ERR_WRONG_NUM_OF_ARGS) \

#define ARGS_EXACTLY_ONE \
    if (args == empty_list || args->cdr != empty_list ) \
        throw TokenError(name, RUN_ERR_WRONG_NUM_OF_ARGS)

#define ARGS_AT_LEAST_ONE \
    if (args == empty_list) \
        throw TokenError(name, RUN_ERR_WRONG_NUM_OF_ARGS)

bool is_list(Cons *ptr) {
    if (ptr == empty_list) return true;
    EvalObj *nptr; 
    for (;;) 
        if ((nptr = ptr->cdr)->is_cons_obj()) 
            ptr = TO_CONS(nptr); 
        else break; 
    return ptr->cdr == empty_list;
}

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
    if (endptr == nptr || endptr != nptr + repr.length())
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
    if (endptr == nptr || endptr != nptr + repr.length())
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
        int spos = -1, ipos = -1;
        size_t len = repr.length();
        bool sign;
        for (size_t i = 0; i < len; i++)
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
#ifndef GMP_SUPPORT
                real = int_ptr->val;
#else
                real = int_ptr->val.get_d();
#endif
            else if ((rat_ptr = RatNumObj::from_string(real_str)))
#ifndef GMP_SUPPORT
                real = rat_ptr->a / double(rat_ptr->b);
#else
                real = rat_ptr->val.get_d();                
#endif
            else if ((real_ptr = RealNumObj::from_string(real_str)))
                real = real_ptr->real;
            else return NULL;
        }
        if (ipos > spos + 1)
        {
            string imag_str = repr.substr(spos + 1, ipos - spos - 1);
            if (int_ptr = IntNumObj::from_string(imag_str))
#ifndef GMP_SUPPORT
                imag = int_ptr->val;
#else
                imag = int_ptr->val.get_d();
#endif
            else if ((rat_ptr = RatNumObj::from_string(imag_str)))
#ifndef GMP_SUPPORT
                imag = rat_ptr->a / double(rat_ptr->b);
#else
                imag = rat_ptr->val.get_d();
#endif
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
#ifndef GMP_SUPPORT
                return new CompNumObj(rat->a / double(rat->b), 0);
#else
                return new CompNumObj(rat->val.get_d(), 0);
#endif
                break;
            }
        case NUM_LVL_INT :
#ifndef GMP_SUPPORT
            return new CompNumObj(static_cast<IntNumObj*>(obj)->val, 0);
#else
            return new CompNumObj(static_cast<IntNumObj*>(obj)->val.get_d(), 0);
#endif
    }
    throw NormalError(INT_ERR);
}

#define A (real)
#define B (imag)
#define C (r->real)
#define D (r->imag)

NumObj *CompNumObj::add(NumObj *_r) {
    CompNumObj *r = static_cast<CompNumObj*>(_r);
    return new CompNumObj(A + C, B + D);
}

NumObj *CompNumObj::sub(NumObj *_r) {
    CompNumObj *r = static_cast<CompNumObj*>(_r);
    return new CompNumObj(A - C, B - D);
}

NumObj *CompNumObj::mul(NumObj *_r) {
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
#ifndef GMP_SUPPORT
                return new RealNumObj(rat->a / double(rat->b));
#else
                return new RealNumObj(rat->val.get_d());
#endif
                break;
            }
        case NUM_LVL_INT:
#ifndef GMP_SUPPORT
            return new RealNumObj(static_cast<IntNumObj*>(obj)->val);
#else
            return new RealNumObj(static_cast<IntNumObj*>(obj)->val.get_d());
#endif

    }
    throw NormalError(INT_ERR);
}

NumObj *RealNumObj::add(NumObj *_r) {
    return new RealNumObj(real + static_cast<RealNumObj*>(_r)->real);
}

NumObj *RealNumObj::sub(NumObj *_r) {
    return new RealNumObj(real - static_cast<RealNumObj*>(_r)->real);
}

NumObj *RealNumObj::mul(NumObj *_r) {
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

#ifndef GMP_SUPPORT
RatNumObj::RatNumObj(int _a, int _b) : 
    ExactNumObj(NUM_LVL_RAT), a(_a), b(_b) {
        int g = gcd(a, b);
        a /= g;
        b /= g;
    }

RatNumObj *RatNumObj::from_string(string repr) {
    int a, b;
    size_t len = repr.length();
    int pos = -1;
    for (size_t i = 0; i < len; i++)
        if (repr[i] == '/') { pos = i; break; }
    bool flag;
    a = str_to_int(repr.substr(0, pos), flag);
    if (!flag) return NULL;
    b = str_to_int(repr.substr(pos + 1, len - pos - 1), flag);
    if (!flag) return NULL;

    return new RatNumObj(a, b);
}
#else
RatNumObj::RatNumObj(mpq_class _val) : 
    ExactNumObj(NUM_LVL_RAT), val(_val) {
    val.canonicalize();
}

RatNumObj *RatNumObj::from_string(string repr) {
    try 
    {
        mpq_class ret(repr, 10);
        return new RatNumObj(ret);
    }
    catch (std::invalid_argument &e)
    {
        return NULL;
    }
}
#endif


RatNumObj *RatNumObj::convert(NumObj *obj) {
    switch (obj->level)
    {
        case NUM_LVL_RAT:
            return static_cast<RatNumObj*>(obj); break;
        case NUM_LVL_INT:
#ifndef GMP_SUPPORT
            return new RatNumObj(static_cast<IntNumObj*>(obj)->val, 1);
#else
            return new RatNumObj(mpq_class(
                        static_cast<IntNumObj*>(obj)->val, 
                        mpz_class(1)));
#endif
    }
    throw NormalError(INT_ERR);
}

#define A (a)
#define B (b)
#define C (r->a)
#define D (r->b)

NumObj *RatNumObj::add(NumObj *_r) {
    RatNumObj *r = static_cast<RatNumObj*>(_r);
#ifndef GMP_SUPPORT
    int na = A * D + B * C, nb = B * D;
    int g = gcd(na, nb);
    na /= g;
    nb /= g;
    return new RatNumObj(na, nb);
#else
    return new RatNumObj(val + r->val);
#endif
}

NumObj *RatNumObj::sub(NumObj *_r) {
    RatNumObj *r = static_cast<RatNumObj*>(_r);
#ifndef GMP_SUPPORT
    int na = A * D - B * C, nb = B * D;
    int g = gcd(na, nb);
    na /= g;
    nb /= g;
    return new RatNumObj(na, nb);
#else
    return new RatNumObj(val - r->val);
#endif
}

NumObj *RatNumObj::mul(NumObj *_r) {
    RatNumObj *r = static_cast<RatNumObj*>(_r);
#ifndef GMP_SUPPORT
    int na = A * C, nb = B * D;
    int g = gcd(na, nb);
    na /= g;
    nb /= g;
    return new RatNumObj(na, nb);
#else
    return new RatNumObj(val * r->val);
#endif
}

NumObj *RatNumObj::div(NumObj *_r) {
    RatNumObj *r = static_cast<RatNumObj*>(_r);
#ifndef GMP_SUPPORT
    int na = A * D, nb = B * C;
    int g = gcd(na, nb);
    na /= g;
    nb /= g;
    return new RatNumObj(na, nb);
#else
    return new RatNumObj(val / r->val);
#endif
}

bool RatNumObj::lt(NumObj *_r) {
    RatNumObj *r = static_cast<RatNumObj*>(_r);
#ifndef GMP_SUPPORT
    return A * D < C * B;
#else
    return val < r->val;
#endif
}

bool RatNumObj::gt(NumObj *_r) {
    RatNumObj *r = static_cast<RatNumObj*>(_r);
#ifndef GMP_SUPPORT
    return A * D > C * B;
#else
    return val > r->val;
#endif
}

bool RatNumObj::eq(NumObj *_r) {
    RatNumObj *r = static_cast<RatNumObj*>(_r);
#ifndef GMP_SUPPORT
    return A * D == C * B;
#else
    return val == r->val;
#endif
}

string RatNumObj::ext_repr() {
#ifndef GMP_SUPPORT
    return int_to_str(A) + "/" + int_to_str(B);
#else
    return val.get_str();
#endif
}


#ifndef GMP_SUPPORT
IntNumObj::IntNumObj(int _val) : ExactNumObj(NUM_LVL_INT), val(_val) {}
IntNumObj *IntNumObj::from_string(string repr) {
    int val = 0;
    for (size_t i = 0; i < repr.length(); i++)
    {
        if (!('0' <= repr[i] && repr[i] <= '9'))
            return NULL;
        val = val * 10 + repr[i] - '0';
    }
    return new IntNumObj(val);
}
#else
IntNumObj::IntNumObj(mpz_class _val) : ExactNumObj(NUM_LVL_INT), val(_val) {}
IntNumObj *IntNumObj::from_string(string repr) {
    try 
    {
        mpz_class ret(repr, 10);
        return new IntNumObj(ret);
    }
    catch (std::invalid_argument &e)
    {
        return NULL;
    }
}
#endif

IntNumObj *IntNumObj::convert(NumObj *obj) {
    switch (obj->level)
    {
        case NUM_LVL_INT :
            return static_cast<IntNumObj*>(obj);
        default:
            throw NormalError(INT_ERR);
    }
}

NumObj *IntNumObj::add(NumObj *_r) {
    return new IntNumObj(val + static_cast<IntNumObj*>(_r)->val);
}

NumObj *IntNumObj::sub(NumObj *_r) {
    return new IntNumObj(val - static_cast<IntNumObj*>(_r)->val);
}

NumObj *IntNumObj::mul(NumObj *_r) {
    return new IntNumObj(val * static_cast<IntNumObj*>(_r)->val);
}

NumObj *IntNumObj::div(NumObj *_r) {
#ifndef GMP_SUPPORT
    return new RatNumObj(val, static_cast<IntNumObj*>(_r)->val);
#else
    return new RatNumObj(mpq_class(val,
                static_cast<IntNumObj*>(_r)->val));
#endif
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
#ifndef GMP_SUPPORT
    return int_to_str(val);
#else
    return val.get_str();
#endif
}

SpecialOptIf::SpecialOptIf() : SpecialOptObj("if") {}

void SpecialOptIf::prepare(Cons *pc) {
#define IF_EXP_ERR \
        throw TokenError(name, RUN_ERR_WRONG_NUM_OF_ARGS)
    state = 0;  // Prepared

    if (pc->cdr->is_cons_obj())
        pc = TO_CONS(pc->cdr);
    else 
        IF_EXP_ERR;
//    if (pc == empty_list)
//        IF_EXP_ERR;

    pc->skip = false;

    if (pc->cdr->is_cons_obj())
        pc = TO_CONS(pc->cdr);
    else
        IF_EXP_ERR;
 //   if (pc == empty_list)
 //       IF_EXP_ERR;

    pc->skip = true;
    if (pc->cdr != empty_list)
    {
        if (pc->cdr->is_cons_obj())
        {
            TO_CONS(pc->cdr)->skip = true;
            if (TO_CONS(pc->cdr)->cdr != empty_list)
                IF_EXP_ERR;
        }
        else
            IF_EXP_ERR;
    }
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
        pc->skip = true;
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
        return TO_CONS(ret_addr->car)->next;
    }
}

string SpecialOptIf::ext_repr() { return string("#<Builtin Macro: if>"); }

SpecialOptLambda::SpecialOptLambda() : SpecialOptObj("lambda") {}
#define FILL_MARKS(pc, flag)  \
do  \
{ \
    EvalObj *nptr; \
    Cons *ptr; \
    for (ptr = TO_CONS(pc->cdr);;)  \
    { \
        ptr->skip = flag; \
        if ((nptr = ptr->cdr)->is_cons_obj()) \
            ptr = TO_CONS(nptr); \
        else break; \
    } \
    if (ptr->cdr != empty_list) \
        throw TokenError(name, RUN_ERR_WRONG_NUM_OF_ARGS); \
} \
while (0)

#define CHECK_PARA_LIST(p) \
do  \
{ \
    if (p == empty_list) break; \
    EvalObj *nptr; \
    Cons *ptr; \
    for (ptr = TO_CONS(p);;)  \
    { \
        if (!ptr->car->is_sym_obj()) \
            throw TokenError(ptr->car->ext_repr(), RUN_ERR_WRONG_TYPE); \
        if ((nptr = ptr->cdr)->is_cons_obj()) \
            ptr = TO_CONS(nptr); \
        else break; \
    } \
    if (ptr->cdr != empty_list) \
        throw TokenError(name, RUN_ERR_WRONG_NUM_OF_ARGS); \
} \
while (0)


void SpecialOptLambda::prepare(Cons *pc) {
    // Do not evaluate anything
    FILL_MARKS(pc, true);
}

Cons *SpecialOptLambda::call(ArgList *args, Environment * &envt, 
        Continuation * &cont, FrameObj ** &top_ptr) {

    Cons *ret_addr = static_cast<RetAddr*>(*top_ptr)->addr;
    Cons *pc = static_cast<Cons*>(ret_addr->car);
    // TODO: remove the following two lines?
    if (pc->cdr == empty_list)
        throw TokenError(name, SYN_ERR_EMPTY_PARA_LIST);
    if (TO_CONS(pc->cdr)->cdr == empty_list)
        throw TokenError(name, SYN_ERR_MISS_OR_EXTRA_EXP);

     // Clear the flag to avoid side-effects (e.g. proc calling)
    FILL_MARKS(pc, false);
   
    pc = TO_CONS(pc->cdr);
    CHECK_PARA_LIST(pc->car);
    SymbolList *para_list = static_cast<SymbolList*>(pc->car); 

    // store a list of expressions inside <body>

    ASTList *body = TO_CONS(pc->cdr);       // Truncate the expression list
    for (Cons *ptr = body; ptr != empty_list; ptr = TO_CONS(ptr->cdr))
        ptr->next = NULL;    // Make each expression an orphan

    *top_ptr++ = new ProcObj(body, envt, para_list);
    return ret_addr->next;  // Move to the next instruction
}

string SpecialOptLambda::ext_repr() { return string("#<Builtin Macro: lambda>"); }

SpecialOptDefine::SpecialOptDefine() : SpecialOptObj("define") {}

void SpecialOptDefine::prepare(Cons *pc) {
    if (!pc->cdr->is_cons_obj())
        throw TokenError(name, RUN_ERR_WRONG_NUM_OF_ARGS);

    if (TO_CONS(pc->cdr)->car->is_simple_obj())  // Simple value assignment
    {
        pc = TO_CONS(pc->cdr);
        if (!pc->cdr->is_cons_obj())
            throw TokenError(name, RUN_ERR_WRONG_NUM_OF_ARGS);
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
    EvalObj *first = TO_CONS(pc->cdr)->car;
    if (first->is_simple_obj())
    {
        if (!first->is_sym_obj())
            throw TokenError(first->ext_repr(), SYN_ERR_NOT_AN_ID);
        ARGS_EXACTLY_TWO;
        id = static_cast<SymObj*>(first);
        obj = TO_CONS(args->cdr)->car;
    }
    else
    {
        // static_cast because of is_simple_obj() is false
        Cons *plst = static_cast<Cons*>(first);

        if (plst == empty_list)
            throw TokenError(name, SYN_ERR_EMPTY_PARA_LIST);
        if (!plst->car->is_sym_obj())
            throw TokenError(first->ext_repr(), SYN_ERR_NOT_AN_ID);
        CHECK_PARA_LIST(plst->cdr);

        id = static_cast<SymObj*>(plst->car);
        ArgList *para_list = TO_CONS(plst->cdr);
        // Clear the flag to avoid side-effects (e.g. proc calling)
        FILL_MARKS(pc, false);

        ASTList *body = TO_CONS(TO_CONS(pc->cdr)->cdr);   // Truncate the expression list

        if (body == empty_list)
            throw TokenError(name, SYN_ERR_MISS_OR_EXTRA_EXP);

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
    if (!pc->cdr->is_cons_obj())
        throw TokenError(name, RUN_ERR_WRONG_NUM_OF_ARGS);

    pc = TO_CONS(pc->cdr);

    pc->skip = true;       // Skip the identifier

    if (!pc->cdr->is_cons_obj())
        throw TokenError(name, RUN_ERR_WRONG_NUM_OF_ARGS);

    pc = TO_CONS(pc->cdr);

    pc->skip = false; 
}

Cons *SpecialOptSet::call(ArgList *args, Environment * &envt, 
        Continuation * &cont, FrameObj ** &top_ptr) {
    Cons *ret_addr = static_cast<RetAddr*>(*top_ptr)->addr;
    Cons *pc = static_cast<Cons*>(ret_addr->car);
    EvalObj *first = TO_CONS(pc->cdr)->car;

    if (!first->is_sym_obj())
        throw TokenError(first->ext_repr(), SYN_ERR_NOT_AN_ID);
    ARGS_EXACTLY_TWO;

    SymObj *id = static_cast<SymObj*>(first);

    bool flag = envt->add_binding(id, TO_CONS(args->cdr)->car, false);
    if (!flag) throw TokenError(id->ext_repr(), RUN_ERR_UNBOUND_VAR);
    *top_ptr++ = new UnspecObj();
    return ret_addr->next;
}

SpecialOptSet::SpecialOptSet() : SpecialOptObj("set!") {}

string SpecialOptSet::ext_repr() { return string("#<Builtin Macro: set!>"); }

SpecialOptQuote::SpecialOptQuote() : SpecialOptObj("quote") {}

void SpecialOptQuote::prepare(Cons *pc) {
    // Do not evaluate anything
    FILL_MARKS(pc, true);
}

Cons *SpecialOptQuote::call(ArgList *args, Environment * &envt, 
        Continuation * &cont, FrameObj ** &top_ptr) {
    Cons *ret_addr = static_cast<RetAddr*>(*top_ptr)->addr;
    Cons *pc = static_cast<Cons*>(ret_addr->car);
    // revert
    FILL_MARKS(pc, false);
    *top_ptr++ = TO_CONS(pc->cdr)->car;
    return ret_addr->next;
}

string SpecialOptQuote::ext_repr() { return string("#<Builtin Macro: quote>"); }

SpecialOptEval::SpecialOptEval() : SpecialOptObj("eval") {}

void SpecialOptEval::prepare(Cons *pc) {
    state = 0;
}

Cons *SpecialOptEval::call(ArgList *args, Environment * &envt, 
        Continuation * &cont, FrameObj ** &top_ptr) {
    if (args->cdr == empty_list ||
        TO_CONS(args->cdr)->cdr != empty_list)
        throw TokenError(name, RUN_ERR_WRONG_NUM_OF_ARGS);
    Cons *ret_addr = static_cast<RetAddr*>(*top_ptr)->addr;
    Cons *pc = static_cast<Cons*>(ret_addr->car);
    if (state)
    {
        *top_ptr++ = TO_CONS(args->cdr)->car;
        return ret_addr->next;          // Move to the next instruction
    }
    else
    {
        state = 1;
        top_ptr += 2;
        return TO_CONS(args->cdr);
    }
}

string SpecialOptEval::ext_repr() { return string("#<Builtin Macro: eval>"); }

BUILTIN_PROC_DEF(make_pair) {
    ARGS_EXACTLY_TWO;
    return new Cons(args->car, TO_CONS(args->cdr)->car);
}

BUILTIN_PROC_DEF(pair_car) {
    ARGS_EXACTLY_ONE;
    if (!args->car->is_cons_obj())
        throw TokenError("pair", RUN_ERR_WRONG_TYPE);

    return TO_CONS(args->car)->car;
}

BUILTIN_PROC_DEF(pair_cdr) {
    ARGS_EXACTLY_ONE;
    if (!args->car->is_cons_obj())
        throw TokenError("pair", RUN_ERR_WRONG_TYPE);

    return TO_CONS(args->car)->cdr;
}

BUILTIN_PROC_DEF(make_list) {
    if (!is_list(args))
        throw TokenError(name, RUN_ERR_WRONG_NUM_OF_ARGS); 
    return args;
}

BUILTIN_PROC_DEF(num_add) {
    ARGS_AT_LEAST_ONE;
    NumObj *res = new IntNumObj(0), *opr; // the most accurate type
    EvalObj *nptr;
    for (;;)
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
        
        if ((nptr = args->cdr)->is_cons_obj())
            args = TO_CONS(nptr);
        else break;
    }
    if (args->cdr != empty_list)
        throw TokenError(name, RUN_ERR_WRONG_NUM_OF_ARGS);
    return res; 
}

BUILTIN_PROC_DEF(num_sub) {
    ARGS_AT_LEAST_ONE;
    if (!args->car->is_num_obj())
        throw TokenError("a number", RUN_ERR_WRONG_TYPE);

    NumObj *res = static_cast<NumObj*>(args->car), *opr;
    EvalObj *nptr; 
    for (;;)
    {
        if ((nptr = args->cdr)->is_cons_obj())
            args = TO_CONS(nptr);
        else break;        

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
    if (args->cdr != empty_list)
        throw TokenError(name, RUN_ERR_WRONG_NUM_OF_ARGS);
    return res; 
}

BUILTIN_PROC_DEF(num_mul) {
    ARGS_AT_LEAST_ONE;
    NumObj *res = new IntNumObj(1), *opr; // the most accurate type
    EvalObj *nptr;
    for (;;)
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
        
        if ((nptr = args->cdr)->is_cons_obj())
            args = TO_CONS(nptr);
        else break;
    }
    if (args->cdr != empty_list)
        throw TokenError(name, RUN_ERR_WRONG_NUM_OF_ARGS);
    return res; 
}

BUILTIN_PROC_DEF(num_div) {
    ARGS_AT_LEAST_ONE;
    if (!args->car->is_num_obj())
        throw TokenError("a number", RUN_ERR_WRONG_TYPE);

    NumObj *res = static_cast<NumObj*>(args->car), *opr;
    EvalObj *nptr; 
    for (;;)
    {
        if ((nptr = args->cdr)->is_cons_obj())
            args = TO_CONS(nptr);
        else break;        

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
    if (args->cdr != empty_list)
        throw TokenError(name, RUN_ERR_WRONG_NUM_OF_ARGS);
    return res; 
}

BUILTIN_PROC_DEF(num_lt) {
    if (args == empty_list)
        return new BoolObj(true);
    // zero arguments
    if (!args->car->is_num_obj())
        throw TokenError("a number", RUN_ERR_WRONG_TYPE);

    NumObj *last = static_cast<NumObj*>(args->car), *opr; 
    EvalObj *nptr;
    
    for (;; last = opr)
    {
        if ((nptr = args->cdr)->is_cons_obj())
            args = TO_CONS(nptr);
        else break;              
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
    if (args->cdr != empty_list)
        throw TokenError(name, RUN_ERR_WRONG_NUM_OF_ARGS);
    return new BoolObj(true);
}

BUILTIN_PROC_DEF(num_gt) {
    if (args == empty_list)
        return new BoolObj(true);
    // zero arguments
    if (!args->car->is_num_obj())
        throw TokenError("a number", RUN_ERR_WRONG_TYPE);

    NumObj *last = static_cast<NumObj*>(args->car), *opr; 
    EvalObj *nptr;
    
    for (;; last = opr)
    {
        if ((nptr = args->cdr)->is_cons_obj())
            args = TO_CONS(nptr);
        else break;              
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
    if (args->cdr != empty_list)
        throw TokenError(name, RUN_ERR_WRONG_NUM_OF_ARGS);
    return new BoolObj(true);
}

BUILTIN_PROC_DEF(num_eq) {
    if (args == empty_list)
        return new BoolObj(true);
    // zero arguments
    if (!args->car->is_num_obj())
        throw TokenError("a number", RUN_ERR_WRONG_TYPE);

    NumObj *last = static_cast<NumObj*>(args->car), *opr; 
    EvalObj *nptr;
    
    for (;; last = opr)
    {
        if ((nptr = args->cdr)->is_cons_obj())
            args = TO_CONS(nptr);
        else break;              
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
    if (args->cdr != empty_list)
        throw TokenError(name, RUN_ERR_WRONG_NUM_OF_ARGS);
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
    return new BoolObj(args->car->is_cons_obj());
}

BUILTIN_PROC_DEF(pair_set_car) {
    ARGS_EXACTLY_TWO;
    if (!args->car->is_cons_obj())
        throw TokenError("pair", RUN_ERR_WRONG_TYPE);
    TO_CONS(args->car)->car = TO_CONS(args->cdr)->car;
    return new UnspecObj();
}

BUILTIN_PROC_DEF(pair_set_cdr) {
    ARGS_EXACTLY_TWO;
    if (!args->car->is_cons_obj())
        throw TokenError("pair", RUN_ERR_WRONG_TYPE);
    TO_CONS(args->car)->cdr = TO_CONS(args->cdr)->car;
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
    if (!args->car->is_cons_obj())
        return new BoolObj(false);
    args = TO_CONS(args->car);
    EvalObj *nptr;
    for (;;)
    {
        if ((nptr = args->cdr)->is_cons_obj())
            args = TO_CONS(nptr);
        else break;
    }
    return new BoolObj(args->cdr == empty_list);
}

BUILTIN_PROC_DEF(num_exact) {
    ARGS_EXACTLY_ONE;
    if (!args->car->is_num_obj())
        throw TokenError("a number", RUN_ERR_WRONG_TYPE);
    return new BoolObj(static_cast<NumObj*>(args->car)->is_exact()); 
}

BUILTIN_PROC_DEF(num_inexact) {
    ARGS_EXACTLY_ONE;
    if (!args->car->is_num_obj())
        throw TokenError("a number", RUN_ERR_WRONG_TYPE);
    return new BoolObj(!static_cast<NumObj*>(args->car)->is_exact()); 
}


BUILTIN_PROC_DEF(display) {
    ARGS_EXACTLY_ONE;
    printf("%s\n", args->car->ext_repr().c_str());
    return new UnspecObj();
}
