#include "builtin.h"
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
    pc = pc->cdr;
    pc->skip = false;
    pc->cdr->skip = true;
    if (pc->cdr->cdr != empty_list)
        pc->cdr->cdr->skip = true;
}

void SpecialOptIf::pre_call(ArgList *arg_list, Cons *pc,
        Environment *envt) {
    pc = dynamic_cast<Cons*>(pc->car); 
    // Condition evaluated and the decision is made
    state = 1;
    if (arg_list->cdr->car->is_true())
    {
        pc = pc->cdr;
        pc->skip = true;
        pc->cdr->skip = false;
        if (pc->cdr->cdr != empty_list)
            pc->cdr->cdr->skip = true; // Eval the former
    }
    else
    {
        pc = pc->cdr;
        pc->skip = true;
        pc->cdr->skip = true;
        if (pc->cdr->cdr != empty_list)
            pc->cdr->cdr->skip = false; //Eval the latter
    }
}

EvalObj *SpecialOptIf::post_call(ArgList *arg_list, Cons *pc,
                                Environment *envt) {
    // Value already evaluated, so just return it
    return arg_list->cdr->car;
}
Cons *SpecialOptIf::call(ArgList *arg_list, Environment * &envt, 
                        Continuation * &cont, FrameObj ** &top_ptr) {
    Cons *ret_addr = dynamic_cast<Cons*>(*top_ptr);
    if (state) 
    {
        *top_ptr = post_call(arg_list, ret_addr, envt);
        return ret_addr->next;          // Move to the next instruction
    }
    else
    {
        pre_call(arg_list, ret_addr, envt);
        top_ptr++;
        // Undo pop and invoke again
        return dynamic_cast<Cons*>(ret_addr->car)->next;
    }
}

string SpecialOptIf::ext_repr() { return string("#<Builtin Macro: if>"); }
#ifdef DEBUG
string SpecialOptIf::_debug_repr() { return ext_repr(); }
#endif

SpecialOptLambda::SpecialOptLambda() : SpecialOptObj() {}
#define FILL_MARKS(pc, flag) \
    for (pc = pc->cdr; pc != empty_list; pc = pc->cdr) \
        pc->skip = flag

void SpecialOptLambda::prepare(Cons *pc) {
    //TODO check number of arguments
    // Do not evaluate anything
    FILL_MARKS(pc, true);
}

Cons *SpecialOptLambda::call(ArgList *arg_list, Environment * &envt, 
                            Continuation * &cont, FrameObj ** &top_ptr) {
    Cons *ret_addr = dynamic_cast<Cons*>(*top_ptr);
    Cons *pc = dynamic_cast<Cons*>(ret_addr->car);
    SymbolList *para_list = dynamic_cast<SymbolList*>(pc->cdr->car);  // parameter list
    // Clear the flag to avoid side-effects (e.g. proc calling)
    FILL_MARKS(pc, false);

    // store a list of expressions inside <body>
    ASTList *body = pc->cdr->cdr;       // Truncate the expression list
    for (Cons *ptr = body; ptr != empty_list; ptr = ptr->cdr)
        ptr->next = NULL;    // Make each expression a orphan

    *top_ptr = new ProcObj(body, envt, para_list);
    return ret_addr->next;  // Move to the next instruction
}

string SpecialOptLambda::ext_repr() { return string("#<Builtin Macro: lambda>"); }
#ifdef DEBUG
string SpecialOptLambda::_debug_repr() { return ext_repr(); }
#endif

SpecialOptDefine::SpecialOptDefine() : SpecialOptObj() {}
void SpecialOptDefine::prepare(Cons *pc) {
    if (pc->cdr->car->is_simple_obj())  // Simple value assignment
    {
        pc->cdr->skip = true;           // Skip the identifier
        pc->cdr->cdr->skip = false; 
    }                                   // Procedure definition
    else FILL_MARKS(pc, true);          // Skip all parts
}
Cons *SpecialOptDefine::call(ArgList *arg_list, Environment * &envt, 
        Continuation * &cont, FrameObj ** &top_ptr) {
    Cons *ret_addr = dynamic_cast<Cons*>(*top_ptr);
    Cons *pc = dynamic_cast<Cons*>(ret_addr->car);
    EvalObj *obj;
    SymObj *id;
    // TODO: check identifier
    if (pc->cdr->car->is_simple_obj())
    {
        id = dynamic_cast<SymObj*>(pc->cdr->car);
        obj = arg_list->cdr->car;
    }
    else
    {
        Cons *plst = dynamic_cast<Cons*>(pc->cdr->car);
        id = dynamic_cast<SymObj*>(plst->car);
        ArgList *para_list = plst->cdr;
        // Clear the flag to avoid side-effects (e.g. proc calling)
        FILL_MARKS(pc, false);
        ASTList *body = pc->cdr->cdr;   // Truncate the expression list
        for (Cons *ptr = body; ptr != empty_list; ptr = ptr->cdr)
            ptr->next = NULL;           // Make each expression a orphan

        obj = new ProcObj(body, envt, para_list);
    }
    envt->add_binding(id, obj);
    *top_ptr = obj;
    return ret_addr->next;
}
string SpecialOptDefine::ext_repr() { return string("#<Builtin Macro: define>"); }
#ifdef DEBUG
string SpecialOptDefine::_debug_repr() { return ext_repr(); }
#endif

void SpecialOptSet::prepare(Cons *pc) {
    // TODO: check number of arguments
    pc->cdr->skip = true;       // Skip the identifier
    pc->cdr->cdr->skip = false; 
}

Cons *SpecialOptSet::call(ArgList *arg_list, Environment * &envt, 
                            Continuation * &cont, FrameObj ** &top_ptr) {
    Cons *ret_addr = dynamic_cast<Cons*>(*top_ptr);
    Cons *pc = dynamic_cast<Cons*>(ret_addr->car);
    SymObj *id = dynamic_cast<SymObj*>(pc->cdr->car);
    if (envt->has_obj(id))
        envt->add_binding(id, arg_list->cdr->car);
    *top_ptr = new UnspecObj();
    return ret_addr->next;
}

string SpecialOptSet::ext_repr() { return string("#<Builtin Macro: set!>"); }
#ifdef DEBUG
string SpecialOptSet::_debug_repr() { return ext_repr(); }
#endif
