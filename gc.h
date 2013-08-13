#ifndef GC_H
#define GC_H

#include "model.h"
#include <map>

const int GC_QUEUE_SIZE = 262144;
const size_t GC_CYC_THRESHOLD = 1000;

typedef std::map<EvalObj*, size_t> EvalObj2Int;
typedef std::set<EvalObj*> EvalObjSet;

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
    

class GarbageCollector {

    struct PendingEntry {
        EvalObj *obj;
        PendingEntry *next;
        PendingEntry(EvalObj *obj, PendingEntry *next);
    };

    EvalObj2Int mapping;
    PendingEntry *pending_list;

    public:
    GarbageCollector();
    void cycle_resolve();
    void force();
    void expose(EvalObj *ptr);
    EvalObj *attach(EvalObj *ptr);
};

extern GarbageCollector gc;

#endif
