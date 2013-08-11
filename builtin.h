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
class SpecialOptIf: public SpecialOptObj {
    private:
        unsigned char state; /**< 0 for prepared, 1 for pre_called */
        /**
         * The evaluator will call this after the <condition> exp is evaluated.
         * And this function tells the evaluator which of <consequence> and
         * <alternative> should be evaluted. */
        void pre_call(Pair *args, Pair *pc,
                Environment *envt);
        /** The system will call this again after the desired result is
         * evaluated, so just return it to let the evaluator know the it's the
         * answer.
         */
        EvalObj *post_call(Pair *args, Pair *pc,
                Environment *envt);
    public:
        SpecialOptIf();
        void prepare(Pair *pc);
        Pair *call(Pair *args, Environment * &envt,
                    Continuation * &cont, FrameObj ** &top_ptr);
        ReprCons *get_repr_cons();
};

/** @class SpecialOptLambda
 * The implementation of `lambda` operator
 */
class SpecialOptLambda: public SpecialOptObj {
    public:
        SpecialOptLambda();
        void prepare(Pair *pc);
        Pair *call(Pair *args, Environment * &envt,
                    Continuation * &cont, FrameObj ** &top_ptr);

        ReprCons *get_repr_cons();
};

/** @class SpecialOptDefine
 * The implementation of `define` operator
 */
class SpecialOptDefine: public SpecialOptObj {
    public:
        SpecialOptDefine();
        void prepare(Pair *pc);
        Pair *call(Pair *args, Environment * &envt,
                    Continuation * &cont, FrameObj ** &top_ptr);
        ReprCons *get_repr_cons();
};

/** @class SpecialOptSet
 * The implementation of `set!` operator
 */
class SpecialOptSet: public SpecialOptObj {
    public:
        SpecialOptSet();
        void prepare(Pair *pc);
        Pair *call(Pair *args, Environment * &envt,
                    Continuation * &cont, FrameObj ** &top_ptr);
        ReprCons *get_repr_cons();
};

/** @class SpecialOptLambda
 * The implementation of `lambda` operator
 */
class SpecialOptQuote: public SpecialOptObj {
    public:
        SpecialOptQuote();
        void prepare(Pair *pc);
        Pair *call(Pair *args, Environment * &envt,
                    Continuation * &cont, FrameObj ** &top_ptr);

        ReprCons *get_repr_cons();
};

/** @class SpecialOptEval
 * The implementation of `eval` operator
 */
class SpecialOptEval: public SpecialOptObj {
    private:
        unsigned char state; /**< 0 for prepared, 1 for pre_called */
    public:
        SpecialOptEval();
        void prepare(Pair *pc);
        Pair *call(Pair *args, Environment * &envt,
                    Continuation * &cont, FrameObj ** &top_ptr);

        ReprCons *get_repr_cons();
};

/** @class SpecialOptAnd
 * The implementation of `and` operator
 */
class SpecialOptAnd: public SpecialOptObj {
    public:
        SpecialOptAnd();
        void prepare(Pair *pc);
        Pair *call(Pair *args, Environment * &envt,
                    Continuation * &cont, FrameObj ** &top_ptr);

        ReprCons *get_repr_cons();
};

/** @class SpecialOptOr
 * The implementation of `and` operator
 */
class SpecialOptOr: public SpecialOptObj {
    public:
        SpecialOptOr();
        void prepare(Pair *pc);
        Pair *call(Pair *args, Environment * &envt,
                    Continuation * &cont, FrameObj ** &top_ptr);

        ReprCons *get_repr_cons();
};

/** @class SpecialOptApply
 * The implementation of `apply` operator
 */
class SpecialOptApply: public SpecialOptObj {
    public:
        SpecialOptApply();
        void prepare(Pair *pc);
        Pair *call(Pair *args, Environment * &envt,
                    Continuation * &cont, FrameObj ** &top_ptr);

        ReprCons *get_repr_cons();
};

/** @class SpecialOptDelay
 * The implementation of `delay` operator
 */
class SpecialOptDelay: public SpecialOptObj {
    public:
        SpecialOptDelay();
        void prepare(Pair *pc);
        Pair *call(Pair *args, Environment * &envt,
                    Continuation * &cont, FrameObj ** &top_ptr);

        ReprCons *get_repr_cons();
};

/** @class SpecialOptForce
 * The implementation of `force` operator
 */
class SpecialOptForce: public SpecialOptObj {
    private:
        bool state;
        PromObj* prom;
    public:
        SpecialOptForce();
        void prepare(Pair *pc);
        Pair *call(Pair *args, Environment * &envt,
                    Continuation * &cont, FrameObj ** &top_ptr);

        ReprCons *get_repr_cons();
};

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

#endif
