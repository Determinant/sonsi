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

#define ADD_BUILTIN_PROC(name, rout) \
    ADD_ENTRY(name, new BuiltinProcObj(rout, name))

    ADD_ENTRY("if", new SpecialOptIf());
    ADD_ENTRY("lambda", new SpecialOptLambda());
    ADD_ENTRY("define", new SpecialOptDefine());
    ADD_ENTRY("set!", new SpecialOptSet());
    ADD_ENTRY("quote", new SpecialOptQuote());
    ADD_ENTRY("eval", new SpecialOptEval());

    ADD_BUILTIN_PROC("+", num_add);
    ADD_BUILTIN_PROC("-", num_sub);
    ADD_BUILTIN_PROC("*", num_mul);
    ADD_BUILTIN_PROC("/", num_div);

    ADD_BUILTIN_PROC("<", num_lt);
    ADD_BUILTIN_PROC(">", num_gt);
    ADD_BUILTIN_PROC("=", num_eq);

    ADD_BUILTIN_PROC("exact?", num_is_exact);
    ADD_BUILTIN_PROC("inexact?", num_is_inexact);

    ADD_BUILTIN_PROC("not", bool_not);
    ADD_BUILTIN_PROC("boolean?", is_boolean);

    ADD_BUILTIN_PROC("pair?", is_pair);
    ADD_BUILTIN_PROC("cons", make_pair);
    ADD_BUILTIN_PROC("car", pair_car);
    ADD_BUILTIN_PROC("cdr", pair_cdr);
    ADD_BUILTIN_PROC("set-car!", pair_set_car);
    ADD_BUILTIN_PROC("set-cdr!", pair_set_cdr);
    ADD_BUILTIN_PROC("null?", is_null);
    ADD_BUILTIN_PROC("list?", is_list);
    ADD_BUILTIN_PROC("list", make_list);
    ADD_BUILTIN_PROC("length", length);
    ADD_BUILTIN_PROC("append", append);

    ADD_BUILTIN_PROC("display", display);
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
        if (!is_list(TO_CONS(pc->car)))
            throw TokenError(pc->car->ext_repr(), RUN_ERR_WRONG_NUM_OF_ARGS); 
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
