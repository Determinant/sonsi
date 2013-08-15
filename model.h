#ifndef MODEL_H
#define MODEL_H

#include <string>
#include <set>

using std::string;

// the range of unsigned char is enough for these types
typedef unsigned char FrameType;
typedef unsigned char NumLvl;

const int CLS_RET_ADDR = 1 << 0;
const int CLS_EVAL_OBJ = 1 << 1;
const int CLS_PAR_BRA = 1 << 2;

const int CLS_SIM_OBJ = 1 << 0;
const int CLS_PAIR_OBJ = 1 << 1;
const int CLS_CONTAINER = 1 << 20;

#define TO_PAIR(ptr) \
    (static_cast<Pair*>(ptr))


/** @class FrameObj
 * Objects that can be held in the evaluation stack
 */
class FrameObj {
    protected:
        /**
         * Report the type of the FrameObj, which can avoid the use of
         * dynamic_cast to improve efficiency. See the constructor for detail
         */
        FrameType ftype;
    public:
        /**
         * Construct an EvalObj
         * @param ftype the type of the FrameObj (CLS_EVAL_OBJ for an EvalObj,
         * CLS_RET_ADDR for a return address)
         */
        FrameObj(FrameType ftype);
        virtual ~FrameObj() {}
        /**
         * Tell whether the object is a bracket, according to ftype
         * @return true for yes
         */
        bool is_parse_bracket();
};


class Pair;
class ReprCons;
/** @class EvalObj
 * Objects that represents a value in evaluation
 */
class EvalObj : public FrameObj {
    private:
        size_t gc_cnt;
    protected:
        /**
         * Report the type of the EvalObj, which can avoid the use of
         * dynamic_cast to improve efficiency. See the constructor for detail
         */
        int otype;
    public:
        /**
         * Construct an EvalObj
         * @param otype the type of the EvalObj (CLS_PAIR_OBJ for a
         * construction, CLS_SIM_OBJ for a simple object), which defaults to
         * CLS_SIM_OBJ
         */
        EvalObj(int otype = CLS_SIM_OBJ);
        ~EvalObj();
        /** Check if the object is a simple object (instead of a call
         * invocation)
         * @return true if the object is not a construction (Pair)
         * */
        bool is_simple_obj();
        /** Check if the object is a symobl */
        bool is_sym_obj();
        /** Check if the object is an operator */
        bool is_opt_obj();
        /** Check if the object is a Pair */
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
        bool is_container();
        int get_otype();
        virtual void prepare(Pair *pc);
        /** Any EvalObj has its external representation */
        string ext_repr();
        /** Always true for all EvalObjs except BoolObj */
        virtual bool is_true();
        /** External representation construction */
        virtual ReprCons *get_repr_cons() = 0;
        bool gc_dec();
        void gc_inc();
        size_t gc_get_cnt();
};

typedef std::set<EvalObj*> EvalObjSet;
class Container: public EvalObj {
    public:
    bool keep;
    size_t gc_refs;
    Container(int otype = 0, bool override = false);
    virtual void gc_decrement() = 0;
    virtual void gc_trigger(EvalObj ** &tail, EvalObjSet &visited) = 0;
};

#endif
