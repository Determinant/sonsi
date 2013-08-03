#ifndef PARSER_H
#define PARSER_H

#include "model.h"
#include <string>

using std::string;

const int TOKEN_BUFF_SIZE = 65536;
const int PARSE_STACK_SIZE = 65536;

class Tokenizor {
    private:
        FILE *stream;
        char *buff_ptr;
    public:
        Tokenizor();
        void set_stream(FILE *stream);
        bool get_token(string &ret);
};

class ASTGenerator {
    private:
        static EvalObj* to_float(const string &);
        static EvalObj* to_int(const string &);
        static EvalObj* to_obj(const string &);
    public:    
        ASTGenerator();
        Cons *absorb(Tokenizor *tk); 
};

#endif
