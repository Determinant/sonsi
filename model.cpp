#include <cstdio>
#include "model.h"
#include "exc.h"
#include "consts.h"

FrameObj::FrameObj(ClassType _ftype) : ftype(_ftype) {}

EmptyList *empty_list = new EmptyList();

EmptyList::EmptyList() : Cons(NULL, NULL) {}

string EmptyList::ext_repr() { return string("()"); }

#ifdef DEBUG
string EmptyList::_debug_repr() { return ext_repr(); }
#endif

bool FrameObj::is_ret_addr() { 
    return ftype & CLS_RET_ADDR;
}

EvalObj::EvalObj(ClassType _otype) : FrameObj(CLS_EVAL_OBJ), otype(_otype) {}

void EvalObj::prepare(Cons *pc) {}

bool EvalObj::is_simple_obj() {
    return otype & CLS_SIM_OBJ;
}

bool EvalObj::is_sym_obj() {
    return otype & CLS_SYM_OBJ;
}

bool EvalObj::is_opt_obj() {
    return otype & CLS_OPT_OBJ;
}

#ifdef DEBUG
void EvalObj::_debug_print() {
    printf("mem: 0x%llX\n%s\n\n", (unsigned long long)this,
            _debug_repr().c_str());
}
#endif

bool EvalObj::is_true() {
    return true;
}

Cons::Cons(EvalObj *_car, Cons *_cdr) : 
    EvalObj(CLS_CONS_OBJ), car(_car), cdr(_cdr), skip(false), 
    next(cdr == empty_list ? NULL : cdr) {}

string Cons::ext_repr() { return string("#<Cons>"); }

#ifdef DEBUG
string Cons::_debug_repr() { return ext_repr(); }

void Cons::_debug_print() {
    printf("mem: 0x%llX (0x%llX . 0x%llX) | 0x%llX\n%s\n", 
            (unsigned long long)this,
            (unsigned long long)car,
            (unsigned long long)cdr,
            (unsigned long long)next,
    ("car: " + car -> ext_repr() + "\n" + \
     "cdr: " + cdr -> ext_repr() + "\n").c_str());
}
#endif

RetAddr::RetAddr(Cons *_addr) : FrameObj(CLS_RET_ADDR), addr(_addr) {}

#ifdef DEBUG
string RetAddr::_debug_repr() { return string("#<Return Address>"); }
#endif

UnspecObj::UnspecObj() : EvalObj() {}

string UnspecObj::ext_repr() { return string("#<Unspecified>"); }

#ifdef DEBUG
string UnspecObj::_debug_repr() { return ext_repr(); }
#endif

SymObj::SymObj(const string &str) : 
    EvalObj(CLS_SIM_OBJ | CLS_SYM_OBJ), val(str) {}

string SymObj::ext_repr() { return "#<Symbol: " + val + ">"; }

#ifdef DEBUG
string SymObj::_debug_repr() { return ext_repr(); }
#endif

OptObj::OptObj() : EvalObj(CLS_SIM_OBJ | CLS_OPT_OBJ) {}

ProcObj::ProcObj(ASTList *_body, 
                    Environment *_envt, 
                    SymbolList *_para_list) :
    OptObj(), body(_body), envt(_envt), para_list(_para_list) {}

Cons *ProcObj::call(ArgList *args, Environment * &genvt,
                    Continuation * &cont, FrameObj ** &top_ptr) {
    // Create a new continuation
    // static_cast see `call` invocation in eval.cpp
    Cons *ret_addr = static_cast<RetAddr*>(*top_ptr)->addr;
    Continuation *_cont = new Continuation(genvt, ret_addr, cont, body);
    // Create local env and recall the closure
    Environment *_envt = new Environment(envt);   
    // static_cast<SymObj*> because the para_list is already checked
    Cons *ptr, *ppar;
    for (ptr = args->cdr, ppar = para_list; 
            ptr != empty_list && ppar != empty_list; 
            ptr = ptr->cdr, ppar = ppar->cdr)
        _envt->add_binding(static_cast<SymObj*>(ppar->car), ptr->car);

    if (ptr != empty_list || ppar != empty_list)
        throw TokenError("", RUN_ERR_WRONG_NUM_OF_ARGS);

    genvt = _envt;
    cont = _cont;                   
    *top_ptr++ = new RetAddr(NULL);   // Mark the entrance of a cont
    return body;                    // Move pc to the proc entry point
}

string ProcObj::ext_repr() { return string("#<Procedure>"); }

#ifdef DEBUG
string ProcObj::_debug_repr() { return ext_repr(); }
#endif

SpecialOptObj::SpecialOptObj() : OptObj() {}

NumberObj::NumberObj() : EvalObj() {}

BuiltinProcObj::BuiltinProcObj(BuiltinProc f, string _name) :
    OptObj(), handler(f), name(_name) {}

Cons *BuiltinProcObj::call(ArgList *args, Environment * &envt,
                                Continuation * &cont, FrameObj ** &top_ptr) {

    Cons *ret_addr = static_cast<RetAddr*>(*top_ptr)->addr;
    *top_ptr++ = handler(args->cdr);
    return ret_addr->next;          // Move to the next instruction
}

string BuiltinProcObj::ext_repr() { 
    return "#<Builtin Procedure: " + name + ">";
}

#ifdef DEBUG
string BuiltinProcObj::_debug_repr() { return ext_repr(); }
#endif

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

Continuation::Continuation(Environment *_envt, Cons *_pc, 
                            Continuation *_prev_cont, 
                            ASTList *_proc_body) : 
        envt(_envt), pc(_pc), prev_cont(_prev_cont), 
        proc_body(_proc_body) {}
