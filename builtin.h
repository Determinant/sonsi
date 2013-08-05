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
        /** Try to construct an RealNumObj object 
         * @return NULL if failed
         */
        static CompNumObj *from_string(string repr);
        /** Convert to a complex number from other numeric types */
        static CompNumObj *convert(NumObj* obj);

        NumObj *plus(NumObj *r);
        NumObj *minus(NumObj *r);
        NumObj *multi(NumObj *r);
        NumObj *div(NumObj *r);
        BoolObj *eq(NumObj *r);
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
        static RealNumObj *convert(NumObj* obj);

        NumObj *plus(NumObj *r);
        NumObj *minus(NumObj *r);
        NumObj *multi(NumObj *r);
        NumObj *div(NumObj *r);
        BoolObj *lt(NumObj *r);
        BoolObj *gt(NumObj *r);
        BoolObj *eq(NumObj *r);
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
        /** Try to construct an RealNumObj object 
         * @return NULL if failed
         */
        static RatNumObj *from_string(string repr);
        /** Convert to a Rational number from other numeric types */
        static RatNumObj *convert(NumObj* obj);

        NumObj *plus(NumObj *r);
        NumObj *minus(NumObj *r);
        NumObj *multi(NumObj *r);
        NumObj *div(NumObj *r);
        BoolObj *lt(NumObj *r);
        BoolObj *gt(NumObj *r);
        BoolObj *eq(NumObj *r);
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
        static IntNumObj *convert(NumObj* obj);

        NumObj *plus(NumObj *r);
        NumObj *minus(NumObj *r);
        NumObj *multi(NumObj *r);
        NumObj *div(NumObj *r);
        BoolObj *lt(NumObj *r);
        BoolObj *gt(NumObj *r);
        BoolObj *eq(NumObj *r);
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

EvalObj *builtin_plus(ArgList *);

EvalObj *builtin_display(ArgList *);
EvalObj *builtin_cons(ArgList *);
EvalObj *builtin_car(ArgList *);
EvalObj *builtin_cdr(ArgList *);
EvalObj *builtin_list(ArgList *);

#endif
