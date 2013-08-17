#ifndef MODEL_H
#define MODEL_H

#include <string>
#include <set>

using std::string;

// the range of unsigned char is enough for these types
typedef unsigned char FrameType;
typedef unsigned char NumLvl;

const int CLS_EVAL_OBJ = 1 << 1;
const int CLS_PAR_BRA = 1 << 2;

const int CLS_SIM_OBJ = 1 << 0;
const int CLS_PAIR_OBJ = 1 << 1;
const int CLS_CONTAINER = 1 << 20;

#define TO_PAIR(ptr) \
    (static_cast<Pair*>(ptr))

/** @class FrameObj
 * Objects that can be held in the parsing stack
 */
class FrameObj {/*{{{*/
    protected:
        /**
         * Report the type of the FrameObj, which can avoid the use of
         * dynamic_cast to improve efficiency. See the constructor for detail
         */
        FrameType ftype;
    public:
        /**
         * Construct a FrameObj
         * @param ftype describes the type of the FrameObj (CLS_EVAL_OBJ for an EvalObj,
         * CLS_PAR_BRA for a bracket)
         */
        FrameObj(FrameType ftype);
        /**
         * The destructor
         */
        virtual ~FrameObj() {}
        /**
         * Tell whether the object is a bracket, according to ftype
         * @return true for yes
         */
        bool is_parse_bracket();
};/*}}}*/

class GCRecord;
class Pair;
class ReprCons;
/** @class EvalObj
 * Objects that represents a value in evaluation
 */
class EvalObj : public FrameObj {/*{{{*/
    protected:
        /**
         * Report the type of the EvalObj, which can avoid the use of
         * dynamic_cast to improve efficiency. See the constructor for detail
         */
        int otype;
    public:
        /**
         * The pointer to the corresponding record in GC
         */
        GCRecord *gc_rec;
        /**
         * Construct an EvalObj
         * @param otype the type of the EvalObj (CLS_PAIR_OBJ for a pair,
         * CLS_SIM_OBJ for a simple object), which defaults to CLS_SIM_OBJ, etc.
         */
        EvalObj(int otype = CLS_SIM_OBJ);
        /**
         * The destructor
         */
        ~EvalObj();
        /** Check if the object is a simple object (instead of a call
         * invocation)
         * @return true if the object is not a pair or an empty list
         * */
        bool is_simple_obj();
        /** Check if the object is a symobl */
        bool is_sym_obj();
        /** Check if the object is an operator */
        bool is_opt_obj();
        /** Check if the object is a pair (notice that an empty list does
         * not counts as a pair here) */
        bool is_pair_obj();
        /** Check if the object is a number */
        bool is_num_obj();
        /** Check if the object is a boolean */
        bool is_bool_obj();
        /** Check if the object is a string */
        bool is_str_obj();
        /** Check if the object is a promise */
        bool is_prom_obj();
        /** Check if the object is a vector */
        bool is_vect_obj();
        /** Check if the object is a container */
        bool is_container();
        /** Get `otype`, used by some routines for efficiency issues */
        int get_otype();
        /** Dummy function, actually used by OptObj, called before the actual
         * invocation */
        virtual void prepare(Pair *pc);
        /** Any EvalObj has its external representation */
        string ext_repr();
        /** Always true for all other EvalObjs except BoolObj */
        virtual bool is_true();
        /** External representation construction, used by `ext_repr()` */
        virtual ReprCons *get_repr_cons() = 0;
};/*}}}*/

/** @class ParseBracket
 * To indiate a left bracket when parsing, used in the parse_stack
 */
class ParseBracket : public FrameObj {/*{{{*/
    public:
        /** The type of the bracket (to distingiush '(' and '#(' ) */
        unsigned char btype;            
        /** Construct a ParseBracket object */
        ParseBracket(unsigned char btype);
};/*}}}*/


/** @class Container 
 * Describes the kind of EvalObjs which may cause circular reference issue,
 * different from some "direct" objects, such as BoolObj, NumberObj...
 */
class Container: public EvalObj {/*{{{*/
    public:
    /** true if the object is still in use, its value is set and read temporarily,
    * check `gc.cpp` */
    bool keep;                  
    /** the counter used when resolving circular references, see `gc.cpp` */
    size_t gc_refs;
    /** Constructor, does an "or" bit operation on the otype provided by
     * its subclasses, i.e. to mark CLS_CONTAINER.
     * The mark will not be not enforced if the `override` is
     * true. */
    Container(int otype = 0, bool override = false);
    /**
     * Decrease the gc_refs of the objects referenced by this container.
     * Used by circular ref resolver to detect the cycle.
     */
    virtual void gc_decrement() = 0;
    /** Push the objects referenced by this container to the queue via the pointer `tail`. Used in the phase of
     * marking `keep`.
     */
    virtual void gc_trigger(EvalObj ** &tail) = 0;
};/*}}}*/

#endif
