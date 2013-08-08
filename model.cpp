#include <cstdio>
#include "model.h"
#include "exc.h"
#include "consts.h"

FrameObj::FrameObj(ClassType _ftype) : ftype(_ftype) {}

EmptyList *empty_list = new EmptyList();

EmptyList::EmptyList() : Pair(NULL, NULL) {}

string EmptyList::ext_repr() { return string("()"); }

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

bool EvalObj::is_cons_obj() {
    return this != empty_list && (otype & CLS_CONS_OBJ);
}


bool EvalObj::is_num_obj() {
    return otype & CLS_NUM_OBJ;
}

bool EvalObj::is_bool_obj() {
    return otype & CLS_BOOL_OBJ;
}

ClassType EvalObj::get_otype() {
    return otype;
}

bool EvalObj::is_true() {
    return true;
}

string EvalObj::ext_repr() {
}

Pair::Pair(EvalObj *_car, EvalObj *_cdr) : 
    EvalObj(CLS_CONS_OBJ), car(_car), cdr(_cdr), skip(false), 
    next(NULL) {}

RetAddr::RetAddr(Pair *_addr) : FrameObj(CLS_RET_ADDR), addr(_addr) {}

ParseBracket::ParseBracket(unsigned char _btype) : 
    FrameObj(CLS_SIM_OBJ | CLS_PAR_BRA), btype(_btype) {}

UnspecObj::UnspecObj() : EvalObj(CLS_SIM_OBJ) {}

string UnspecObj::ext_repr() { return string("#<Unspecified>"); }

SymObj::SymObj(const string &str) : 
    EvalObj(CLS_SIM_OBJ | CLS_SYM_OBJ), val(str) {}

string SymObj::ext_repr() { return val; }

OptObj::OptObj() : EvalObj(CLS_SIM_OBJ | CLS_OPT_OBJ) {}

ProcObj::ProcObj(Pair *_body, 
                    Environment *_envt, 
                    EvalObj *_params) :
    OptObj(), body(_body), envt(_envt), params(_params) {}

Pair *ProcObj::call(ArgList *args, Environment * &genvt,
                    Continuation * &cont, FrameObj ** &top_ptr) {
    // Create a new continuation
    // static_cast see `call` invocation in eval.cpp
    Pair *ret_addr = static_cast<RetAddr*>(*top_ptr)->addr;
    Continuation *_cont = new Continuation(genvt, ret_addr, cont, body);
    // Create local env and recall the closure
    Environment *_envt = new Environment(envt);   
    // static_cast<SymObj*> because the params is already checked
    EvalObj *ppar, *nptr;
    for (ppar = params;
            ppar->is_cons_obj(); 
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

    genvt = _envt;
    cont = _cont;                   
    *top_ptr++ = new RetAddr(NULL);   // Mark the entrance of a cont
    return body;                    // Move pc to the proc entry point
}

string ProcObj::ext_repr() { return string("#<Procedure>"); }

SpecialOptObj::SpecialOptObj(string _name) : OptObj(), name(_name) {}

BoolObj::BoolObj(bool _val) : EvalObj(CLS_SIM_OBJ | CLS_BOOL_OBJ), val(_val) {}

bool BoolObj::is_true() { return val; }

string BoolObj::ext_repr() { return string(val ? "#t" : "#f"); }

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
    EvalObj(CLS_SIM_OBJ | CLS_NUM_OBJ), level(_level), exactness(_exactness) {}

bool NumObj::is_exact() { return exactness; }

StrObj::StrObj(string _str) : EvalObj(CLS_SIM_OBJ | CLS_STR_OBJ), str(_str) {}

string StrObj::ext_repr() { return str; }

CharObj::CharObj(char _ch) : EvalObj(CLS_SIM_OBJ | CLS_CHAR_OBJ), ch(_ch) {}

CharObj *CharObj::from_string(string repr) {
    int len = repr.length();
    if (len < 2) return NULL;
    if (repr[0] != '#' || repr[1] != '\\') return NULL;
    if (len == 3) return new CharObj(repr[2]);
    string char_name = repr.substr(2, len - 2);
    if (char_name == "newline") return new CharObj('\n');
    if (char_name == "space") return new CharObj(' ');
    throw TokenError(char_name, RUN_ERR_UNKNOWN_CHAR_NAME);
}

string CharObj::ext_repr() {
    string val = "";
    if (ch == ' ') val = "space";
    else if (ch == '\n') val = "newline";
    else val += ch;
    return "#\\" + val;
}

VecObj::VecObj() : EvalObj(CLS_SIM_OBJ | CLS_VECT_OBJ) {}

void VecObj::resize(int new_size) {
    vec.resize(new_size);
}

void VecObj::push_back(EvalObj *new_elem) {
    vec.push_back(new_elem);
}

StrObj *StrObj::from_string(string repr) {
    size_t len = repr.length();
    if (repr[0] == '\"' && repr[len - 1] == '\"')
        return new StrObj(repr.substr(1, len - 2));
    return NULL;
}

BuiltinProcObj::BuiltinProcObj(BuiltinProc f, string _name) :
    OptObj(), handler(f), name(_name) {}

Pair *BuiltinProcObj::call(ArgList *args, Environment * &envt,
                                Continuation * &cont, FrameObj ** &top_ptr) {

    Pair *ret_addr = static_cast<RetAddr*>(*top_ptr)->addr;
    *top_ptr++ = handler(TO_PAIR(args->cdr), name);
    return ret_addr->next;          // Move to the next instruction
}

string BuiltinProcObj::ext_repr() { 
    return "#<Builtin Procedure: " + name + ">";
}

Environment::Environment(Environment *_prev_envt) : prev_envt(_prev_envt) {}

bool Environment::add_binding(SymObj *sym_obj, EvalObj *eval_obj, bool def) {
    bool has_key = binding.count(sym_obj->val);
    if (!def && !has_key) return false;
    binding[sym_obj->val] = eval_obj;
    return true;
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
                            Continuation *_prev_cont, 
                            Pair *_proc_body) : 
        envt(_envt), pc(_pc), prev_cont(_prev_cont), 
        proc_body(_proc_body) {}
