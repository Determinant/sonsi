#ifndef TYPES_H
#define TYPES_H

#include "model.h"
#include <string>
#include <list>
#include <map>
#include <vector>
#include <set>
#include <gmpxx.h>

using std::list;
using std::string;
using std::map;
using std::vector;
using std::set;

const int CLS_OPT_OBJ = 1 << 3;
const int CLS_PROM_OBJ = 1 << 9;

const int CLS_SYM_OBJ = 1 << 2;
const int CLS_NUM_OBJ = 1 << 4;
const int CLS_BOOL_OBJ = 1 << 5;
const int CLS_CHAR_OBJ = 1 << 6;
const int CLS_STR_OBJ = 1 << 7;
const int CLS_VECT_OBJ = 1 << 8;

const int CLS_CONT_OBJ = 1 << 9;
const int CLS_ENVT_OBJ = 1 << 10;

static const int NUM_LVL_COMP = 0;
static const int NUM_LVL_REAL = 1;
static const int NUM_LVL_RAT = 2;
static const int NUM_LVL_INT = 3;

typedef set<EvalObj*> EvalObjAddrHash;
typedef vector<EvalObj*> EvalObjVec;
typedef map<string, EvalObj*> Str2EvalObj;
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
        EvalObj *cdr;                      /**< cdr (as in Scheme) */
        Pair* next;                     /**< The next branch in effect */

        Pair(EvalObj *car, EvalObj *cdr);  /**< Create a Pair (car . cdr) */
        ~Pair();
        ReprCons *get_repr_cons();
        void gc_decrement();
        void gc_trigger(EvalObj ** &tail, EvalObjSet &visited);
};/*}}}*/

/** @class EmptyList
 * The empty list (special situation of a list)
 */
class EmptyList: public Pair {/*{{{*/
    public:
        EmptyList();
        ReprCons *get_repr_cons();
};/*}}}*/

class ReprCons {/*{{{*/
    public:
        EvalObj *ori;
        bool done;
        string repr;
        ReprCons(bool done, EvalObj *ori = NULL);
        virtual EvalObj *next(const string &prev) = 0;
};/*}}}*/

class ReprStr : public ReprCons {/*{{{*/
    public:
        ReprStr(string repr);
        EvalObj *next(const string &prev);
};/*}}}*/

class PairReprCons : public ReprCons {/*{{{*/
    private:
        int state;
        EvalObj *ptr;
    public:
        PairReprCons(Pair *ptr, EvalObj *ori);
        EvalObj *next(const string &prev);
};/*}}}*/

class VecObj;
class VectReprCons : public ReprCons {/*{{{*/
    private:
        VecObj *ptr;
        size_t idx;
    public:
        VectReprCons(VecObj *ptr, EvalObj *ori);
        EvalObj *next(const string &prev);
};/*}}}*/

/** @class ParseBracket
 * To indiate a left bracket when parsing, used in the parse_stack
 */
class ParseBracket : public FrameObj {/*{{{*/
    public:
        unsigned char btype;            /**< The type of the bracket */
        /** Construct a ParseBracket object */
        ParseBracket(unsigned char btype);
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
        string val;

        SymObj(const string &);
        ReprCons *get_repr_cons();
};/*}}}*/

// Everything is cons
class Environment;
class Continuation;

/** @class OptObj
 * "Operators" in general sense
 */
class OptObj: public Container {/*{{{*/
    public:

        OptObj(int otype = 0);
        /**
         * The function is called when an operation is needed.
         * @param args The argument list (the first one is the opt itself)
         * @param envt The current environment (may be modified)
         * @param cont The current continuation (may be modified)
         * @param top_ptr Pointing to the top of the stack (may be modified)
         * @return New value for pc register
         */
        virtual Pair *call(Pair *args, Environment * &envt,
                            Continuation * &cont, EvalObj ** &top_ptr, Pair *pc) = 0;
        virtual void gc_decrement();
        virtual void gc_trigger(EvalObj ** &tail, EvalObjSet &visited);

};/*}}}*/

/** @class ProcObj
 * User-defined procedures
 */
class ProcObj: public OptObj {/*{{{*/
    public:
        /** The procedure body, a list of expressions to be evaluated */
        Pair *body;
        /** The arguments: <list> | var1 ... | var1 var2 ... . varn */
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
        void gc_trigger(EvalObj ** &tail, EvalObjSet &visited);
};/*}}}*/

/** @class SpecialOptObj
 * Special builtin syntax (`if`, `define`, `lambda`, etc.)
 */
class SpecialOptObj: public OptObj {/*{{{*/
    protected:
        string name;
    public:
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
        virtual NumObj *clone() const = 0;
        bool is_exact();
        virtual NumObj *convert(NumObj *r) = 0;
        virtual void add(NumObj *r) = 0;
        virtual void sub(NumObj *r) = 0;
        virtual void mul(NumObj *r) = 0;
        virtual void div(NumObj *r) = 0;
        virtual void abs();

        virtual bool lt(NumObj *r);
        virtual bool gt(NumObj *r);
        virtual bool le(NumObj *r);
        virtual bool ge(NumObj *r);
        virtual bool eq(NumObj *r) = 0;
};/*}}}*/

/** @class StrObj
 * String support
 */
class StrObj: public EvalObj {/*{{{*/
    public:
        string str;

        /** Construct a string object */
        StrObj(string str);
        /** Try to construct an StrObj object
         * @return NULL if failed
         */
        static StrObj *from_string(string repr);
        bool lt(StrObj *r);
        bool gt(StrObj *r);
        bool le(StrObj *r);
        bool ge(StrObj *r);
        bool eq(StrObj *r);
        ReprCons *get_repr_cons();
};/*}}}*/

/** @class CharObj
 * Character type support
 */
class CharObj: public EvalObj {/*{{{*/
    public:
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
        EvalObjVec vec;
        /** Construct a vector object */
        VecObj(size_t size = 0, EvalObj *fill = NULL);
        ~VecObj();
        size_t get_size();
        /** Add a new element to the rear */
        void push_back(EvalObj *new_elem);
        /** Fill the vector with obj */
        void fill(EvalObj *obj);
        /** Replace an existing element in the vector */
        void set(size_t idx, EvalObj *obj);
        EvalObj *get(size_t idx);
        ReprCons *get_repr_cons();

        void gc_decrement();
        void gc_trigger(EvalObj ** &tail, EvalObjSet &visited);
};/*}}}*/

/**
 * @class PromObj
 * Promise support (partial)
 */
class PromObj: public Container {/*{{{*/
    private:
        Pair *entry;
        EvalObj *mem;
    public:
        PromObj(EvalObj *exp);
        ~PromObj();
        Pair *get_entry();
        EvalObj *get_mem();
        void feed_mem(EvalObj *res);
        ReprCons *get_repr_cons();

        void gc_decrement();
        void gc_trigger(EvalObj ** &tail, EvalObjSet &visited);
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
        Environment *get_prev();

        void gc_decrement();
        void gc_trigger(EvalObj ** &tail, EvalObjSet &visited);
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
        void gc_trigger(EvalObj ** &tail, EvalObjSet &visited);
};/*}}}*/

/** @class InexactNumObj
 * Inexact number implementation (using doubles)
 */
class InexactNumObj: public NumObj {/*{{{*/
    public:
        InexactNumObj(NumLvl level);
};/*}}}*/

/** @class CompNumObj
 * Complex numbers
 */
class CompNumObj: public InexactNumObj {/*{{{*/
    public:
        double real, imag;

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
        ExactNumObj(NumLvl level);
        virtual IntNumObj *to_int() = 0;
};/*}}}*/

/** @class RatNumObj
 * Rational numbers
 */
class RatNumObj: public ExactNumObj {/*{{{*/
    public:
#ifndef GMP_SUPPORT
        int a, b;
        /** Construct a rational number */
        RatNumObj(int _a, int _b);
#else
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
        int val;
        /** Construct a integer */
        IntNumObj(int val);
        int get_i();
#else
        mpz_class val;
        /** Construct a integer */
        IntNumObj(mpz_class val);
        int get_i();
        /** Copy constructor */
        IntNumObj *to_int();
        IntNumObj(const IntNumObj &ori);
#endif
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
        void mod(NumObj *r);
        void rem(NumObj *r);
        void quo(NumObj *r);
        void gcd(NumObj *r);
        void lcm(NumObj *r);

        bool lt(NumObj *r);
        bool gt(NumObj *r);
        bool le(NumObj *r);
        bool ge(NumObj *r);
        bool eq(NumObj *r);

        ReprCons *get_repr_cons();
};/*}}}*/

bool is_zero(double);
#endif
