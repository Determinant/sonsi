#ifndef MODEL_H
#define MODEL_H

#include <string>
#include <list>
#include <map>

using std::list;
using std::string;
using std::map;

typedef unsigned char ClassType;  // that range is enough

static const int CLS_RET_ADDR = 1 << 0;
static const int CLS_EVAL_OBJ = 1 << 1;

static const int CLS_SIM_OBJ = 1 << 0;
static const int CLS_CONS_OBJ = 1 << 1;
static const int CLS_SYM_OBJ = 1 << 2;
static const int CLS_OPT_OBJ = 1 << 3;

/** @class FrameObj
 * Objects that can be held in the evaluation stack
 */
class FrameObj {
    protected:
        /**
         * Report the type of the FrameObj, which can avoid the use of
         * dynamic_cast to improve efficiency. See the constructor for detail
         */
        ClassType ftype;   
    public:
        /**
         * Construct an EvalObj
         * @param ftype the type of the FrameObj (CLS_EVAL_OBJ for an EvalObj,
         * CLS_RET_ADDR for a return address)
         */
        FrameObj(ClassType ftype);
        virtual ~FrameObj() {}
        /**
         * Tell whether the object is a return address, according to ftype
         * @return true for yes
         */
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
    protected:
        /**
         * Report the type of the EvalObj, which can avoid the use of
         * dynamic_cast to improve efficiency. See the constructor for detail
         */
        ClassType otype;
    public:
        /**
         * Construct an EvalObj
         * @param otype the type of the EvalObj (CLS_CONS_OBJ for a
         * construction, CLS_SIM_OBJ for a simple object), which defaults to
         * CLS_SIM_OBJ
         */
        EvalObj(ClassType otype = CLS_SIM_OBJ);
        /** Check if the object is a simple object (instead of a call
         * invocation) 
         * @return true if the object is not a construction (Cons)
         * */
        bool is_simple_obj();
        /** Check if the object is a symobl */
        bool is_sym_obj();
        /** Check if the object is an operator */
        bool is_opt_obj();
        virtual void prepare(Cons *pc);
        /** Any EvalObj has its external representation */
        virtual string ext_repr() = 0;  
        /** Always true for all EvalObjs except BoolObj */
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
        Cons* next;                     /**< The next branch in effect */

        Cons(EvalObj *car, Cons *cdr);  /**< Create a Cons (car . cdr) */
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
        /** Constructs a return address object which refers to the node addr in
         * the AST */
        RetAddr(Cons *addr);
#ifdef DEBUG
        string _debug_repr();
#endif
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
         * @param args The argument list (the first one is the opt itself)
         * @param envt The current environment (may be modified)
         * @param cont The current continuation (may be modified)
         * @param top_ptr Pointing to the top of the stack (may be modified)
         * @return New value for pc register 
         */
        virtual Cons *call(ArgList *args, Environment * &envt, 
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

        /** Conctructs a ProcObj */
        ProcObj(ASTList *body, Environment *envt, SymbolList *para_list);
        Cons *call(ArgList *args, Environment * &envt,
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
        /**
         * Make a BuiltinProcObj which invokes proc when called
         * @param proc the actual handler
         * @param name the name of this built-in procedure
         */
        BuiltinProcObj(BuiltinProc proc, string name);
        Cons *call(ArgList *args, Environment * &envt,
                    Continuation * &cont, FrameObj ** &top_ptr);
#ifdef DEBUG
        string _debug_repr();
#endif
        string ext_repr();
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
        Environment *prev_envt; /**< Pointer to the upper-level environment */
        Str2EvalObj binding;    /**< Store all pairs of identifier and its
                                  corresponding obj */
    public:
        /** Create an runtime environment
         * @param prev_envt the outer environment
         */
        Environment(Environment *prev_envt);
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
};

/** @class Continuation
 * Save the registers and necessary information when a user-defined call is
 * being made (Behave like a stack frame in C). When the call has accomplished,
 * the system will restore all the registers according to the continuation.
 */
class Continuation {
    public:
        /** Linking the previous continuation on the chain */
        Continuation *prev_cont;    
        Environment *envt;  /**< The saved envt */
        Cons *pc;           /**< The saved pc */
        /** Pointing to the current expression that is being evaluated.
         * When its value goes to empty_list, the call is accomplished.
         */
        ASTList *proc_body;

        /** Create a continuation */
        Continuation(Environment *envt, Cons *pc, Continuation *prev_cont, 
                ASTList *proc_body);
};

#endif
