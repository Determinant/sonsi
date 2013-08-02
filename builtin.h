#ifndef BUILTIN_H
#define BUILTIN_H

#include "model.h"
#include <string>

using std::string;

/** @class BoolObj
 * Booleans
 */
class BoolObj: public EvalObj {
    private:
        bool val;                       /**< true for #t, false for #f */ 
    public:
        BoolObj(bool); 
        bool is_true();                 /**< Override EvalObj `is_true()` */
#ifdef DEBUG
        string _debug_repr();
#endif
        string ext_repr();
};

/** @class IntObj
 * A simple implementation of integers
 * Will be removed in the future
 */
class IntObj: public NumberObj {
    private:
        int val;
    public:
        IntObj(int);
#ifdef DEBUG
        string _debug_repr();
#endif
        string ext_repr();
};

/** @class FloatObj
 * Floating point numbers
 */
class FloatObj: public NumberObj {
    private:
        double val;
    public:
        FloatObj(double);
#ifdef DEBUG
        string _debug_repr();
#endif
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
        void pre_call(ArgList *arg_list, Cons *pc, 
                Environment *envt);
        /** The system will call this again after the desired result is
         * evaluated, so just return it to let the evaluator know the it's the
         * answer.
         */
        EvalObj *post_call(ArgList *arg_list, Cons *pc, 
                Environment *envt);
    public:
        SpecialOptIf();
        void prepare(Cons *pc);
        Cons *call(ArgList *arg_list, Environment * &envt, 
                    Continuation * &cont, FrameObj ** &top_ptr);
#ifdef DEBUG
        string _debug_repr();
#endif
        string ext_repr();
};

/** @class SpecialOptLambda
 * The implementation of `lambda` operator
 */
class SpecialOptLambda: public SpecialOptObj {
    public:
        SpecialOptLambda();
        void prepare(Cons *pc);
        Cons *call(ArgList *arg_list, Environment * &envt, 
                    Continuation * &cont, FrameObj ** &top_ptr);

#ifdef DEBUG
        string _debug_repr();
#endif
        string ext_repr();
};

/** @class SpecialOptDefine
 * The implementation of `define` operator
 */
class SpecialOptDefine: public SpecialOptObj {
    public:
        SpecialOptDefine(); 
        void prepare(Cons *pc);
        Cons *call(ArgList *arg_list, Environment * &envt, 
                    Continuation * &cont, FrameObj ** &top_ptr);
#ifdef DEBUG
        string _debug_repr();
#endif
        string ext_repr();
};

/** @class SpecialOptSet
 * The implementation of `set!` operator
 */
class SpecialOptSet: public SpecialOptObj {
    public:
        SpecialOptSet();
        void prepare(Cons *pc);
        Cons *call(ArgList *arg_list, Environment * &envt, 
                    Continuation * &cont, FrameObj ** &top_ptr);
#ifdef DEBUG
        string _debug_repr();
#endif
        string ext_repr();
};

#endif
