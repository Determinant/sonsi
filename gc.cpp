#include "gc.h"
#include "exc.h"
#include "consts.h"
#include <vector>

#if defined(GC_DEBUG) || defined (GC_INFO)
#include <cstdio>
typedef unsigned long long ull;
#endif

static EvalObj *gcq[GC_QUEUE_SIZE];
static Container *cyc_list[GC_QUEUE_SIZE];

GarbageCollector::GarbageCollector() {
    mapping.clear();
    pending_list = NULL;
    resolve_threshold = GC_CYC_THRESHOLD;
}

GarbageCollector::PendingEntry::PendingEntry(
        EvalObj *_obj, PendingEntry *_next) : obj(_obj), next(_next) {}


void GarbageCollector::expose(EvalObj *ptr) {
    bool flag = mapping.count(ptr);
    if (flag)
    {
#ifdef GC_DEBUG
        fprintf(stderr, "GC: 0x%llx exposed. count = %lu \"%s\"\n", 
            (ull)ptr, mapping[ptr] - 1, ptr->ext_repr().c_str());
#endif
        if (!--mapping[ptr])
        {
#ifdef GC_DEBUG
            fprintf(stderr, "GC: 0x%llx pending. \n", (ull)ptr);
#endif
            pending_list = new PendingEntry(ptr, pending_list);
        } 
    }
}

void GarbageCollector::force() {
    EvalObj **l = gcq, **r = l;
    for (PendingEntry *p = pending_list, *np; p; p = np)
    {
        np = p->next;
        if (mapping.count(p->obj) && mapping[p->obj] == 0)
            *r++ = p->obj;
        delete p;
    }   // fetch the pending pointers in the list
    // clear the list
    pending_list = NULL; 
/*    for (EvalObj2Int::iterator it = mapping.begin(); 
            it != mapping.end(); it++)
        if (it->second == 0) *r++ = it->first;*/

#ifdef GC_INFO
    fprintf(stderr, "%ld\n", mapping.size());
    size_t cnt = 0;
#endif
#ifdef GC_DEBUG
    fprintf(stderr, 
            "================================\n"
            "GC: Forcing the clear process...\n");
#endif
    for (; l != r; l++)
    {
#ifdef GC_DEBUG
        fprintf(stderr, "GC: !!! destroying space 0x%llx: %s. \n", (ull)*l, (*l)->ext_repr().c_str());
#endif
#ifdef GC_INFO
        cnt++;
#endif
        delete *l;
        mapping.erase(*l);
        // maybe it's a complex structure, 
        // so that more pointers are reported
        for (PendingEntry *p = pending_list, *np; p; p = np)
        {
            np = p->next;
            *r++ = p->obj;
            if (r == gcq + GC_QUEUE_SIZE)
                throw NormalError(RUN_ERR_GC_OVERFLOW);
            delete p;
        }   
        pending_list = NULL;
    }
#ifdef GC_INFO
    fprintf(stderr, "GC: Forced clear, %lu objects are freed, "
            "%lu remains\n"
            "=============================\n", cnt, mapping.size());
        
#endif
#ifdef GC_DEBUG
/*    for (EvalObj2Int::iterator it = mapping.begin();
            it != mapping.end(); it++)
        fprintf(stderr, "%llx => %s\n", (ull)it->first, it->first->ext_repr().c_str());
        */
#endif
}

EvalObj *GarbageCollector::attach(EvalObj *ptr) {
    if (!ptr) return NULL;   // NULL pointer
    bool flag = mapping.count(ptr);
    if (flag) mapping[ptr]++;
    else mapping[ptr] = 1;
#ifdef GC_DEBUG
    fprintf(stderr, "GC: 0x%llx attached. count = %lu \"%s\"\n", 
            (ull)ptr, mapping[ptr], ptr->ext_repr().c_str());
#endif
    if (mapping.size() > GC_QUEUE_SIZE >> 1)
        force();
    return ptr; // passing through
}

void GarbageCollector::cycle_resolve() {
    if (mapping.size() < resolve_threshold)
        return; 
    EvalObjSet visited;
    Container **clptr = cyc_list;
    for (EvalObj2Int::iterator it = mapping.begin();
            it != mapping.end(); it++)
        if (it->first->is_container())
        {
            Container *p = static_cast<Container*>(it->first);
            (*clptr++ = p)->gc_refs = it->second;   // init the count
            p->keep = false;
        }

    EvalObj **l = gcq, **r = l;
    for (Container **p = cyc_list; p < clptr; p++)
        (*p)->gc_decrement();
    for (Container **p = cyc_list; p < clptr; p++)
        if ((*p)->gc_refs) 
        {
            *r++ = *p;
            visited.insert(*p);
        }
    for (; l != r; l++)
    {
        Container *p = static_cast<Container*>(*l);
        p->keep = true;        
        p->gc_trigger(r, visited);
    }
    for (Container **p = cyc_list; p < clptr; p++)
        if (!(*p)->keep) 
        {
            delete *p;
            mapping.erase(*p);
        }
#ifdef GC_INFO
    fprintf(stderr, "GC: cycle resolved.\n");
#endif
    force();
}

size_t GarbageCollector::get_remaining() {
    return mapping.size();
}

void GarbageCollector::set_resolve_threshold(size_t new_thres) {
    resolve_threshold = new_thres;
}
