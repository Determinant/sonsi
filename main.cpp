#include "model.h"
#include "builtin.h"
#include "parser.h"
#include "eval.h"
#include "exc.h"
#include <cstdio>

#ifdef DEBUG
extern Cons *empty_list;
void tree_print(Cons *ptr) {
    ptr->_debug_print();
    if (ptr->car->is_cons_obj())
        tree_print(TO_CONS(ptr->car));
    if (ptr->cdr->is_cons_obj())
        tree_print(TO_CONS(ptr->cdr));
}
#endif

int main() {
    //freopen("in.scm", "r", stdin);
    Tokenizor *tk = new Tokenizor();
    ASTGenerator *ast = new ASTGenerator();
    Evaluator *eval = new Evaluator();

    int rcnt = 0;
    while (1)
    {
        fprintf(stderr, "Sonsi> ");
        try
        {
            Cons *tree = ast->absorb(tk);
            if (!tree) break;
            //tree_print(tree);
            fprintf(stderr, "Ret> $%d = %s\n", rcnt++, 
                    eval->run_expr(tree)->ext_repr().c_str());
        }
        catch (GeneralError &e)
        {
            fprintf(stderr, "An error occured: %s\n", e.get_msg().c_str());
        }
    }
}
