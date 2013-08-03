#include <cstdio>
#include "model.h"

EmptyList *empty_list = new EmptyList();

EmptyList::EmptyList() : Cons(NULL, NULL) {}
string EmptyList::ext_repr() { return string("()"); }
#ifdef DEBUG
string EmptyList::_debug_repr() { return ext_repr(); }
#endif

bool FrameObj::is_ret_addr() { 
    return ftype == CLS_RET_ADDR;
}

EvalObj::EvalObj() : FrameObj() { ftype = CLS_EVAL_OBJ; }
void EvalObj::prepare(Cons *pc) {}
bool EvalObj::is_simple_obj() {
    return otype == CLS_SIM_OBJ;
}
void EvalObj::_debug_print() {
    printf("mem: 0x%llX\n%s\n\n", (unsigned long long)this,
            _debug_repr().c_str());
}

bool EvalObj::is_true() {
    return true;
}

Cons::Cons(EvalObj *_car, Cons *_cdr) : 
    EvalObj(), car(_car), cdr(_cdr), skip(false), next(cdr) {}

string Cons::ext_repr() { return string("#<Cons>"); }
#ifdef DEBUG
string Cons::_debug_repr() { return ext_repr(); }
void Cons::_debug_print() {
    printf("mem: 0x%llX 0x%llX 0x%llX\n%s\n", 
            (unsigned long long)this,
            (unsigned long long)car,
            (unsigned long long)cdr,
    ("car: " + car -> ext_repr() + "\n" + \
     "cdr: " + cdr -> ext_repr() + "\n").c_str());
}
#endif

RetAddr::RetAddr(Cons *_addr) : FrameObj(), addr(_addr) {
    ftype = CLS_RET_ADDR;
}
#ifdef DEBUG
string RetAddr::_debug_repr() { return string("#<Return Address>"); }
#endif

UnspecObj::UnspecObj() : EvalObj() {}
string UnspecObj::ext_repr() { return string("#<Unspecified>"); }
#ifdef DEBUG
string UnspecObj::_debug_repr() { return ext_repr(); }
#endif

SymObj::SymObj(const string &str) : EvalObj(), val(str) {}
string SymObj::ext_repr() { return "#<Symbol: " + val + ">"; }
#ifdef DEBUG
string SymObj::_debug_repr() { return ext_repr(); }
#endif

OptObj::OptObj() : EvalObj() {}

ProcObj::ProcObj(ASTList *_body, 
                    Environment *_envt, 
                    SymbolList *_para_list) :
    OptObj(), body(_body), envt(_envt), para_list(_para_list) {}

Cons *ProcObj::call(ArgList *arg_list, Environment * &envt,
                    Continuation * &cont, FrameObj ** &top_ptr) {
    // Create a new continuation
    Cons *ret_addr = dynamic_cast<Cons*>(*top_ptr);
    Continuation *ncont = new Continuation(envt, ret_addr, cont, body);
    cont = ncont;                   // Add to the cont chain
    envt = new Environment(envt);   // Create local env and recall the closure
    // TODO: Compare the arguments to the parameters
    for (Cons *ptr = arg_list->cdr, *ppar = para_list; 
            ptr != empty_list; ptr = ptr->cdr)
        envt->add_binding(dynamic_cast<SymObj*>(ppar->car), ptr->car);
    *top_ptr = new RetAddr(NULL);   // Mark the entrance of a cont
    return body;                    // Move pc to the proc entry point
}

string ProcObj::ext_repr() { return string("#<Procedure"); }
#ifdef DEBUG
string ProcObj::_debug_repr() { return ext_repr(); }
#endif

SpecialOptObj::SpecialOptObj() : OptObj() {}

NumberObj::NumberObj() : EvalObj() {}

BuiltinProcObj::BuiltinProcObj(BuiltinProc f, const string &_name) :
    OptObj(), handler(f), name(_name) {}

Cons *BuiltinProcObj::call(ArgList *arg_list, Environment * &envt,
                                Continuation * &cont, FrameObj ** &top_ptr) {

    Cons *ret_addr = dynamic_cast<Cons*>(*top_ptr);
    *top_ptr = handler(arg_list->cdr);
    return ret_addr->next;          // Move to the next instruction
}

Environment::Environment(Environment *_prev_envt) : prev_envt(_prev_envt) {}
void Environment::add_binding(SymObj *sym_obj, EvalObj *eval_obj) {
    binding[sym_obj->val] = eval_obj;
}
EvalObj *Environment::get_obj(SymObj *sym_obj) {
    string name(sym_obj->val);
    for (Environment *ptr = this; ptr; ptr = prev_envt)
    {
        bool has_key = binding.count(name);
        if (has_key) return binding[name];
    }
    //TODO: exc key not found
}
bool Environment::has_obj(SymObj *sym_obj) {
    string name(sym_obj->val);
    for (Environment *ptr = this; ptr; ptr = prev_envt)
        if (binding.count(name))
            return true;
    return false;
}

Continuation::Continuation(Environment *_envt, Cons *_pc, 
                            Continuation *_prev_cont, 
                            ASTList *_proc_body, 
                            unsigned int _body_cnt) : 
        envt(_envt), pc(_pc), prev_cont(_prev_cont), 
        proc_body(_proc_body), body_cnt(_body_cnt) {}
