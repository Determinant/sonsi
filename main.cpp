#include "model.h"
#include "builtin.h"
#include "parser.h"
#include "eval.h"
#include "exc.h"
#include <cstdio>

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
            Pair *tree = ast->absorb(tk);
            if (!tree) break;
            fprintf(stderr, "Ret> $%d = %s\n", rcnt++,
                    eval->run_expr(tree)->ext_repr().c_str());
        }
        catch (GeneralError &e)
        {
            fprintf(stderr, "An error occured: %s\n", e.get_msg().c_str());
        }
    }
}
