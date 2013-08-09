#include <cstdio>
#include "model.h"
#include "exc.h"
#include "consts.h"

static ReprCons *repr_stack[REPR_STACK_SIZE];
EvalObjAddrHash hash;

FrameObj::FrameObj(ClassType _ftype) : ftype(_ftype) {}

EmptyList *empty_list = new EmptyList();

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
                if (hash.count((*top_ptr)->ori))
                    *top_ptr = new ReprStr("#inf#");
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
                if (hash.count((*top_ptr)->ori))
                    *top_ptr = new ReprStr("#inf#");
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

Pair::Pair(EvalObj *_car, EvalObj *_cdr) : 
    EvalObj(CLS_PAIR_OBJ), car(_car), cdr(_cdr), 
    next(NULL) {}

    ReprCons *Pair::get_repr_cons() {
        return new PairReprCons(this, this);
    }

RetAddr::RetAddr(Pair *_addr) : FrameObj(CLS_RET_ADDR), addr(_addr) {}

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

OptObj::OptObj() : EvalObj(CLS_SIM_OBJ | CLS_OPT_OBJ) {}

ProcObj::ProcObj(Pair *_body, 
        Environment *_envt, 
        EvalObj *_params) :
    OptObj(), body(_body), params(_params), envt(_envt) {}

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

        genvt = _envt;
        cont = _cont;                   
        *top_ptr++ = new RetAddr(NULL);   // Mark the entrance of a cont
        return body;                    // Move pc to the proc entry point
    }

ReprCons *ProcObj::get_repr_cons() { 
    return new ReprStr("#<Procedure>"); 
}

SpecialOptObj::SpecialOptObj(string _name) : OptObj(), name(_name) {}

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

VecObj::VecObj() : EvalObj(CLS_SIM_OBJ | CLS_VECT_OBJ) {}

EvalObj *VecObj::get_obj(int idx) {
    return vec[idx];
}

size_t VecObj::get_size() { 
    return vec.end() - vec.begin();
}

void VecObj::resize(int new_size) {
    vec.resize(new_size);
}

void VecObj::push_back(EvalObj *new_elem) {
    vec.push_back(new_elem);
}

ReprCons *VecObj::get_repr_cons() {
    return new VectReprCons(this, this);
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

ReprCons *BuiltinProcObj::get_repr_cons() { 
    return new ReprStr("#<Builtin Procedure: " + name + ">");
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
    prev_cont(_prev_cont), envt(_envt), pc(_pc), 
    proc_body(_proc_body) {}

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

        if (idx && ptr->get_obj(idx - 1)->is_pair_obj())
            repr += ")";

        if (idx == ptr->get_size())
        {
            repr += ")";
            return NULL;
        }
        else 
        {
            if (idx) repr += " ";
            EvalObj *res = ptr->get_obj(idx++);
            if (res->is_pair_obj())
                repr += "(";
            return res;
        }
    }

bool is_list(Pair *ptr) {
    if (ptr == empty_list) return true;
    EvalObj *nptr;  
    for (;;)  
        if ((nptr = ptr->cdr)->is_pair_obj())  
            ptr = TO_PAIR(nptr);  
        else break;  
    return ptr->cdr == empty_list;
} 

