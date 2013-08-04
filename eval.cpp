#include "eval.h"
#include "builtin.h"
#include "exc.h"
#include "consts.h"
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
    ADD_ENTRY("cons", new BuiltinProcObj(builtin_cons, "cons"));
    ADD_ENTRY("car", new BuiltinProcObj(builtin_car, "car"));
    ADD_ENTRY("cdr", new BuiltinProcObj(builtin_cdr, "cdr"));
    ADD_ENTRY("if", new SpecialOptIf());
    ADD_ENTRY("lambda", new SpecialOptLambda());
    ADD_ENTRY("define", new SpecialOptDefine());
    ADD_ENTRY("set!", new SpecialOptSet());
}

Evaluator::Evaluator() {
    envt = new Environment(NULL);       // Top-level Environment
    add_builtin_routines();
}

void push(Cons * &pc, FrameObj ** &top_ptr, Environment *envt) {
    if (pc->car->is_simple_obj())           // Not an opt invocation
    {
        *top_ptr = envt->get_obj(pc->car);  // Objectify the symbol
        // static_cast because of is_simple_obj() is true
        static_cast<EvalObj*>(*top_ptr)->prepare(pc);
        top_ptr++;
        pc = pc->next;                      // Move to the next instruction
    }
    else                                    // Operational Invocation
    {
        if (pc->car == empty_list)
            throw NormalError(SYN_ERR_EMPTY_COMB);

        *top_ptr++ = new RetAddr(pc);       // Push the return address
        // static_cast because of is_simple_obj() is false
        pc = static_cast<Cons*>(pc->car);  // Go deeper to enter the call
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
                args = new Cons(static_cast<EvalObj*>(*top_ptr), args);
                //< static_cast because the while condition
            RetAddr *ret_addr = static_cast<RetAddr*>(*top_ptr);
            if (!ret_addr->addr)
            {
                Cons *nexp = TO_CONS(cont->proc_body->cdr);
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
            {
                EvalObj *opt = args->car;
                if (opt->is_opt_obj())
                    pc = static_cast<OptObj*>(opt)->
                        call(args, envt, cont, top_ptr);
                else
                    throw TokenError(opt->ext_repr(), SYN_ERR_CAN_NOT_APPLY);
            }
        }
    }
    // static_cast because the previous while condition
    return static_cast<EvalObj*>(*(eval_stack));
}
