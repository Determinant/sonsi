#include "gc.h"
#include "exc.h"
#include "consts.h"
#include <vector>

#include <cstdio>
#if defined(GC_DEBUG) || defined (GC_INFO)
typedef unsigned long long ull;
#endif

static EvalObj *gcq[GC_QUEUE_SIZE];
static Container *cyc_list[GC_QUEUE_SIZE];
ObjEntry *oe_null;

GarbageCollector::GarbageCollector() {
    joined = new ObjEntry(NULL, NULL);
    joined->next = oe_null = new ObjEntry(NULL, NULL);
    joined_size = 0;
    pending_list = NULL;
    resolve_threshold = GC_CYC_THRESHOLD;
}

GarbageCollector::PendingEntry::PendingEntry(
        EvalObj *_obj, PendingEntry *_next) : obj(_obj), next(_next) {}


void GarbageCollector::expose(EvalObj *ptr) {
    if (ptr == NULL || ptr->gc_obj == NULL) return;
#ifdef GC_DEBUG
    fprintf(stderr, "GC: 0x%llx exposed. count = %lu \"%s\"\n", 
            (ull)ptr, ptr->gc_obj->gc_cnt - 1, ptr->ext_repr().c_str());
#endif
    if (--ptr->gc_obj->gc_cnt == 0)
    {
#ifdef GC_DEBUG
        fprintf(stderr, "GC: 0x%llx pending. \n", (ull)ptr);
#endif
        pending_list = new PendingEntry(ptr, pending_list);
    } 
}

void GarbageCollector::force() {
    EvalObj **l = gcq, **r = l;
    for (PendingEntry *p = pending_list, *np; p; p = np)
    {
        np = p->next;
        EvalObj *obj = p->obj;
        if (obj->gc_obj && !obj->gc_obj->gc_cnt)
            *r++ = obj;
        delete p;
    }   // fetch the pending pointers in the list
    // clear the list
    pending_list = NULL; 

#ifdef GC_INFO
    fprintf(stderr, "%ld\n", joined_size);
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
        fprintf(stderr, "GC: !!! destroying space 0x%llx: %s. \n", 
                (ull)*l, (*l)->ext_repr().c_str());
#endif
#ifdef GC_INFO
        cnt++;
#endif
        delete *l;
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
            "=============================\n", cnt, joined_size);
        
#endif
#ifdef GC_DEBUG
#endif
}

EvalObj *GarbageCollector::attach(EvalObj *ptr) {
    if (!ptr) return NULL;   // NULL pointer
/*    bool flag = mapping.count(ptr);
    if (flag) mapping[ptr]++;
    else mapping[ptr] = 1;
    */
    ptr->gc_obj->gc_cnt++;
#ifdef GC_DEBUG
    fprintf(stderr, "GC: 0x%llx attached. count = %lu \"%s\"\n", 
            (ull)ptr, ptr->gc_obj->gc_cnt, ptr->ext_repr().c_str());
#endif
/*    if (mapping.size() > GC_QUEUE_SIZE >> 1)
        force();*/
    return ptr; // passing through
}

void GarbageCollector::cycle_resolve() {
    EvalObjSet visited;
    Container **clptr = cyc_list;
    for (ObjEntry *i = joined->next; i != oe_null; i = i->next)
    {
        EvalObj *obj = i->obj;
        if (obj->is_container())
        {
            Container *p = static_cast<Container*>(obj);
            (*clptr++ = p)->gc_refs = i->gc_cnt;   // init the count
            p->keep = false;
        }
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
        }
#ifdef GC_INFO
    fprintf(stderr, "GC: cycle resolved.\n");
#endif
}

void GarbageCollector::collect() {
    force();
//    if (joined_size >= resolve_threshold) 
    {
        cycle_resolve();
        force();
    }
}

size_t GarbageCollector::get_remaining() {
    return joined_size;
}

void GarbageCollector::set_resolve_threshold(size_t new_thres) {
    resolve_threshold = new_thres;
}

ObjEntry *GarbageCollector::join(EvalObj *ptr) {
    ObjEntry *p = new ObjEntry(joined, joined->next);
    p->prev->next = p;
    p->next->prev = p;
    p->gc_cnt = 0;
    p->obj = ptr;
    joined_size++;
    return p;
}

void GarbageCollector::quit(EvalObj *ptr) {
    ObjEntry *p = ptr->gc_obj;
    p->prev->next = p->next;
    p->next->prev = p->prev;
    ptr->gc_obj = NULL;
    delete p;
    joined_size--;
}

ObjEntry::ObjEntry(ObjEntry *_prev, ObjEntry *_next) :
   prev(_prev), next(_next) {} 
