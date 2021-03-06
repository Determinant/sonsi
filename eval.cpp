#include "consts.h"
#include "eval.h"
#include "builtin.h"
#include "exc.h"
#include "gc.h"

#include <cstdio>

static const int EVAL_STACK_SIZE = 262144;
extern Pair *empty_list;

/** The stack for evaluating expressions */
EvalObj *eval_stack[EVAL_STACK_SIZE];

/** Add all kinds of built-in facilities before the evaluation */
void Evaluator::add_builtin_routines() {

#define ADD_ENTRY(name, rout) \
    do { \
        SymObj *tmp = new SymObj(name); \
        envt->add_binding(tmp, rout); \
        delete tmp; \
    } while (0)

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

    ADD_BUILTIN_PROC("gc-status", gc_status);
    ADD_BUILTIN_PROC("set-gc-resolve-threshold!", set_gc_resolve_threshold);
}

Evaluator::Evaluator() {
    envt = new Environment(NULL);       // Top-level Environment
    gc.attach(envt);
    add_builtin_routines();
}

/**
 * Initiate the `next` pointer of an s-expression (a list)
 */
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

/**
 * Push a new stack frame according to current pc
 */
inline void push(Pair * &pc, EvalObj ** &top_ptr, 
        Environment * &envt, Continuation * &cont) {
    if (pc->car->is_simple_obj())           // Not an opt invocation
    {
        // Objectify the symbol
        *top_ptr++ = gc.attach(envt->get_obj(pc->car));  
        pc = pc->next;                      // Move to the next instruction
    }
    else                                    // Operational Invocation
    {
        if (pc->car == empty_list)
            throw NormalError(SYN_ERR_EMPTY_COMB);

        if (!cont->tail)                    // a normal invocation
        {
            gc.expose(cont);
            cont = new Continuation(envt, pc, cont);
            gc.attach(cont);
            *top_ptr++ = gc.attach(cont);
        }
        else cont->tail = false;            // tail recursion opt


        if (!make_exec(TO_PAIR(pc->car)))
            // not a valid an invocation
            throw TokenError(pc->car->ext_repr(), RUN_ERR_WRONG_NUM_OF_ARGS);
        // dive into the call
        cont->prog = pc = TO_PAIR(pc->car);  
        // "pre-call" for some `SpecialObj`
        envt->get_obj(pc->car)->prepare(pc);
    }
}

/**
 * The main routine for the evaluation
 */
EvalObj *Evaluator::run_expr(Pair *prog) {

    EvalObj **top_ptr = eval_stack;
    Pair *pc = prog;
    Continuation *bcont = new Continuation(NULL, NULL, NULL), // dummy cont
                 *cont = bcont; 

#ifdef GC_DEBUG
    fprintf(stderr, "Start the evaluation...\n");
#endif

    // attach current cont and the whole s-expression tree, so they will not be
    // garbage-collected
    gc.attach(cont);
    gc.attach(prog);        
    // envt is this->envt
    push(pc, top_ptr, envt, cont);

    // (cont != bcont) Still need to evaluate at least one level of invocation
    while (cont != bcont)   
    {
        if (top_ptr == eval_stack + EVAL_STACK_SIZE)
            throw TokenError("Evaluation", RUN_ERR_STACK_OVERFLOW);
        if (pc)
            push(pc, top_ptr, envt, cont);
        else    // All arguments are evaluated
        {
            Pair *args = empty_list;
            while (*(--top_ptr) != cont)
            {
                EvalObj* obj = static_cast<EvalObj*>(*top_ptr);
                gc.expose(obj);
                // old args is auto attached due to the constructor of `Pair`
                args = new Pair(obj, args); 
            }
            // manually protect the head pointer
            gc.attach(args);
            if ((args->car)->is_opt_obj())
            {
                OptObj *opt = static_cast<OptObj*>(args->car);
                pc = opt->call(args, envt, cont, top_ptr, cont->prog);
            }
            else
                throw TokenError((args->car)->ext_repr(), SYN_ERR_CAN_NOT_APPLY);
            //            gc.collect(); THIS IS DEPRECATED BECAUSE IT'S NOT A SAFE POINT
            //            ANYMORE DUE TO THE TAIL RECURSION OPT
        }
    }
    // remove the protection
    gc.expose(prog);
    gc.expose(cont);
    // static_cast because the previous while condition
    return static_cast<EvalObj*>(*(eval_stack));
}
