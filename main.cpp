#include "model.h"
#include "builtin.h"
#include "parser.h"
#include "eval.h"
#include "exc.h"
#include "gc.h"

#include <cstdio>
#include <cstdlib>

GarbageCollector gc;
Tokenizor tk;
ASTGenerator ast;
Evaluator eval;

void load_file(const char *fname) {
    FILE *f = fopen(fname, "r");
    if (!f)
    {
        printf("Can not open file: %s\n", fname);
        exit(0);
    }
    tk.set_stream(f);
    while (1)
    {
        try
        {
            Pair *tree = ast.absorb(&tk);
            if (!tree) break;
            eval.run_expr(tree);
        }
        catch (GeneralError &e)
        {
            fprintf(stderr, "An error occured: %s\n", e.get_msg().c_str());
        }
    }
}

void print_help(const char *cmd) {
    fprintf(stderr, 
            "Sonsi: Stupid and Obvious Scheme Interpreter\n"
            "Usage: %s OPTION ...\n"
            "Evaluate Scheme code, interactively or from a script.\n\n"
            "  FILE \t\tload Scheme source code from FILE, and exit\n"
            "The above switches stop argument processing\n\n"
            "  -l FILE \tload Scheme source code from FILE\n"
            "  -h, --help \tdisplay this help and exit\n", cmd);
    exit(0);
}

int main(int argc, char **argv) {

    for (int i = 1; i < argc; i++)
    {
        if (*argv[i] == '-')    // parsing options
        {
            if (strcmp(argv[i], "-l") == 0)
            {
                if (i + 1 < argc)
                    load_file(argv[++i]);
                else 
                {
                    puts("missing argument to `-l` switch");
                    print_help(*argv);
                }
            }
            else if (strcmp(argv[i], "-h") == 0 ||
                     strcmp(argv[i], "--help") == 0)
                print_help(*argv);
            else
            {
                printf("unrecognized switch `%s`\n", argv[i]);
                print_help(*argv);
            }
        }
        else
        {
            load_file(argv[i]); 
            exit(0);
        }
    }

    int rcnt = 0;
    tk.set_stream(stdin);  // interactive mode
    while (1)
    {
        fprintf(stderr, "Sonsi> ");
        try
        {
            Pair *tree = ast.absorb(&tk);
            if (!tree) break;
            string output = eval.run_expr(tree)->ext_repr();
            fprintf(stderr, "Ret> $%d = %s\n", rcnt++, output.c_str());
        }
        catch (GeneralError &e)
        {
            fprintf(stderr, "An error occured: %s\n", e.get_msg().c_str());
        }
    }
}
