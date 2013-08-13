#include "eval.h"
#include "builtin.h"
#include "exc.h"
#include "consts.h"
#include "gc.h"
#include <cstdio>

extern Pair *empty_list;
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
    ADD_ENTRY("and", new SpecialOptAnd());
    ADD_ENTRY("or", new SpecialOptOr());
    ADD_ENTRY("apply", new SpecialOptApply());
    ADD_ENTRY("delay", new SpecialOptDelay());
    ADD_ENTRY("force", new SpecialOptForce());

    ADD_BUILTIN_PROC("+", num_add);
    ADD_BUILTIN_PROC("-", num_sub);
    ADD_BUILTIN_PROC("*", num_mul);
    ADD_BUILTIN_PROC("/", num_div);

    ADD_BUILTIN_PROC("<", num_lt);
    ADD_BUILTIN_PROC("<=", num_le);
    ADD_BUILTIN_PROC(">", num_gt);
    ADD_BUILTIN_PROC(">=", num_ge);
    ADD_BUILTIN_PROC("=", num_eq);

    ADD_BUILTIN_PROC("exact?", num_is_exact);
    ADD_BUILTIN_PROC("inexact?", num_is_inexact);
    ADD_BUILTIN_PROC("number?", is_number);
    ADD_BUILTIN_PROC("complex?", is_complex);
    ADD_BUILTIN_PROC("real?", is_real);
    ADD_BUILTIN_PROC("rational?", is_rational);
    ADD_BUILTIN_PROC("integer?", is_integer);
    ADD_BUILTIN_PROC("abs", num_abs);
    ADD_BUILTIN_PROC("modulo", num_mod);
    ADD_BUILTIN_PROC("remainder", num_rem);
    ADD_BUILTIN_PROC("quotient", num_quo);
    ADD_BUILTIN_PROC("gcd", num_gcd);
    ADD_BUILTIN_PROC("lcm", num_lcm);


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
    ADD_BUILTIN_PROC("reverse", reverse);
    ADD_BUILTIN_PROC("list-tail", list_tail);

    ADD_BUILTIN_PROC("eqv?", is_eqv);
    ADD_BUILTIN_PROC("eq?", is_eqv);
    ADD_BUILTIN_PROC("equal?", is_equal);

    ADD_BUILTIN_PROC("display", display);
    ADD_BUILTIN_PROC("string?", is_string);
    ADD_BUILTIN_PROC("symbol?", is_symbol);
    ADD_BUILTIN_PROC("string<?", string_lt);
    ADD_BUILTIN_PROC("string<=?", string_le);
    ADD_BUILTIN_PROC("string>?", string_gt);
    ADD_BUILTIN_PROC("string>=?", string_ge);
    ADD_BUILTIN_PROC("string=?", string_eq);

    ADD_BUILTIN_PROC("make-vector", make_vector);
    ADD_BUILTIN_PROC("vector-set!", vector_set);
    ADD_BUILTIN_PROC("vector-ref", vector_ref);
    ADD_BUILTIN_PROC("vector-length", vector_length);
}

Evaluator::Evaluator() {
    envt = new Environment(NULL);       // Top-level Environment
    gc.attach(envt);
    add_builtin_routines();
}

inline bool make_exec(Pair *ptr) {
    if (ptr == empty_list) return true;
    EvalObj *nptr;
    for (;;)
        if ((nptr = ptr->cdr)->is_pair_obj())
        {
            ptr->next = TO_PAIR(nptr);
            ptr = ptr->next;
        }
        else break;
    ptr->next = NULL;
    return ptr->cdr == empty_list;
}

inline void push(Pair * &pc, FrameObj ** &top_ptr, Environment *envt) {
//    if (pc->car == NULL)
 //       puts("oops");
    if (pc->car->is_simple_obj())           // Not an opt invocation
    {
        *top_ptr++ = gc.attach(envt->get_obj(pc->car));  // Objectify the symbol
        pc = pc->next;                      // Move to the next instruction
//        if (pc == empty_list)
//        puts("oops");
    }
    else                                    // Operational Invocation
    {
        if (pc->car == empty_list)
            throw NormalError(SYN_ERR_EMPTY_COMB);

        *top_ptr++ = new RetAddr(pc, NULL);       // Push the return address
        if (!make_exec(TO_PAIR(pc->car)))
            throw TokenError(pc->car->ext_repr(), RUN_ERR_WRONG_NUM_OF_ARGS);
        // static_cast because of is_simple_obj() is false
        pc = static_cast<Pair*>(pc->car);  // Go deeper to enter the call
        envt->get_obj(pc->car)->prepare(pc);
    }
}

void print_stack(FrameObj **top) {
    for (FrameObj **i = eval_stack; i < top; i++)
    {
        if ((*i)->is_ret_addr())
            puts("<return addr>");
        else 
            printf("%s\n", static_cast<EvalObj*>(*i)->ext_repr().c_str());
    }
    puts("");
}

EvalObj *Evaluator::run_expr(Pair *prog) {
    FrameObj **top_ptr = eval_stack;
    Pair *pc = prog;
    Continuation *cont = NULL;
#ifdef GC_DEBUG
    fprintf(stderr, "Start the evaluation...\n");
#endif
    // envt is this->envt
    push(pc, top_ptr, envt);
    gc.attach(prog);

    while((*eval_stack)->is_ret_addr())
    {
        if (top_ptr == eval_stack + EVAL_STACK_SIZE)
            throw TokenError("Evaluation", RUN_ERR_STACK_OVERFLOW);
        if (pc)
            push(pc, top_ptr, envt);
        else
        {
            Pair *args = empty_list;
            while (!(*(--top_ptr))->is_ret_addr())
            {
                EvalObj* obj = static_cast<EvalObj*>(*top_ptr);
                gc.expose(obj);
                args = new Pair(obj, args);
            }
            //< static_cast because the while condition
            RetAddr *ret_addr = static_cast<RetAddr*>(*top_ptr);
            gc.attach(args);
            if (!ret_addr->addr)
            {
                Pair *nexp = TO_PAIR(cont->proc_body->cdr);
                cont->proc_body = nexp;
                if (nexp == empty_list)
                {
                    *top_ptr = gc.attach(args->car);

                    gc.expose(envt);
                    envt = cont->envt;
                    gc.attach(envt);

                    pc = cont->pc->next;

                    gc.expose(cont);
                    cont = cont->prev_cont;
                    gc.attach(cont);
                }
                else pc = nexp;
                gc.expose(args);
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
                gc.force();
            }
        }
    }
    gc.expose(prog);
    // static_cast because the previous while condition
    return static_cast<EvalObj*>(*(eval_stack));
}
