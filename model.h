#ifndef MODEL_H
#define MODEL_H

#include <string>
#include <list>
#include <map>

using std::list;
using std::string;
using std::map;

typedef unsigned char ClassType;  // that range is enough

static const int CLS_RET_ADDR = 0;
static const int CLS_EVAL_OBJ = 1;
static const int CLS_SIM_OBJ = 0;
static const int CLS_CONS_OBJ = 1;

/** @class FrameObj
 * Objects that can be held in the evaluation stack
 */
class FrameObj {
    protected:
        ClassType ftype;   // avoid the use of dynamic_cast to improve efficiency
    public:
        virtual ~FrameObj() {}
        bool is_ret_addr();
#ifdef DEBUG
        virtual string _debug_repr() = 0;  
#endif
};


class Cons;
/** @class EvalObj
 * Objects that represents a value in evaluation
 */
class EvalObj : public FrameObj {
    private:
        ClassType otype;    // avoid the use of dynamic_cast to improve efficiency
    public:
        EvalObj();
        bool is_simple_obj();
        /** External representation of this object */
        virtual void prepare(Cons *pc);
        virtual string ext_repr() = 0;  
        /**< Always true for all EvalObjs except BoolObj */
        virtual bool is_true();         
#ifdef DEBUG
        virtual void _debug_print();
#endif
};

/** @class Cons
 * Pair construct, which can be used to represent a list, or further
 * more, a syntax tree
 * (car . cdr) in Scheme
 */
class Cons : public EvalObj {
    public:
        EvalObj *car;                   /**< car (as in Scheme) */
        Cons *cdr;                      /**< cdr (as in Scheme) */
        bool skip;                      /**< Wether to skip the current branch */
        Cons* next;                      /**< The next branch in effect         */

        Cons(EvalObj *, Cons *);
#ifdef DEBUG
        void _debug_print();
        string _debug_repr();
#endif
        string ext_repr();
};

/** @class EmptyList
 * The empty list (special situation of a list)
 */
class EmptyList: public Cons {
    public:
        EmptyList();
#ifdef DEBUG
        string _debug_repr();
#endif
        string ext_repr();
};

/** @class RetAddr
 * Tracking the caller's Cons pointer
 */
class RetAddr : public FrameObj {
    public:
        Cons* addr;                      /**< The return address  */

        RetAddr(Cons *);
        string _debug_repr();
};


/** @class UnspecObj
 * The "unspecified" value returned by some builtin procedures
 */
class UnspecObj: public EvalObj {
    public:
        UnspecObj();
#ifdef DEBUG
        string _debug_repr();
#endif
        string ext_repr();
};

/** @class SymObj
 * Symbols
 */
class SymObj: public EvalObj {
    public:
        string val;

        SymObj(const string &);
#ifdef DEBUG
        string _debug_repr();
#endif
        string ext_repr();
};

// Everything is cons
typedef Cons ASTList;
typedef Cons SymbolList;
typedef Cons ArgList;
class Environment;
class Continuation;

/** @class OptObj
 * "Operators" in general sense
 */
class OptObj: public EvalObj {
    public:
        OptObj();
        /**
         * The function is called when an operation is needed.
         * @param arg_list The argument list (the first one is the opt itself)
         * @param envt The current environment (may be modified)
         * @param cont The current continuation (may be modified)
         * @param top_ptr Pointing to the top of the stack (may be modified)
         * @return New value for pc register 
         */
        virtual Cons *call(ArgList *arg_list, Environment * &envt, 
                            Continuation * &cont, FrameObj ** &top_ptr) = 0;
};

/** @class ProcObj
 * User-defined procedures
 */
class ProcObj: public OptObj {
    public:
        /** The procedure body, a list of expressions to be evaluated */
        ASTList *body;        
        /** The arguments, a list of Symbols */
        SymbolList *para_list;
        /** Pointer to the environment */
        Environment *envt;

        ProcObj(ASTList *, Environment *, SymbolList *);
        Cons *call(ArgList *arg_list, Environment * &envt,
                    Continuation * &cont, FrameObj ** &top_ptr);
#ifdef DEBUG
        string _debug_repr();
#endif
        string ext_repr();
};

/** @class SpecialOptObj
 * Special builtin syntax (`if`, `define`, `lambda`, etc.)
 */
class SpecialOptObj: public OptObj {
    public:
        SpecialOptObj();
};

typedef EvalObj* (*BuiltinProc)(ArgList *);
/** @class BuiltinProcObj
 * Wrapping class for builtin procedures (arithmetic operators, etc.)
 */
class BuiltinProcObj: public OptObj {
    private:
        /** The function that tackle the inputs in effect */
        BuiltinProc handler;        
        string name;
    public:
        BuiltinProcObj(BuiltinProc, const string &);
        Cons *call(ArgList *arg_list, Environment * &envt,
                    Continuation * &cont, FrameObj ** &top_ptr);
};

/** @class NumberObj
 * The top level abstract of numbers
 */

class NumberObj: public EvalObj {
    public:
        NumberObj();
};

typedef map<string, EvalObj*> Str2EvalObj;
/** @class Environment
 * The environment of current evaluation, i.e. the local variable binding 
 */
class Environment {
    private:
        Environment *prev_envt; /**< Pointer to the upper level environment */
        Str2EvalObj binding;    /**< Store all pairs of identifier and its
                                  corresponding obj */
    public:
        Environment(Environment * = NULL);
        void add_binding(SymObj *, EvalObj *);  
        EvalObj *get_obj(SymObj *);
        bool has_obj(SymObj *);
};

class Continuation {
    public:
        Continuation *prev_cont;
        Environment *envt;
        Cons *pc;
        ASTList *proc_body;
        unsigned int body_cnt;

        Continuation(Environment *, Cons *, Continuation *, 
                ASTList *, unsigned int = 0);
};

#endif
