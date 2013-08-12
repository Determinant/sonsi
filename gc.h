#ifndef GC_H
#define GC_H

#include "model.h"
#include <map>

const int GC_QUEUE_SIZE = 262144;

typedef std::map<EvalObj*, size_t> EvalObj2Int;

class GarbageCollector {

    struct PendingEntry {
        EvalObj *obj;
        PendingEntry *next;
        PendingEntry(EvalObj *obj, PendingEntry *next);
    };

    EvalObj2Int mapping;
    size_t pend_cnt;
    PendingEntry *pending_list;

    public:
    void force();
    void expose(EvalObj *ptr);
    EvalObj *attach(EvalObj *ptr);
};

extern GarbageCollector gc;

#endif
