#ifndef GC_H
#define GC_H

#include "model.h"
#include <map>

const int GC_QUEUE_SIZE = 262144;
const size_t GC_CYC_THRESHOLD = GC_QUEUE_SIZE >> 1;

typedef std::set<EvalObj*> EvalObjSet;
class GarbageCollector;

#define GC_CYC_TRIGGER(ptr) \
    do { \
        if ((ptr) && (ptr)->is_container() &&  \
                !(static_cast<Container*>(ptr)->keep)) \
        { \
            static_cast<Container*>(ptr)->keep = true; \
            *tail++ = (ptr); \
        } \
    } while (0)

#define GC_CYC_DEC(ptr) \
    do { \
        if ((ptr) && (ptr)->is_container()) \
        static_cast<Container*>(ptr)->gc_refs--; \
    } while (0)

extern GarbageCollector gc;
#define EXIT_CURRENT_ENVT(lenvt) \
    do { \
        gc.expose(lenvt); \
        lenvt = cont->envt; \
        gc.attach(lenvt); \
    } while (0)
#define EXIT_CURRENT_CONT(cont) \
    do { \
        gc.expose(cont); \
        cont = cont->prev_cont; \
        gc.attach(cont); \
    } while (0)

#define EXIT_CURRENT_EXEC(lenvt, cont, args) \
    do { \
        EXIT_CURRENT_ENVT(lenvt); \
        EXIT_CURRENT_CONT(cont); \
        gc.expose(args); \
        gc.collect(); \
    } while (0)

/** @class GCRecord 
 * The record object which keeps track of the GC info of corresponding
 * EvalObj
 */
struct GCRecord {
    EvalObj *obj;           /**< The pointer to the EvalObj */
    size_t gc_cnt;          /**< Reference counter */
    GCRecord *prev;         /**< Previous GCRecord in the list */
    GCRecord *next;         /**< Next GCRecord in the the list */
    /** The constructor */
    GCRecord(GCRecord *prev, GCRecord *next);
};

/** @class GarbageCollector
 * Which takes the responsibility of taking care of all existing EvalObj
 * in use as well as recycling those aren't
 */
class GarbageCollector {

    struct PendingEntry {
        EvalObj *obj;
        PendingEntry *next;
        PendingEntry(EvalObj *obj, PendingEntry *next);
    };

    GCRecord *joined;
    PendingEntry *pending_list;
    size_t resolve_threshold;
    size_t joined_size;

    void cycle_resolve();
    void force();

    public:

    GarbageCollector();         /**< The constructor */
    void collect();             /**< To detect and recycle the garbage */
    /**< Call this when a pointer is attached to the EvalObj */
    EvalObj *attach(EvalObj *ptr);
    void expose(EvalObj *ptr);  /**< Call this when a pointer is detached
                                  from the EvalObj */
    /** Call this when an EvalObj first appears */ 
    GCRecord *join(EvalObj *ptr);  
    /** Call this when an EvalObj is destroyed */
    void quit(EvalObj *ptr);

    /** Get the number of EvalObj in use */
    size_t get_remaining();
    /** Set the threshold for cycle_resolve */
    void set_resolve_threshold(size_t new_thres);
};


#endif
