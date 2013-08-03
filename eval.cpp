#include "eval.h"
#include "builtin.h"
#include <cstdio>

extern Cons *empty_list;
const int EVAL_STACK_SIZE = 65536;
FrameObj *eval_stack[EVAL_STACK_SIZE];

void Evaluator::add_builtin_routines() {
    
#define ADD_ENTRY(name, rout) \
    envt->add_binding(new SymObj(name), rout)

    ADD_ENTRY("+", new BuiltinProcObj(builtin_plus, "+"));
    ADD_ENTRY("-", new BuiltinProcObj(builtin_minus, "-"));
    ADD_ENTRY("*", new BuiltinProcObj(builtin_times, "*"));
    ADD_ENTRY("/", new BuiltinProcObj(builtin_div, "/"));
    ADD_ENTRY(">", new BuiltinProcObj(builtin_gt, ">"));
    ADD_ENTRY("<", new BuiltinProcObj(builtin_lt, "<"));
    ADD_ENTRY("=", new BuiltinProcObj(builtin_arithmetic_eq, "="));
    ADD_ENTRY("display", new BuiltinProcObj(builtin_display, "display"));
    ADD_ENTRY("if", new SpecialOptIf());
    ADD_ENTRY("lambda", new SpecialOptLambda());
    ADD_ENTRY("define", new SpecialOptDefine());
    ADD_ENTRY("set", new SpecialOptSet());
}

Evaluator::Evaluator() {
    envt = new Environment();       // Top-level Environment
    add_builtin_routines();
}

void push(Cons * &pc, FrameObj ** &top_ptr, Environment *envt) {
    if (pc->car->is_simple_obj())           // Not an opt invocation
    {
        *top_ptr = envt->get_obj(pc->car);  // Objectify the symbol
        dynamic_cast<EvalObj*>(*top_ptr)->prepare(pc);
        top_ptr++;
        pc = pc->next;                      // Move to the next instruction
    }
    else                                    // Operational Invocation
    {
        *top_ptr++ = new RetAddr(pc);       // Push the return address
        pc = dynamic_cast<Cons*>(pc->car);  // Go deeper to enter the call
    }
}

void stack_print(FrameObj **top_ptr) {
    for (FrameObj **ptr = eval_stack; ptr != top_ptr; ptr++)
        printf("%s\n", (*ptr)->_debug_repr().c_str());
    puts("");
}

EvalObj *Evaluator::run_expr(Cons *prog) {
    FrameObj **top_ptr = eval_stack;
    Cons *pc = prog;
    Continuation *cont = NULL;
    // envt is this->envt
    push(pc, top_ptr, envt);   
    
    while((*eval_stack)->is_ret_addr())
    {
        for (; pc && pc->skip; pc = pc->next);
        if (pc)
            push(pc, top_ptr, envt);
        else
        {
            Cons *args = empty_list;
            while (!(*(--top_ptr))->is_ret_addr())
                args = new Cons(dynamic_cast<EvalObj*>(*top_ptr), args);
            RetAddr *ret_addr = dynamic_cast<RetAddr*>(*top_ptr);
            if (!ret_addr->addr)
            {
                Cons *nexp = cont->proc_body->cdr;
                cont->proc_body = nexp;
                if (nexp == empty_list)
                {
                    *top_ptr = args->car;
                    envt = cont->envt;
                    pc = cont->pc->next;
                    cont = cont->prev_cont;
                }
                else pc = nexp;
                top_ptr++;
            }
            else 
                pc = dynamic_cast<OptObj*>(args->car)->call(args, envt, cont, top_ptr);
        }
    }
    return dynamic_cast<EvalObj*>(*(eval_stack));
}
