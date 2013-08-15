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
    if ((ptr) && (ptr)->is_container() && !visited.count(ptr)) \
        visited.insert(*tail++ = (ptr)); \
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

class GarbageCollector {

    struct PendingEntry {
        EvalObj *obj;
        PendingEntry *next;
        PendingEntry(EvalObj *obj, PendingEntry *next);
    };

    EvalObjSet joined;
    PendingEntry *pending_list;
    size_t resolve_threshold;

    public:
    GarbageCollector();
    void collect();
    void cycle_resolve();
    void force();
    void expose(EvalObj *ptr);
    void set_resolve_threshold(size_t new_thres);
    void join(EvalObj *ptr);
    void quit(EvalObj *ptr);
    size_t get_remaining();
    EvalObj *attach(EvalObj *ptr);
};


#endif
