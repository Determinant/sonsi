#ifndef BUILTIN_H
#define BUILTIN_H

#include "model.h"
#include <string>

using std::string;



/** @class InexactNumObj
 * Inexact number implementation (using doubles)
 */
class InexactNumObj: public NumObj {
    public:
        InexactNumObj(NumLvl level);
};

/** @class CompNumObj
 * Complex numbers
 */
class CompNumObj: public InexactNumObj {
    public:
        double real, imag;

        /** Construct a complex number */
        CompNumObj(double _real, double _imag);
        /** Try to construct an CompNumObj object 
         * @return NULL if failed
         */
        static CompNumObj *from_string(string repr);
        /** Convert to a complex number from other numeric types */
        CompNumObj *convert(NumObj* obj);

        NumObj *plus(NumObj *r);
        NumObj *minus(NumObj *r);
        NumObj *multi(NumObj *r);
        NumObj *div(NumObj *r);
        bool lt(NumObj *r);
        bool gt(NumObj *r);
        bool eq(NumObj *r);
        string ext_repr();
};

/** @class RealNumObj
 * Real numbers
 */
class RealNumObj: public InexactNumObj {
    public:
        double real;
        /** Construct a real number */
        RealNumObj(double _real);
        /** Try to construct an RealNumObj object 
         * @return NULL if failed
         */
        static RealNumObj *from_string(string repr);
        /** Convert to a real number from other numeric types */
        RealNumObj *convert(NumObj* obj);

        NumObj *plus(NumObj *r);
        NumObj *minus(NumObj *r);
        NumObj *multi(NumObj *r);
        NumObj *div(NumObj *r);
        bool lt(NumObj *r);
        bool gt(NumObj *r);
        bool eq(NumObj *r);
        string ext_repr();

};


/** @class ExactNumObj
 * Exact number implementation (using gmp)
 */
class ExactNumObj: public NumObj {
    public:
        ExactNumObj(NumLvl level);
};

/** @class RatNumObj
 * Rational numbers
 */
class RatNumObj: public ExactNumObj {
    public:
        int a, b;
        /** Construct a rational number */
        RatNumObj(int _a, int _b);
        /** Try to construct an RatNumObj object 
         * @return NULL if failed
         */
        static RatNumObj *from_string(string repr);
        /** Convert to a Rational number from other numeric types */
        RatNumObj *convert(NumObj* obj);

        NumObj *plus(NumObj *r);
        NumObj *minus(NumObj *r);
        NumObj *multi(NumObj *r);
        NumObj *div(NumObj *r);
        bool lt(NumObj *r);
        bool gt(NumObj *r);
        bool eq(NumObj *r);
        string ext_repr();
};

/** @class IntNumObj
 * Integers
 */
class IntNumObj: public ExactNumObj {
    public:
        int val;
        /** Construct a integer */
        IntNumObj(int _val);
        /** Try to construct an IntNumObj object 
         * @return NULL if failed
         */
        static IntNumObj *from_string(string repr);
        /** Convert to a integer from other numeric types */
        IntNumObj *convert(NumObj* obj);

        NumObj *plus(NumObj *r);
        NumObj *minus(NumObj *r);
        NumObj *multi(NumObj *r);
        NumObj *div(NumObj *r);
        bool lt(NumObj *r);
        bool gt(NumObj *r);
        bool eq(NumObj *r);
        string ext_repr();
};


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
        void pre_call(ArgList *args, Cons *pc, 
                Environment *envt);
        /** The system will call this again after the desired result is
         * evaluated, so just return it to let the evaluator know the it's the
         * answer.
         */
        EvalObj *post_call(ArgList *args, Cons *pc, 
                Environment *envt);
    public:
        SpecialOptIf();
        void prepare(Cons *pc);
        Cons *call(ArgList *args, Environment * &envt, 
                    Continuation * &cont, FrameObj ** &top_ptr);
        string ext_repr();
};

/** @class SpecialOptLambda
 * The implementation of `lambda` operator
 */
class SpecialOptLambda: public SpecialOptObj {
    public:
        SpecialOptLambda();
        void prepare(Cons *pc);
        Cons *call(ArgList *args, Environment * &envt, 
                    Continuation * &cont, FrameObj ** &top_ptr);

        string ext_repr();
};

/** @class SpecialOptDefine
 * The implementation of `define` operator
 */
class SpecialOptDefine: public SpecialOptObj {
    public:
        SpecialOptDefine(); 
        void prepare(Cons *pc);
        Cons *call(ArgList *args, Environment * &envt, 
                    Continuation * &cont, FrameObj ** &top_ptr);
        string ext_repr();
};

/** @class SpecialOptSet
 * The implementation of `set!` operator
 */
class SpecialOptSet: public SpecialOptObj {
    public:
        SpecialOptSet();
        void prepare(Cons *pc);
        Cons *call(ArgList *args, Environment * &envt, 
                    Continuation * &cont, FrameObj ** &top_ptr);
        string ext_repr();
};

/** @class SpecialOptLambda
 * The implementation of `lambda` operator
 */
class SpecialOptQuote: public SpecialOptObj {
    public:
        SpecialOptQuote();
        void prepare(Cons *pc);
        Cons *call(ArgList *args, Environment * &envt, 
                    Continuation * &cont, FrameObj ** &top_ptr);

        string ext_repr();
};

#define BUILTIN_PROC_DEF(func)\
    EvalObj *(func)(ArgList *args, const string &name)

BUILTIN_PROC_DEF(num_add);
BUILTIN_PROC_DEF(num_sub);
BUILTIN_PROC_DEF(num_multi);
BUILTIN_PROC_DEF(num_div);

BUILTIN_PROC_DEF(num_lt);
BUILTIN_PROC_DEF(num_gt);
BUILTIN_PROC_DEF(num_eq);

BUILTIN_PROC_DEF(num_exact);
BUILTIN_PROC_DEF(num_inexact);

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

BUILTIN_PROC_DEF(display);
BUILTIN_PROC_DEF(make_list);


#endif
