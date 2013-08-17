#ifndef TYPES_H
#define TYPES_H

#include "model.h"

#include <string>
#include <list>
#include <map>
#include <vector>
#include <set>
#include <gmpxx.h>

using std::string;

const int CLS_PROM_OBJ = 1 << 9;
const int CLS_SYM_OBJ = 1 << 2;
const int CLS_NUM_OBJ = 1 << 4;
const int CLS_BOOL_OBJ = 1 << 5;
const int CLS_CHAR_OBJ = 1 << 6;
const int CLS_STR_OBJ = 1 << 7;
const int CLS_VECT_OBJ = 1 << 8;

const int CLS_OPT_OBJ = 1 << 3;
const int CLS_CONT_OBJ = 1 << 9;
const int CLS_ENVT_OBJ = 1 << 10;

static const int NUM_LVL_COMP = 0;
static const int NUM_LVL_REAL = 1;
static const int NUM_LVL_RAT = 2;
static const int NUM_LVL_INT = 3;

typedef std::vector<EvalObj*> EvalObjVec;
typedef std::map<string, EvalObj*> Str2EvalObj;
typedef EvalObj* (*BuiltinProc)(Pair *, const string &);

class PairReprCons;
/** @class Pair
 * Pair construct, which can be used to represent a list, or further
 * more, a syntax tree
 * (car . cdr) in Scheme
 */
class Pair : public Container {/*{{{*/
    public:
        EvalObj *car;                   /**< car (as in Scheme) */
        EvalObj *cdr;                   /**< cdr (as in Scheme) */
        /** The next branch in effect (make sense only in evaluation) */
        Pair* next;

        Pair(EvalObj *car, EvalObj *cdr);   /**< Create a Pair (car . cdr) */
        ~Pair();                            /**< The destructor */
        ReprCons *get_repr_cons();
        void gc_decrement();
        void gc_trigger(EvalObj ** &tail);
};/*}}}*/

/** @class EmptyList
 * The empty list (special situation of a list)
 */
class EmptyList: public Pair {/*{{{*/
    public:
        EmptyList();        /**< The constructor */
        ReprCons *get_repr_cons();
};/*}}}*/

/** @class ReprCons
 * The abstraction class to represent a representation construction, which is
 * used as stack frame in `ext_repr`.
 */
class ReprCons {/*{{{*/
    public:
        EvalObj *ori;           /**< Reflexive pointer to the obj */
        bool prim;             /**< true if no further expansion is needed */
        /** The finally generated external represenation of the EvalObj */
        string repr;
        /** The constructor */
        ReprCons(bool prim, EvalObj *ori = NULL);
        /** This function is called to get the next component in a complex
         * EvalObj
         * @param prev Feed the string form of the previous component */
        virtual EvalObj *next(const string &prev) = 0;
};/*}}}*/

/** @class ReprStr
 * Used to store the generated external representation.
 */
class ReprStr : public ReprCons {/*{{{*/
    public:
        /** The constructor */
        ReprStr(string repr);
        EvalObj *next(const string &prev);
};/*}}}*/

/** @class PairReprCons
 * The ReprCons implementation of Pair
 */
class PairReprCons : public ReprCons {/*{{{*/
    private:
        int state;
        EvalObj *ptr;
    public:
        /** The constructor */
        PairReprCons(Pair *ptr, EvalObj *ori);
        EvalObj *next(const string &prev);
};/*}}}*/

class VecObj;

/** @class VectReprCons
 * The ReprCons implementation of VecObj
 */
class VectReprCons : public ReprCons {/*{{{*/
    private:
        VecObj *ptr;
        size_t idx;
    public:
        /** The constructor */
        VectReprCons(VecObj *ptr, EvalObj *ori);
        EvalObj *next(const string &prev);
};/*}}}*/


/** @class UnspecObj
 * The "unspecified" value returned by some builtin procedures
 */
class UnspecObj: public EvalObj {/*{{{*/
    public:
        UnspecObj();
        ReprCons *get_repr_cons();
};/*}}}*/

/** @class SymObj
 * Symbols
 */
class SymObj: public EvalObj {/*{{{*/
    public:
        /** Storage implementation: string */
        string val;
        /** The constructor */
        SymObj(const string &);
        ReprCons *get_repr_cons();
};/*}}}*/

class Environment;
class Continuation;

/** @class OptObj
 * "Operators" in general sense
 */
class OptObj: public Container {/*{{{*/
    public:

        /** The constructor */
        OptObj(int otype = 0);
        /**
         * The function is called when an operation is needed.
         * @param args The argument list (the first one is the opt itself)
         * @param envt The current environment (may be modified)
         * @param cont The current continuation (may be modified)
         * @param top_ptr Pointing to the top of the stack (may be modified)
         * @param pc Pointing to the entry of the call in AST
         * @return New value for pc register
         */
        virtual Pair *call(Pair *args, Environment * &envt,
                            Continuation * &cont, EvalObj ** &top_ptr, Pair *pc) = 0;
        virtual void gc_decrement();
        virtual void gc_trigger(EvalObj ** &tail);

};/*}}}*/

/** @class ProcObj
 * User-defined procedures
 */
class ProcObj: public OptObj {/*{{{*/
    public:
        /** The procedure body, a list of expressions to be evaluated */
        Pair *body;
        /** The arguments: \<list\> | var_1 ... | var_1 var_2 ... . var_n */
        EvalObj *params;
        /** Pointer to the environment */
        Environment *envt;

        /** Conctructs a ProcObj */
        ProcObj(Pair *body, Environment *envt, EvalObj *params);
        ~ProcObj();
        Pair *call(Pair *args, Environment * &envt,
                    Continuation * &cont, EvalObj ** &top_ptr, Pair *pc);
        ReprCons *get_repr_cons();

        void gc_decrement();
        void gc_trigger(EvalObj ** &tail);
};/*}}}*/

/** @class SpecialOptObj
 * Special builtin syntax (`if`, `define`, `lambda`, etc.)
 */
class SpecialOptObj: public OptObj {/*{{{*/
    protected:
        /** The name of this operator */
        string name;
    public:
        /** The constructor */
        SpecialOptObj(string name);
        ReprCons *get_repr_cons();
};/*}}}*/

/** @class BuiltinProcObj
 * Wrapping class for builtin procedures (arithmetic operators, etc.)
 */
class BuiltinProcObj: public OptObj {/*{{{*/
    private:
        /** The function that tackle the inputs in effect */
        BuiltinProc handler;
        string name;
    public:
        /**
         * Make a BuiltinProcObj which invokes proc when called
         * @param proc the actual handler
         * @param name the name of this built-in procedure
         */
        BuiltinProcObj(BuiltinProc proc, string name);
        Pair *call(Pair *args, Environment * &envt,
                    Continuation * &cont, EvalObj ** &top_ptr, Pair *pc);
        ReprCons *get_repr_cons();
};/*}}}*/

/** @class BoolObj
 * Booleans
 */
class BoolObj: public EvalObj {/*{{{*/
    public:
        bool val;                       /**< true for \#t, false for \#f */
        BoolObj(bool);                  /**< Converts a C bool value to a BoolObj*/
        bool is_true();                 /**< Override EvalObj `is_true()` */
        ReprCons *get_repr_cons();
        /** Try to construct an BoolObj object
         * @return NULL if failed
         */
        static BoolObj *from_string(string repr);
};/*}}}*/

/** @class NumObj
 * The top level abstract of numbers
 */
class NumObj: public EvalObj {/*{{{*/
    protected:
        /** True if the number is of exact value */
        bool exactness;
    public:
        /** The level of the specific number. The smaller the level
         * is, the more generic that number is.
         */
        NumLvl level;

        /**
         * Construct a general Numeric object
         */
        NumObj(NumLvl level, bool _exactness);
        /** Deep-copy a NumObj */
        virtual NumObj *clone() const = 0;
        /** @return true if it's an exact numeric value */
        bool is_exact();
        /** Upper convert a NumObj `r` to the same type as `this` */
        virtual NumObj *convert(NumObj *r) = 0;
        virtual void add(NumObj *r) = 0;    /**< Addition */
        virtual void sub(NumObj *r) = 0;    /**< Substraction */
        virtual void mul(NumObj *r) = 0;    /**< Multiplication */
        virtual void div(NumObj *r) = 0;    /**< Division */
        virtual void abs();                 /**< Absolute function */

        virtual bool lt(NumObj *r);         /**< "<" implementation of numbers */
        virtual bool gt(NumObj *r);         /**< ">" implementation of numbers */
        virtual bool le(NumObj *r);         /**< "<=" implementation of numbers */
        virtual bool ge(NumObj *r);         /**< ">=" implementation of numbers */
        virtual bool eq(NumObj *r) = 0;     /**< "=" implementation of numbers */
};/*}}}*/

/** @class StrObj
 * String support
 */
class StrObj: public EvalObj {/*{{{*/
    public:
        /** Storage implementation: C++ string */
        string str;

        /** Construct a string object */
        StrObj(string str);
        /** Try to construct an StrObj object
         * @return NULL if failed
         */
        static StrObj *from_string(string repr);

        bool lt(StrObj *r);     /**< "<" implementation of strings */
        bool gt(StrObj *r);     /**< ">" implementation of strings */
        bool le(StrObj *r);     /**< "<=" implementation of strings */
        bool ge(StrObj *r);     /**< ">=" implementation of strings */
        bool eq(StrObj *r);     /**< "="  implementation of strings */

        ReprCons *get_repr_cons();
};/*}}}*/

/** @class CharObj
 * Character type support
 */
class CharObj: public EvalObj {/*{{{*/
    public:
        /** Storage implementation */
        char ch;
        /** Construct a string object */
        CharObj(char ch);
        /** Try to construct an CharObj object
         * @return NULL if failed
         */
        static CharObj *from_string(string repr);
        ReprCons *get_repr_cons();
};/*}}}*/


/**
 * @class VecObj
 * Vector support (currently a wrapper of STL vector)
 */
class VecObj: public Container {/*{{{*/
    public:
        /** Storage implementation: C++ STL vector */
        EvalObjVec vec;
        /** Construct a vector object */
        VecObj(size_t size = 0, EvalObj *fill = NULL);
        ~VecObj();
        /** Get the actual size of this vector */
        size_t get_size();
        /** Add a new element to the rear */
        void push_back(EvalObj *new_elem);
        /** Fill the vector with obj */
        void fill(EvalObj *obj);
        /** Replace an existing element in the vector */
        void set(size_t idx, EvalObj *obj);
        /** Get the pointer to an element by index */
        EvalObj *get(size_t idx);
        ReprCons *get_repr_cons();

        void gc_decrement();
        void gc_trigger(EvalObj ** &tail);
};/*}}}*/

/**
 * @class PromObj
 * Promise support (partial)
 */
class PromObj: public Container {/*{{{*/
    private:
        /** The delayed expression */
        Pair *exp;
        /** The memorized result */
        EvalObj *mem;
    public:
        /** Construct with a delayed expression */
        PromObj(EvalObj *_exp);
        /** The destructor */
        ~PromObj();
        /** Get the delayed expression */
        Pair *get_exp();
        /** Extract the memorized result */
        EvalObj *get_mem();
        /** Provide with the result to let the PromObj remember */
        void feed_mem(EvalObj *res);
        ReprCons *get_repr_cons();

        void gc_decrement();
        void gc_trigger(EvalObj ** &tail);
};/*}}}*/

/** @class Environment
 * The environment of current evaluation, i.e. the local variable binding
 */
class Environment : public Container{/*{{{*/
    private:
        Environment *prev_envt; /**< Pointer to the upper-level environment */
        Str2EvalObj binding;    /**< Store all pairs of identifier and its
                                  corresponding obj */
    public:
        /** Create an runtime environment
         * @param prev_envt the outer environment
         */
        Environment(Environment *prev_envt);
        ~Environment();
        /** Add a binding entry which binds sym_obj to eval_obj
         * @param def true to force the assignment
         * @param sym_obj SymbolObj which provides with the identifier
         * @param eval_obj EvalObj to which the identifier is binding
         * @return when def is set to false, this return value is true iff. the
         * assignment carried out successfully
         */
        bool add_binding(SymObj *sym_obj, EvalObj *eval_obj, bool def = true);
        /** Extract the corresponding EvalObj if obj is a SymObj, or just
         * simply return obj as it is
         * @param obj the object as request
         * */
        EvalObj *get_obj(EvalObj *obj);
        ReprCons *get_repr_cons();

        void gc_decrement();
        void gc_trigger(EvalObj ** &tail);
};/*}}}*/

/** @class Continuation
 * Save the registers and necessary information when a user-defined call is
 * being made (Behave like a stack frame in C). When the call has accomplished,
 * the system will restore all the registers according to the continuation.
 */
class Continuation : public Container {/*{{{*/
    public:
        /** Linking the previous continuation on the chain */
        Continuation *prev_cont;
        Environment *envt;  /**< The saved envt */
        Pair *pc;           /**< The saved pc */
        Pair *state;        /**< The state of this compound */
        Pair *prog;         /**< Pointing to ast */
        bool tail;          /**< If the proper tail opt is on */

        /** Create a continuation */
        Continuation(Environment *envt, Pair *pc, Continuation *prev_cont);
        ~Continuation();
        ReprCons *get_repr_cons();

        void gc_decrement();
        void gc_trigger(EvalObj ** &tail);
};/*}}}*/

/** @class InexactNumObj
 * Inexact number implementation (using doubles)
 */
class InexactNumObj: public NumObj {/*{{{*/
    public:
        /** Construct an InexactNumObj */
        InexactNumObj(NumLvl level);
};/*}}}*/

/** @class CompNumObj
 * Complex numbers
 */
class CompNumObj: public InexactNumObj {/*{{{*/
    public:
        /** Storage implementation: real part */
        double real;
        /** Storage implementation: imaginary part */
        double imag;

        /** Construct a complex number */
        CompNumObj(double _real, double _imag);
        NumObj *clone() const;
        /** Try to construct an CompNumObj object
         * @return NULL if failed
         */
        static CompNumObj *from_string(string repr);
        /** Convert to a complex number from other numeric types */
        CompNumObj *convert(NumObj* obj);

        void add(NumObj *r);
        void sub(NumObj *r);
        void mul(NumObj *r);
        void div(NumObj *r);
        bool eq(NumObj *r);
        ReprCons *get_repr_cons();
};/*}}}*/

/** @class RealNumObj
 * Real numbers
 */
class RealNumObj: public InexactNumObj {/*{{{*/
    public:
        /** Storage implementation: real part */
        double real;
        /** Construct a real number */
        RealNumObj(double _real);
        NumObj *clone() const;
        /** Try to construct an RealNumObj object
         * @return NULL if failed
         */
        static RealNumObj *from_string(string repr);
        /** Convert to a real number from other numeric types */
        RealNumObj *convert(NumObj* obj);

        void add(NumObj *r);
        void sub(NumObj *r);
        void mul(NumObj *r);
        void div(NumObj *r);
        void abs();
        bool lt(NumObj *r);
        bool gt(NumObj *r);
        bool le(NumObj *r);
        bool ge(NumObj *r);
        bool eq(NumObj *r);
        ReprCons *get_repr_cons();

};/*}}}*/


class IntNumObj;
/** @class ExactNumObj
 * Exact number implementation (using gmp)
 */
class ExactNumObj: public NumObj {/*{{{*/
    public:
        /** Construct an ExactNumObj */
        ExactNumObj(NumLvl level);
        /** Convert an exact number to an integer.  An exception will be
         * throwed if it fails.  */
        virtual IntNumObj *to_int() = 0;
};/*}}}*/

/** @class RatNumObj
 * Rational numbers
 */
class RatNumObj: public ExactNumObj {/*{{{*/
    public:
#ifndef GMP_SUPPORT
        /** Storage implementation of numerator: C-integer */
        int a;
        /** Storage implementation of denominator: C-integer */
        int b;
        /** Construct a rational number */
        RatNumObj(int _a, int _b);
#else
        /** Storage implementation: GMP Rational */
        mpq_class val;
        RatNumObj(mpq_class val);
        RatNumObj(const RatNumObj &ori);
        IntNumObj *to_int();
#endif
        NumObj *clone() const;
        /** Try to construct an RatNumObj object
         * @return NULL if failed
         */
        static RatNumObj *from_string(string repr);
        /** Convert to a Rational number from other numeric types */
        RatNumObj *convert(NumObj* obj);

        void add(NumObj *r);
        void sub(NumObj *r);
        void mul(NumObj *r);
        void div(NumObj *r);
        void abs();
        bool lt(NumObj *r);
        bool gt(NumObj *r);
        bool le(NumObj *r);
        bool ge(NumObj *r);
        bool eq(NumObj *r);

        ReprCons *get_repr_cons();
};/*}}}*/

/** @class IntNumObj
 * Integers
 */
class IntNumObj: public ExactNumObj {/*{{{*/
    public:
#ifndef GMP_SUPPORT
        /** Storage implementation: C-integer */
        int val;
        /** Construct a integer */
        IntNumObj(int val);
#else
        /** Storage implementation: GMP integer */
        mpz_class val;
        /** Construct a integer */
        IntNumObj(mpz_class val);
        /** Copy constructor */
        IntNumObj *to_int();
        IntNumObj(const IntNumObj &ori);
#endif
        /** Get the C-format integer representation of the storage
         * implementation value */
        int get_i();
        NumObj *clone() const;
        /** Try to construct an IntNumObj object
         * @return NULL if failed
         */
        static IntNumObj *from_string(string repr);
        /** Convert to a integer from other numeric types */
        IntNumObj *convert(NumObj* obj);

        void add(NumObj *r);
        void sub(NumObj *r);
        void mul(NumObj *r);
        void div(NumObj *r);
        void abs();

        void mod(NumObj *r);    /**< Modulo */
        void rem(NumObj *r);    /**< Remainder */
        void quo(NumObj *r);    /**< Quotient */
        void gcd(NumObj *r);    /**< The greatest common divisor */
        void lcm(NumObj *r);    /**< The least common divisor */

        bool lt(NumObj *r);
        bool gt(NumObj *r);
        bool le(NumObj *r);
        bool ge(NumObj *r);
        bool eq(NumObj *r);

        ReprCons *get_repr_cons();
};/*}}}*/

bool is_zero(double);
#endif
