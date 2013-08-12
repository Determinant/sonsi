#include "model.h"
#include "builtin.h"
#include "parser.h"
#include "eval.h"
#include "exc.h"
#include <cstdio>

int main(int argc, char **argv) {
    //freopen("in.scm", "r", stdin);
    Tokenizor *tk = new Tokenizor();
    ASTGenerator *ast = new ASTGenerator();
    Evaluator *eval = new Evaluator();

    bool interactive = false;
    bool preload = false;
    char *fname;

    int rcnt = 0;
    for (int i = 1; i < argc; i++)
        if (strcmp(argv[i], "-i") == 0)
            interactive = true;
        else if (strcmp(argv[i], "-l") == 0 && i < argc - 1)
        {
            preload = true;
            fname = argv[i + 1];
        }

    if (preload)
    {
        FILE *f = fopen(fname, "r");
        if (!f)
        {
            printf("Can not open file: %s\n", fname);
            return 0;
        }
        tk->set_stream(f);
        while (1)
        {
            try
            {
                Pair *tree = ast->absorb(tk);
                if (!tree) break;
                eval->run_expr(tree)->ext_repr().c_str();
            }
            catch (GeneralError &e)
            {
                fprintf(stderr, "An error occured: %s\n", e.get_msg().c_str());
            }
        }
        interactive = true;
    }
    tk->set_stream(stdin); 
    while (1)
    {
        if (interactive)
        fprintf(stderr, "Sonsi> ");
        try
        {
            Pair *tree = ast->absorb(tk);
            if (!tree) break;
            string output = eval->run_expr(tree)->ext_repr();
            if (interactive)
                fprintf(stderr, "Ret> $%d = %s\n", rcnt++, output.c_str());
        }
        catch (GeneralError &e)
        {
            fprintf(stderr, "An error occured: %s\n", e.get_msg().c_str());
        }
    }
}
