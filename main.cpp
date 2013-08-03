#include "model.h"
#include "builtin.h"
#include "parser.h"
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
    Tokenizor *tk = new Tokenizor();
    ASTGenerator *ast = new ASTGenerator();
    Cons *tree = ast->absorb(tk);
    tree_print(tree);
}
