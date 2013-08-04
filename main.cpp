#include "model.h"
#include "builtin.h"
#include "parser.h"
#include "eval.h"
#include "exc.h"
#include <cstdio>

#ifdef DEBUG
extern Cons *empty_list;
void tree_print(Cons *ptr) {
    if (!ptr || ptr == empty_list) return;
    ptr->_debug_print();
    tree_print(dynamic_cast<Cons*>(ptr->car));
    tree_print(ptr->cdr);
}
#endif

int main() {
    //freopen("in", "r", stdin);
    Tokenizor *tk = new Tokenizor();
    ASTGenerator *ast = new ASTGenerator();
    Evaluator *eval = new Evaluator();

    while (1)
    {
        Cons *tree = ast->absorb(tk);
        if (!tree) break;
        //tree_print(tree);
        try
        {
            printf("%s\n", eval->run_expr(tree)->ext_repr().c_str());
        }
        catch (GeneralError &e)
        {
            printf("An error occured: %s\n", e.get_msg().c_str());
        }
    }
}
