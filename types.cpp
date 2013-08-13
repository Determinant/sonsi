#include "types.h"
#include "model.h"
#include "exc.h"
#include "consts.h"
#include "gc.h"

#include <cmath>
#include <cstdlib>
#include <sstream>
#include <iomanip>

const double EPS = 1e-16;
const int PREC = 16;

extern EmptyList *empty_list;
extern UnspecObj *unspec_obj;

Pair::Pair(EvalObj *_car, EvalObj *_cdr) : 
Container(CLS_PAIR_OBJ), car(_car), cdr(_cdr), next(NULL) {

    gc.attach(car);
    gc.attach(cdr);
}

Pair::~Pair() {
    gc.expose(car);
    gc.expose(cdr);
}

void Pair::gc_decrement() {
    GC_CYC_DEC(car);
    GC_CYC_DEC(cdr);
}

void Pair::gc_trigger(EvalObj ** &tail, EvalObjSet &visited) {
    GC_CYC_TRIGGER(car);
    GC_CYC_TRIGGER(cdr);
}

ReprCons *Pair::get_repr_cons() {
    return new PairReprCons(this, this);
}

ParseBracket::ParseBracket(unsigned char _btype) :
    FrameObj(CLS_SIM_OBJ | CLS_PAR_BRA), btype(_btype) {}

    UnspecObj::UnspecObj() : EvalObj(CLS_SIM_OBJ) {}

    ReprCons *UnspecObj::get_repr_cons() {
        return new ReprStr("#<Unspecified>");
    }

SymObj::SymObj(const string &str) :
    EvalObj(CLS_SIM_OBJ | CLS_SYM_OBJ), val(str) {}

    ReprCons *SymObj::get_repr_cons() {
        return new ReprStr(val);
    }

OptObj::OptObj(int otype) : Container(otype | CLS_SIM_OBJ | CLS_OPT_OBJ, true) {}

void OptObj::gc_decrement() {}
void OptObj::gc_trigger(EvalObj ** &tail, EvalObjSet &visited) {}

ProcObj::ProcObj(Pair *_body, Environment *_envt, EvalObj *_params) :
    OptObj(CLS_CONTAINER), body(_body), params(_params), envt(_envt) {
    gc.attach(body);
    gc.attach(params);
    gc.attach(envt);
}

ProcObj::~ProcObj() {
    gc.expose(body);
    gc.expose(params);
    gc.expose(envt);
}

Pair *ProcObj::call(Pair *_args, Environment * &genvt,
        Continuation * &cont, FrameObj ** &top_ptr) {
    // Create a new continuation
    // static_cast see `call` invocation in eval.cpp
    Pair *ret_addr = static_cast<RetAddr*>(*top_ptr)->addr;
    Continuation *_cont = new Continuation(genvt, ret_addr, cont, body);
    // Create local env and recall the closure
    Environment *_envt = new Environment(envt);
    // static_cast<SymObj*> because the params is already checked
    EvalObj *ppar, *nptr;
    Pair *args = _args;
    for (ppar = params;
            ppar->is_pair_obj();
            ppar = TO_PAIR(ppar)->cdr)
    {
        if ((nptr = args->cdr) != empty_list)
            args = TO_PAIR(nptr);
        else break;
        _envt->add_binding(static_cast<SymObj*>(TO_PAIR(ppar)->car), args->car);
    }

    if (ppar->is_sym_obj())
        _envt->add_binding(static_cast<SymObj*>(ppar), args->cdr); // (... . var_n)
    else if (args->cdr != empty_list || ppar != empty_list)
        throw TokenError("", RUN_ERR_WRONG_NUM_OF_ARGS);

    gc.expose(genvt);
    genvt = _envt;
    gc.attach(genvt);

    gc.expose(cont);
    cont = _cont;
    gc.attach(cont);

    delete *top_ptr;                    // release ret addr
    *top_ptr++ = new RetAddr(NULL);     // Mark the entrance of a cont
    gc.expose(_args);
    return body;                        // Move pc to the proc entry point
}

void ProcObj::gc_decrement() {
    GC_CYC_DEC(body);
    GC_CYC_DEC(params);
    GC_CYC_DEC(envt);
}

void ProcObj::gc_trigger(EvalObj ** &tail, EvalObjSet &visited) {
    GC_CYC_TRIGGER(body);
    GC_CYC_TRIGGER(params);
    GC_CYC_TRIGGER(envt);
}

ReprCons *ProcObj::get_repr_cons() {
    return new ReprStr("#<Procedure>");
}

SpecialOptObj::SpecialOptObj(string _name) : OptObj(), name(_name) {}
ReprCons *SpecialOptObj::get_repr_cons() {
    return new ReprStr("#<Built-in Opt: " + name + ">");
}

BoolObj::BoolObj(bool _val) : EvalObj(CLS_SIM_OBJ | CLS_BOOL_OBJ), val(_val) {}

bool BoolObj::is_true() { return val; }

ReprCons *BoolObj::get_repr_cons() {
    return new ReprStr(val ? "#t" : "#f");
}

BoolObj *BoolObj::from_string(string repr) {
    if (repr.length() != 2 || repr[0] != '#')
        return NULL;
    if (repr[1] == 't')
        return new BoolObj(true);
    else if (repr[1] == 'f')
        return new BoolObj(false);
    return NULL;
}

NumObj::NumObj(NumLvl _level, bool _exactness) :
    EvalObj(CLS_SIM_OBJ | CLS_NUM_OBJ), exactness(_exactness), level(_level) {}

    bool NumObj::is_exact() { return exactness; }

    StrObj::StrObj(string _str) : EvalObj(CLS_SIM_OBJ | CLS_STR_OBJ), str(_str) {}

    ReprCons *StrObj::get_repr_cons() {
        return new ReprStr(str);
    }

CharObj::CharObj(char _ch) : EvalObj(CLS_SIM_OBJ | CLS_CHAR_OBJ), ch(_ch) {}

CharObj *CharObj::from_string(string repr) {
    size_t len = repr.length();
    if (len < 2) return NULL;
    if (repr[0] != '#' || repr[1] != '\\') return NULL;
    if (len == 3) return new CharObj(repr[2]);
    string char_name = repr.substr(2, len - 2);
    if (char_name == "newline") return new CharObj('\n');
    if (char_name == "space") return new CharObj(' ');
    throw TokenError(char_name, RUN_ERR_UNKNOWN_CHAR_NAME);
}

ReprCons *CharObj::get_repr_cons() {
    string val = "";
    if (ch == ' ') val = "space";
    else if (ch == '\n') val = "newline";
    else val += ch;
    return new ReprStr("#\\" + val);
}

VecObj::VecObj(size_t size, EvalObj *fill) :
    Container(CLS_SIM_OBJ | CLS_VECT_OBJ) {
    vec.resize(size);
    for (size_t i = 0; i < size; i++)
    {
        vec[i] = fill;
        gc.attach(fill);
    }
}

VecObj::~VecObj() {
    for (EvalObjVec::iterator it = vec.begin();
            it != vec.end(); it++)
        gc.expose(*it);
}

EvalObj *VecObj::get(size_t idx) {
    return vec[idx];
}

void VecObj::set(size_t idx, EvalObj *obj) {
    if (idx >= get_size())
        throw NormalError(RUN_ERR_VALUE_OUT_OF_RANGE);
    gc.expose(vec[idx]);
    vec[idx] = obj;
    gc.attach(obj);
}

size_t VecObj::get_size() {
    return vec.end() - vec.begin();
}

void VecObj::push_back(EvalObj *new_elem) {
    gc.attach(new_elem);
    vec.push_back(new_elem);
}

void VecObj::fill(EvalObj *obj) {
    for (EvalObjVec::iterator it = vec.begin();
            it != vec.end(); it++)
    {
        gc.expose(*it);
        *it = obj;
        gc.attach(obj);
    }
}

ReprCons *VecObj::get_repr_cons() {
    return new VectReprCons(this, this);
}

void VecObj::gc_decrement() {
    for (EvalObjVec::iterator it = vec.begin();
            it != vec.end(); it++)
        GC_CYC_DEC(*it);
}

void VecObj::gc_trigger(EvalObj ** &tail, EvalObjSet &visited) {
    for (EvalObjVec::iterator it = vec.begin();
            it != vec.end(); it++)
        GC_CYC_TRIGGER(*it);
}

StrObj *StrObj::from_string(string repr) {
    size_t len = repr.length();
    if (repr[0] == '\"' && repr[len - 1] == '\"')
        return new StrObj(repr.substr(1, len - 2));
    return NULL;
}

bool StrObj::lt(StrObj *r) {
    return str < r->str;
}

bool StrObj::gt(StrObj *r) {
    return str > r->str;
}

bool StrObj::le(StrObj *r) {
    return str <= r->str;
}

bool StrObj::ge(StrObj *r) {
    return str >= r->str;
}

bool StrObj::eq(StrObj *r) {
    return str == r->str;
}

BuiltinProcObj::BuiltinProcObj(BuiltinProc f, string _name) :
    OptObj(), handler(f), name(_name) {}

    Pair *BuiltinProcObj::call(Pair *args, Environment * &envt,
            Continuation * &cont, FrameObj ** &top_ptr) {

        Pair *ret_addr = static_cast<RetAddr*>(*top_ptr)->addr;
        delete *top_ptr;
        *top_ptr++ = gc.attach(handler(TO_PAIR(args->cdr), name));
        gc.expose(args);
        return ret_addr->next;          // Move to the next instruction
    }

ReprCons *BuiltinProcObj::get_repr_cons() {
    return new ReprStr("#<Builtin Procedure: " + name + ">");
}

Environment::Environment(Environment *_prev_envt) : 
    Container(), prev_envt(_prev_envt) {
    gc.attach(prev_envt);
}

Environment::~Environment() {
    for (Str2EvalObj::iterator it = binding.begin();
            it != binding.end(); it++)
        gc.expose(it->second);
    gc.expose(prev_envt);
}

ReprCons *Environment::get_repr_cons() {
    return new ReprStr("#<Environment>");
}

void Environment::gc_decrement() {
    GC_CYC_DEC(prev_envt);
    for (Str2EvalObj::iterator it = binding.begin();
            it != binding.end(); it++)
        GC_CYC_DEC(it->second);
}

void Environment::gc_trigger(EvalObj ** &tail, EvalObjSet &visited) {
    GC_CYC_TRIGGER(prev_envt);
    for (Str2EvalObj::iterator it = binding.begin();
            it != binding.end(); it++)
        GC_CYC_TRIGGER(it->second);
}

bool Environment::add_binding(SymObj *sym_obj, EvalObj *eval_obj, bool def) {
    bool found = false;
    string name(sym_obj->val);
    if (!def)
    {
        for (Environment *ptr = this; ptr; ptr = ptr->prev_envt)
        {
            bool has_key = ptr->binding.count(name);
            if (has_key)
            {
                EvalObj * &ref = ptr->binding[name];
                gc.expose(ref);
                ref = eval_obj;
                gc.attach(ref);
                found = true;
                break;
            }
        }
        return found;
    }
    else
    {
        if (!binding.count(name))
        {
            binding[name] = eval_obj;
            gc.attach(eval_obj);
        }
        else
        {
            EvalObj * &ref = binding[name];
            gc.expose(ref);
            ref = eval_obj;
            gc.attach(ref);
        }
        return true;
    }
}

EvalObj *Environment::get_obj(EvalObj *obj) {
    if (!obj->is_sym_obj()) return obj;
    SymObj *sym_obj = static_cast<SymObj*>(obj);

    string name(sym_obj->val);
    for (Environment *ptr = this; ptr; ptr = ptr->prev_envt)
    {
        bool has_key = ptr->binding.count(name);
        if (has_key) return ptr->binding[name];
    }
    // Object not found
    throw TokenError(name, RUN_ERR_UNBOUND_VAR);
}

Continuation::Continuation(Environment *_envt, Pair *_pc,
        Continuation *_prev_cont, Pair *_proc_body) :
    Container(), prev_cont(_prev_cont), envt(_envt), pc(_pc), proc_body(_proc_body) {
        gc.attach(prev_cont);
        gc.attach(envt);
    }

Continuation::~Continuation() {
    gc.expose(prev_cont);
    gc.expose(envt);
}

void Continuation::gc_decrement() {
    GC_CYC_DEC(prev_cont);
    GC_CYC_DEC(envt);
}

void Continuation::gc_trigger(EvalObj ** &tail, EvalObjSet &visited) {
    GC_CYC_TRIGGER(prev_cont);
    GC_CYC_TRIGGER(envt);
}

ReprCons *Continuation::get_repr_cons() {
    return new ReprStr("#<Continuation>");
}

ReprCons::ReprCons(bool _done, EvalObj *_ori) : ori(_ori), done(_done) {}
ReprStr::ReprStr(string _repr) : ReprCons(true) { repr = _repr; }
EvalObj *ReprStr::next(const string &prev) {
    throw NormalError(INT_ERR);
}

PairReprCons::PairReprCons(Pair *_ptr, EvalObj *_ori) :
    ReprCons(false, _ori), state(0), ptr(_ptr) {}

    EvalObj *PairReprCons::next(const string &prev) {
        repr += prev;
        EvalObj *res;
        if (state == 0)
        {
            state = 1;
            res = TO_PAIR(ptr)->car;
            if (res->is_pair_obj())
                repr += "(";
            return res;
        }
        else if (state == 1)
        {
            state = 2;
            if (TO_PAIR(ptr)->car->is_pair_obj())
                repr += ")";
            ptr = TO_PAIR(ptr)->cdr;
            if (ptr == empty_list)
                return NULL;
            repr += " ";
            if (ptr->is_simple_obj())
                repr += ". ";
            return ptr;
        }
        else
        {
            return NULL;
        }
    }

VectReprCons::VectReprCons(VecObj *_ptr, EvalObj *_ori) :
    ReprCons(false, _ori), ptr(_ptr), idx(0) { repr = "#("; }

    EvalObj *VectReprCons::next(const string &prev) {
        repr += prev;

        if (idx && ptr->get(idx - 1)->is_pair_obj())
            repr += ")";

        if (idx == ptr->get_size())
        {
            repr += ")";
            return NULL;
        }
        else
        {
            if (idx) repr += " ";
            EvalObj *res = ptr->get(idx++);
            if (res->is_pair_obj())
                repr += "(";
            return res;
        }
    }

PromObj::PromObj(EvalObj *exp) :
    Container(CLS_SIM_OBJ | CLS_PROM_OBJ),
    entry(new Pair(exp, empty_list)), mem(NULL) {
        gc.attach(entry);
        entry->next = NULL;
    }

PromObj::~PromObj() {
    gc.expose(entry);
    gc.expose(mem);
}

void PromObj::gc_decrement() {
    GC_CYC_DEC(entry);
    GC_CYC_DEC(mem);
}

void PromObj::gc_trigger(EvalObj ** &tail, EvalObjSet &visited) {
    GC_CYC_TRIGGER(entry);
    GC_CYC_TRIGGER(mem);
}

Pair *PromObj::get_entry() { return entry; }

ReprCons *PromObj::get_repr_cons() { return new ReprStr("#<Promise>"); }

EvalObj *PromObj::get_mem() { return mem; }

void PromObj::feed_mem(EvalObj *res) {
    gc.attach(mem = res);
}


string double_to_str(double val, bool force_sign = false) {
    std::stringstream ss;
    if (force_sign) ss << std::showpos;
    ss << std::setprecision(PREC);
    ss << val;
    return ss.str();
}

string int_to_str(int val) {
    std::stringstream ss;
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

bool is_zero(double x) {
    return -EPS < x && x < EPS;
}

InexactNumObj::InexactNumObj(NumLvl level) : NumObj(level, false) {}

CompNumObj::CompNumObj(double _real, double _imag) :
    InexactNumObj(NUM_LVL_COMP), real(_real), imag(_imag) {}

    CompNumObj *CompNumObj::from_string(string repr) {
        // spos: the position of the last sign
        // ipos: the position of i
        long long spos = -1, ipos = -1;
        size_t len = repr.length();
        bool sign = false;
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

NumObj *CompNumObj::clone() const {
    return new CompNumObj(*this);
}

CompNumObj *CompNumObj::convert(NumObj *obj) {
    switch (obj->level)
    {
        case NUM_LVL_COMP :
            return new CompNumObj(*static_cast<CompNumObj*>(obj));
            break;
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

void CompNumObj::add(NumObj *_r) {
    CompNumObj *r = static_cast<CompNumObj*>(_r);
    real += C;
    imag += D;
}

void CompNumObj::sub(NumObj *_r) {
    CompNumObj *r = static_cast<CompNumObj*>(_r);
    real -= C;
    imag -= D;
}

void CompNumObj::mul(NumObj *_r) {
    CompNumObj *r = static_cast<CompNumObj*>(_r);
    double ra = A * C - B * D;
    double rb = B * C + A * D;
    A = ra;
    B = rb;
}

void CompNumObj::div(NumObj *_r) {
    CompNumObj *r = static_cast<CompNumObj*>(_r);
    double f = C * C + D * D;
    if (f == 0)
        throw NormalError(RUN_ERR_NUMERIC_OVERFLOW);
    f = 1 / f;
    double ra = (A * C + B * D) * f;
    double rb = (B * C - A * D) * f;
    A = ra;
    B = rb;
}

bool NumObj::lt(NumObj *_r) {
    throw TokenError("a comparable number", RUN_ERR_WRONG_TYPE);
}

bool NumObj::gt(NumObj *_r) {
    throw TokenError("a comparable number", RUN_ERR_WRONG_TYPE);
}

bool NumObj::le(NumObj *_r) {
    throw TokenError("a comparable number", RUN_ERR_WRONG_TYPE);
}

bool NumObj::ge(NumObj *_r) {
    throw TokenError("a comparable number", RUN_ERR_WRONG_TYPE);
}

void NumObj::abs() {
    throw TokenError("a real number", RUN_ERR_WRONG_TYPE);
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

NumObj *RealNumObj::clone() const {
    return new RealNumObj(*this);
}

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
            return new RealNumObj(*static_cast<RealNumObj*>(obj));
            break;
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

void RealNumObj::add(NumObj *_r) {
    real += static_cast<RealNumObj*>(_r)->real;
}

void RealNumObj::sub(NumObj *_r) {
    real -= static_cast<RealNumObj*>(_r)->real;
}

void RealNumObj::mul(NumObj *_r) {
    real *= static_cast<RealNumObj*>(_r)->real;
}

void RealNumObj::div(NumObj *_r) {
    real /= static_cast<RealNumObj*>(_r)->real;
}

void RealNumObj::abs() {
    real = fabs(real);
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

bool RealNumObj::le(NumObj *_r) {
    return real <= static_cast<RealNumObj*>(_r)->real;
}

bool RealNumObj::ge(NumObj *_r) {
    return real >= static_cast<RealNumObj*>(_r)->real;
}


ReprCons *RealNumObj::get_repr_cons() {
    return new ReprStr(double_to_str(real));
}

ExactNumObj::ExactNumObj(NumLvl level) : NumObj(level, true) {}

#ifndef GMP_SUPPORT
RatNumObj::RatNumObj(int _a, int _b) :
    ExactNumObj(NUM_LVL_RAT), a(_a), b(_b) {
        if (b == 0)
            throw NormalError(RUN_ERR_NUMERIC_OVERFLOW);
        if (b < 0) a = -a, b = -b;
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
RatNumObj::RatNumObj(mpq_class _val) : ExactNumObj(NUM_LVL_RAT), val(_val) {
        val.canonicalize();
}

NumObj *RatNumObj::clone() const {
    return new RatNumObj(*this);
}

RatNumObj *RatNumObj::from_string(string repr) {
    try
    {
        mpq_class ret(repr, 10);
        if (ret.get_den() == 0)
            throw NormalError(RUN_ERR_NUMERIC_OVERFLOW);
        ret.canonicalize();
        return new RatNumObj(ret);
    }
    catch (std::invalid_argument &e)
    {
        return NULL;
    }
}

RatNumObj::RatNumObj(const RatNumObj &ori) :
    ExactNumObj(NUM_LVL_RAT), val(ori.val.get_mpq_t()) {}
#endif


RatNumObj *RatNumObj::convert(NumObj *obj) {
    switch (obj->level)
    {
        case NUM_LVL_RAT:
            return new RatNumObj(*static_cast<RatNumObj*>(obj));
            break;
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

void RatNumObj::add(NumObj *_r) {
    RatNumObj *r = static_cast<RatNumObj*>(_r);
#ifndef GMP_SUPPORT
    A = A * D + B * C;
    B = B * D;
    int g = gcd(na, nb);
    A /= g;
    B /= g;
#else
    val += r->val;
#endif
}

void RatNumObj::sub(NumObj *_r) {
    RatNumObj *r = static_cast<RatNumObj*>(_r);
#ifndef GMP_SUPPORT
    A = A * D - B * C;
    B = B * D;
    int g = gcd(na, nb);
    A /= g;
    B /= g;
#else
    val -= r->val;
#endif
}

void RatNumObj::mul(NumObj *_r) {
    RatNumObj *r = static_cast<RatNumObj*>(_r);
#ifndef GMP_SUPPORT
    A = A * C;
    B = B * D;
    int g = gcd(na, nb);
    A /= g;
    B /= g;
#else
    val *= r->val;
#endif
}

void RatNumObj::div(NumObj *_r) {
    RatNumObj *r = static_cast<RatNumObj*>(_r);
#ifndef GMP_SUPPORT
    A = A * D;
    B = B * C;
    int g = gcd(na, nb);
    A /= g;
    B /= g;
#else
    if (r->val == 0)
        throw NormalError(RUN_ERR_NUMERIC_OVERFLOW);
    val /= r->val;
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

bool RatNumObj::le(NumObj *_r) {
    RatNumObj *r = static_cast<RatNumObj*>(_r);
#ifndef GMP_SUPPORT
    return A * D <= C * B;
#else
    return val <= r->val;
#endif
}

bool RatNumObj::ge(NumObj *_r) {
    RatNumObj *r = static_cast<RatNumObj*>(_r);
#ifndef GMP_SUPPORT
    return A * D >= C * B;
#else
    return val >= r->val;
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

void RatNumObj::abs() {
#ifndef GMP_SUPPORT
    if (a < 0) a = -a;
#else
    val = ::abs(val);
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
IntNumObj::IntNumObj(const IntNumObj &ori) :
    ExactNumObj(NUM_LVL_INT), val(ori.val.get_mpz_t()) {}
#endif


NumObj *IntNumObj::clone() const {
    return new IntNumObj(*this);
}

IntNumObj *IntNumObj::convert(NumObj *obj) {
    switch (obj->level)
    {
        case NUM_LVL_INT :
            return new IntNumObj(*static_cast<IntNumObj*>(obj));
        default:
            throw NormalError(INT_ERR);
    }
}

void IntNumObj::add(NumObj *_r) {
    val += static_cast<IntNumObj*>(_r)->val;
}

void IntNumObj::sub(NumObj *_r) {
    val -= static_cast<IntNumObj*>(_r)->val;
}

void IntNumObj::mul(NumObj *_r) {
    val *= static_cast<IntNumObj*>(_r)->val;
}

void IntNumObj::abs() {
    val = ::abs(val);
}

void IntNumObj::rem(NumObj *_r) {
    const mpz_class &rval(static_cast<IntNumObj*>(_r)->val);
    if (rval == 0) throw NormalError(RUN_ERR_NUMERIC_OVERFLOW);
    val %= rval;
}

void IntNumObj::mod(NumObj *_r) {
    const mpz_class &rval = static_cast<IntNumObj*>(_r)->val;
    if (rval == 0) throw NormalError(RUN_ERR_NUMERIC_OVERFLOW);
    val %= rval;
    if (val != 0 && sgn(val) != sgn(rval))
        val += rval;
}

void IntNumObj::div(NumObj *_r) {
    const mpz_class &rval = static_cast<IntNumObj*>(_r)->val;
    if (rval == 0) throw NormalError(RUN_ERR_NUMERIC_OVERFLOW);
    val /= rval;
}

void IntNumObj::gcd(NumObj *_r) {
    mpz_t g;
    mpz_gcd(g, val.get_mpz_t(), static_cast<IntNumObj*>(_r)->val.get_mpz_t());
    val = mpz_class(g);
}

void IntNumObj::lcm(NumObj *_r) {
    mpz_t l;
    mpz_lcm(l, val.get_mpz_t(), static_cast<IntNumObj*>(_r)->val.get_mpz_t());
    val = mpz_class(l);
}

bool IntNumObj::lt(NumObj *_r) {
    return val < static_cast<IntNumObj*>(_r)->val;
}

bool IntNumObj::gt(NumObj *_r) {
    return val > static_cast<IntNumObj*>(_r)->val;
}

bool IntNumObj::le(NumObj *_r) {
    return val <= static_cast<IntNumObj*>(_r)->val;
}

bool IntNumObj::ge(NumObj *_r) {
    return val >= static_cast<IntNumObj*>(_r)->val;
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
