#ifndef EVAL_H
#define EVAL_H
#include "model.h"

class Evaluator {
    private:
        Environment *envt;
        void add_builtin_routines();
    public:
        Evaluator();
        EvalObj *run_expr(Cons *prog);
};

#endif
