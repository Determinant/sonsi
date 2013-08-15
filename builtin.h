#ifndef BUILTIN_H
#define BUILTIN_H

#include "model.h"
#include "types.h"
#include <string>

using std::string;

const int EQUAL_QUEUE_SIZE = 262144;

/** @class SpecialOptIf
 * The implementation of `if` operator
 */
class SpecialOptIf: public SpecialOptObj {/*{{{*/
    private:
        unsigned char state; /**< 0 for prepared, 1 for pre_called */
    public:
        /** Construct a `if` operator */
        SpecialOptIf();
        /** Prevent <condition> and <consequence> from being evaluated */
        void prepare(Pair *pc);
        /** When it's invoked at the first time, it will determined which of
         * <condition> and <consequence> should be evaluated.  Then when it's
         * invoked again, it will tell the system the corresponding result.*/
        Pair *call(Pair *args, Environment * &envt,
                Continuation * &cont, EvalObj ** &top_ptr, Pair *pc);
};/*}}}*/

/** @class SpecialOptLambda
 * The implementation of `lambda` operator
 */
class SpecialOptLambda: public SpecialOptObj {/*{{{*/
    public:
        /** Construct a `lambda` operator */
        SpecialOptLambda();
        /** Prevent all parts of the expression being evaluated */
        void prepare(Pair *pc);
        /** Make up a ProcObj and push into the stack */
        Pair *call(Pair *args, Environment * &envt,
                Continuation * &cont, EvalObj ** &top_ptr, Pair *pc);

};/*}}}*/

/** @class SpecialOptDefine
 * The implementation of `define` operator
 */
class SpecialOptDefine: public SpecialOptObj {/*{{{*/
    public:
        /** Construct a `define` operator */
        SpecialOptDefine();
        /** Prevent some parts from being evaluated */
        void prepare(Pair *pc);
        /** See `SpecialOptLambda` */
        Pair *call(Pair *args, Environment * &envt,
                Continuation * &cont, EvalObj ** &top_ptr, Pair *pc);
};/*}}}*/

/** @class SpecialOptSet
 * The implementation of `set!` operator
 */
class SpecialOptSet: public SpecialOptObj {/*{{{*/
    public:
        /** Construct a `set!` operator */
        SpecialOptSet();
        /** See `SpecialOptDefine */
        void prepare(Pair *pc);
        /** See `SpecialOptDefine */
        Pair *call(Pair *args, Environment * &envt,
                Continuation * &cont, EvalObj ** &top_ptr, Pair *pc);
};/*}}}*/

/** @class SpecialOptLambda
 * The implementation of `lambda` operator
 */
class SpecialOptQuote: public SpecialOptObj {/*{{{*/
    public:
        /** Construct a `quote` operator */
        SpecialOptQuote();
        /** Prevent the literal part from being evaluated */
        void prepare(Pair *pc);
        /** Return the literal */
        Pair *call(Pair *args, Environment * &envt,
                Continuation * &cont, EvalObj ** &top_ptr, Pair *pc);

};/*}}}*/

/** @class SpecialOptEval
 * The implementation of `eval` operator
 */
class SpecialOptEval: public SpecialOptObj {/*{{{*/
    private:
        unsigned char state; /**< 0 for prepared, 1 for pre_called */
    public:
        /** Construct an `eval` operator */
        SpecialOptEval();
        /** Set state to 0 */
        void prepare(Pair *pc);
        /** Behaves like the one in `SpecialOptIf` */
        Pair *call(Pair *args, Environment * &envt,
                Continuation * &cont, EvalObj ** &top_ptr, Pair *pc);

};/*}}}*/

/** @class SpecialOptAnd
 * The implementation of `and` operator
 */
class SpecialOptAnd: public SpecialOptObj {/*{{{*/
    public:
        /** Construct an `and` operator */
        SpecialOptAnd();
        /** Prevent all parts from being evaluated */
        void prepare(Pair *pc);
        /** Acts like `SpecialOptIf` */
        Pair *call(Pair *args, Environment * &envt,
                Continuation * &cont, EvalObj ** &top_ptr, Pair *pc);

};/*}}}*/

/** @class SpecialOptOr
 * The implementation of `and` operator
 */
class SpecialOptOr: public SpecialOptObj {/*{{{*/
    public:
        /** Construct an `or` operator */
        SpecialOptOr();
        /** See `SpecialOptAnd` */
        void prepare(Pair *pc);
        /** See `SpecialOptAnd` */
        Pair *call(Pair *args, Environment * &envt,
                Continuation * &cont, EvalObj ** &top_ptr, Pair *pc);

};/*}}}*/

/** @class SpecialOptApply
 * The implementation of `apply` operator
 */
class SpecialOptApply: public SpecialOptObj {/*{{{*/
    public:
        /** Construct an `apply` operator */
        SpecialOptApply();
        /** Do nothing */
        void prepare(Pair *pc);
        /** Provoke the <proc> with args */
        Pair *call(Pair *args, Environment * &envt,
                Continuation * &cont, EvalObj ** &top_ptr, Pair *pc);

};/*}}}*/

/** @class SpecialOptDelay
 * The implementation of `delay` operator
 */
class SpecialOptDelay: public SpecialOptObj {/*{{{*/
    public:
        /** Construct a `delay` operator */
        SpecialOptDelay();
        /** Do nothing */
        void prepare(Pair *pc);
        /** Make up a PromObj and push into the stack */
        Pair *call(Pair *args, Environment * &envt,
                Continuation * &cont, EvalObj ** &top_ptr, Pair *pc);

};/*}}}*/

/** @class SpecialOptForce
 * The implementation of `force` operator
 */
class SpecialOptForce: public SpecialOptObj {/*{{{*/
    private:
        unsigned char state;
        PromObj* prom;
    public:
        /** Construct a `force` operator */
        SpecialOptForce();
        /** Set the state to 0 */
        void prepare(Pair *pc);
        /** Force the evaluation of a promise. If the promise has not been
         * evaluated yet, then evaluate and feed the result to its memory,
         * while if it has already been evaluated, just push the result into
         * the stack */
        Pair *call(Pair *args, Environment * &envt,
                Continuation * &cont, EvalObj ** &top_ptr, Pair *pc);

};/*}}}*/

#define BUILTIN_PROC_DEF(func)\
    EvalObj *(func)(Pair *args, const string &name)

BUILTIN_PROC_DEF(num_add);
BUILTIN_PROC_DEF(num_sub);
BUILTIN_PROC_DEF(num_mul);
BUILTIN_PROC_DEF(num_div);

BUILTIN_PROC_DEF(num_lt);
BUILTIN_PROC_DEF(num_le);
BUILTIN_PROC_DEF(num_gt);
BUILTIN_PROC_DEF(num_ge);
BUILTIN_PROC_DEF(num_eq);

BUILTIN_PROC_DEF(num_is_exact);
BUILTIN_PROC_DEF(num_is_inexact);
BUILTIN_PROC_DEF(is_number);
BUILTIN_PROC_DEF(is_complex);
BUILTIN_PROC_DEF(is_real);
BUILTIN_PROC_DEF(is_rational);
BUILTIN_PROC_DEF(is_integer);
BUILTIN_PROC_DEF(num_abs);
BUILTIN_PROC_DEF(num_mod);
BUILTIN_PROC_DEF(num_rem);
BUILTIN_PROC_DEF(num_quo);
BUILTIN_PROC_DEF(num_gcd);
BUILTIN_PROC_DEF(num_lcm);

BUILTIN_PROC_DEF(bool_not);
BUILTIN_PROC_DEF(is_boolean);

BUILTIN_PROC_DEF(is_pair);
BUILTIN_PROC_DEF(make_pair);
BUILTIN_PROC_DEF(pair_car);
BUILTIN_PROC_DEF(pair_cdr);
BUILTIN_PROC_DEF(pair_set_car);
BUILTIN_PROC_DEF(pair_set_cdr);
BUILTIN_PROC_DEF(is_null);
BUILTIN_PROC_DEF(is_list);
BUILTIN_PROC_DEF(make_list);
BUILTIN_PROC_DEF(length);
BUILTIN_PROC_DEF(append);
BUILTIN_PROC_DEF(reverse);
BUILTIN_PROC_DEF(list_tail);

BUILTIN_PROC_DEF(is_eqv);
BUILTIN_PROC_DEF(is_equal);

BUILTIN_PROC_DEF(display);
BUILTIN_PROC_DEF(is_string);
BUILTIN_PROC_DEF(is_symbol);
BUILTIN_PROC_DEF(string_lt);
BUILTIN_PROC_DEF(string_le);
BUILTIN_PROC_DEF(string_gt);
BUILTIN_PROC_DEF(string_ge);
BUILTIN_PROC_DEF(string_eq);

BUILTIN_PROC_DEF(make_vector);
BUILTIN_PROC_DEF(vector_set);
BUILTIN_PROC_DEF(vector_ref);
BUILTIN_PROC_DEF(vector_length);

BUILTIN_PROC_DEF(gc_status);
BUILTIN_PROC_DEF(set_gc_resolve_threshold);

#endif
