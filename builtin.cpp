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
        long long spos = -1, ipos = -1;
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
            if ((int_ptr = IntNumObj::from_string(real_str)))
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
            if ((int_ptr = IntNumObj::from_string(imag_str)))
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


ReprCons *CompNumObj::get_repr_cons() {
    return new ReprStr(double_to_str(real) + double_to_str(imag, true) + "i");
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

ReprCons *RealNumObj::get_repr_cons() {
    return new ReprStr(double_to_str(real));
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

ReprCons *RatNumObj::get_repr_cons() {
#ifndef GMP_SUPPORT
    return new ReprStr(int_to_str(A) + "/" + int_to_str(B));
#else
    return new ReprStr(val.get_str());
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
int IntNumObj::get_i() { return val; }
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
int IntNumObj::get_i() { return val.get_si(); }
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

ReprCons *IntNumObj::get_repr_cons() {
#ifndef GMP_SUPPORT
    return new ReprStr(int_to_str(val));
#else
    return new ReprStr(val.get_str());
#endif
}

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

void SpecialOptIf::pre_call(ArgList *args, Pair *pc,
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

EvalObj *SpecialOptIf::post_call(ArgList *args, Pair *pc,
        Environment *envt) {
    // Value already evaluated, so just return it
    return TO_PAIR(args->cdr)->car;
}

Pair *SpecialOptIf::call(ArgList *args, Environment * &envt, 
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

Pair *SpecialOptLambda::call(ArgList *args, Environment * &envt, 
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

Pair *SpecialOptDefine::call(ArgList *args, Environment * &envt, 
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

Pair *SpecialOptSet::call(ArgList *args, Environment * &envt, 
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

Pair *SpecialOptQuote::call(ArgList *args, Environment * &envt, 
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

Pair *SpecialOptEval::call(ArgList *args, Environment * &envt, 
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

Pair *SpecialOptAnd::call(ArgList *args, Environment * &envt, 
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
    ARGS_AT_LEAST_ONE;
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
    ARGS_AT_LEAST_ONE;
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

BUILTIN_PROC_DEF(num_lt) {
    if (args == empty_list)
        return new BoolObj(true);
    // zero arguments
    if (!args->car->is_num_obj())
        throw TokenError("a number", RUN_ERR_WRONG_TYPE);

    NumObj *last = static_cast<NumObj*>(args->car), *opr; 
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
        if (a == empty_list)
            continue;
        if (otype & CLS_PAIR_OBJ)
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
                *r1 = TO_PAIR(a)->car;
                INC1(r1);
                CHK1;
            }

            for (EvalObjVec::iterator 
                    it = vb->vec.begin(); 
                    it != vb->vec.end(); it++)
            {
                *r2 = TO_PAIR(b)->car;
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
    }
    return new BoolObj(true);
}

BUILTIN_PROC_DEF(display) {
    ARGS_EXACTLY_ONE;
    printf("%s\n", args->car->ext_repr().c_str());
    return new UnspecObj();
}
