#include <cstdio>
#include <cctype>
#include <sstream>
#include "parser.h"
#include "exc.h"
#include "consts.h"
#include "builtin.h"
#include "gc.h"

using std::stringstream;

static char buff[TOKEN_BUFF_SIZE];
static FrameObj *parse_stack[PARSE_STACK_SIZE];
extern Pair *empty_list;

Tokenizor::Tokenizor() : stream(stdin), buff_ptr(buff), escaping(false) {}
void Tokenizor::set_stream(FILE *_stream) {
    stream = _stream;
}

#define IS_NEWLINE(ch) \
    ((ch) == '\n')
#define IS_QUOTE(ch) \
    ((ch) == '\"')
#define IS_SLASH(ch) \
    ((ch) == '\\')
#define IS_BRACKET(ch) \
    ((ch) == '(' || (ch) == ')')
#define IS_SPACE(ch) \
    ((ch) == ' ' || (ch) == '\t' || IS_NEWLINE(ch))
#define IS_COMMENT(ch) \
    ((ch) == ';')
#define IS_LITERAL(ch) \
    ((ch) == '\'')
#define IS_DELIMITER(ch) \
    (IS_BRACKET(ch) || IS_SPACE(ch) ||  \
     IS_COMMENT(ch) || IS_QUOTE(ch))

#define POP \
    do { \
        *buff_ptr = 0; \
        ret = string(buff); \
        buff_ptr = buff; \
    } while (0)
#define TOP (*(buff_ptr - 1))

string str_to_lower(string str) {
    size_t len = str.length();
    for (size_t i = 0; i < len; i++)
        if ('A' <= str[i] && str[i] <= 'Z')
            str[i] -= 'A' - 'a';
    return str;
}

bool Tokenizor::get_token(string &ret) {
    char ch;
    bool flag = false;
    while (fread(&ch, 1, 1, stream))
    {
        if (escaping)
        {
            escaping = false;
            switch (ch)
            {
                case '\\': *buff_ptr++ = '\\'; break;
                case '\"': *buff_ptr++ = '\"'; break;
                case 'n': *buff_ptr++ = '\n'; break;
                case 't': *buff_ptr++ = '\t'; break;
                default: {
                             buff_ptr = buff;
                             throw TokenError(string("") + ch,
                                     PAR_ERR_ILLEGAL_CHAR_IN_ESC);
                         }
            }
        }
        else
        {
            bool in_quote = buff_ptr != buff && IS_QUOTE(*buff);
            if (buff_ptr != buff)
            {
                if (buff_ptr - buff == 1 && IS_LITERAL(TOP))
                {
                    POP;
                    flag = true;
                }
                else if ((IS_BRACKET(TOP) || IS_DELIMITER(ch)))
                {
                    if (IS_COMMENT(*buff))
                    {
                        if (IS_NEWLINE(ch)) buff_ptr = buff;
                        else buff_ptr = buff + 1;
                    }
                    else if (!in_quote) // not in a double-quote
                    {
                        if (!(buff_ptr - buff == 1 && ch == '(' && TOP == '#'))
                        {
                            POP;
                            flag = true;
                        }
                    }
                    else if (IS_QUOTE(ch))
                    {
                        // in a double-quote which is being enclosed
                        *buff_ptr++ = '\"';
                        POP;
                        return true;    // prevent duplicate quote sign
                    }
                }
            }
            if (in_quote || !IS_SPACE(ch))
            {
                if (in_quote && IS_SLASH(ch))
                    escaping = true;
                else
                    *buff_ptr++ = ch;
            }
            if (flag) return true;
        }
    }
    if (buff_ptr != buff) POP;
    return false; // can't read more
}

ASTGenerator::ASTGenerator() {}


EvalObj *ASTGenerator::to_obj(const string &str) {
    EvalObj *res = NULL;
    if ((res = BoolObj::from_string(str))) return res;
    if ((res = CharObj::from_string(str))) return res;
    if ((res = IntNumObj::from_string(str))) return res;
    if ((res = RatNumObj::from_string(str))) return res;
    if ((res = RealNumObj::from_string(str))) return res;
    if ((res = CompNumObj::from_string(str))) return res;
    if ((res = StrObj::from_string(str))) return res;
    return new SymObj(str_to_lower(str)); // otherwise we assume it a symbol
}

#define TO_EVAL(ptr) \
    (static_cast<EvalObj*>(ptr))
#define TO_BRACKET(ptr) \
    (static_cast<ParseBracket*>(ptr))
#define IS_BRAKET(ptr) \
    ((ptr)->is_parse_bracket())

Pair *ASTGenerator::absorb(Tokenizor *tk) {
    FrameObj **top_ptr = parse_stack;
    for (;;)
    {
        if (top_ptr == parse_stack + PARSE_STACK_SIZE)
            throw TokenError("Parser", RUN_ERR_STACK_OVERFLOW);

        if (top_ptr - parse_stack > 1 &&
                !IS_BRAKET(*(top_ptr - 1)) &&
                IS_BRAKET(*(top_ptr - 2)))
        {
            ParseBracket *bptr = TO_BRACKET(*(top_ptr - 2));
            if (bptr->btype == 2)
            {
                top_ptr -= 2;
                Pair *lst_cdr = new Pair(TO_EVAL(*(top_ptr + 1)), empty_list);
                Pair *lst = new Pair(new SymObj("quote"), lst_cdr);
                *top_ptr++ = lst;
            }
        }

        if (top_ptr > parse_stack && !IS_BRAKET(*parse_stack))
            return new Pair(TO_EVAL(*(top_ptr - 1)), empty_list);
        string token;
        if (!tk->get_token(token)) return NULL;
        if (token == "(")       // a list
            *top_ptr++ = new ParseBracket(0);
        else if (token == "#(") // a vector
            *top_ptr++ = new ParseBracket(1);
        else if (token == "\'") // syntatic sugar for quote
            *top_ptr++ = new ParseBracket(2);
        else if (token == ")")
        {
            if (top_ptr == parse_stack)
                throw NormalError(READ_ERR_UNEXPECTED_RIGHT_BRACKET);
            EvalObj *lst = empty_list;
            bool improper = false;
            while (top_ptr >= parse_stack && !IS_BRAKET(*(--top_ptr)))
            {
                EvalObj *obj = TO_EVAL(*top_ptr);
                if (obj->is_sym_obj() && static_cast<SymObj*>(obj)->val == ".")
                {
                    if (improper ||
                        lst == empty_list ||
                        TO_PAIR(lst)->cdr != empty_list)
                        throw NormalError(PAR_ERR_IMPROPER_PAIR);
                    improper = true;
                    lst = TO_PAIR(lst)->car;
                }
                else
                {
                    Pair *_lst = new Pair(obj, lst);    // Collect the list
                    lst = _lst;
                }
            }

            ParseBracket *bptr = TO_BRACKET(*top_ptr);
            if (bptr->btype == 0)
                *top_ptr++ = lst;
            else if (bptr->btype == 1)
            {
                if (improper) throw NormalError(PAR_ERR_IMPROPER_VECT);
                VecObj *vec = new VecObj();
                for (Pair *ptr = TO_PAIR(lst); ptr != empty_list; ptr = TO_PAIR(ptr->cdr))
                    vec->push_back(ptr->car);
                *top_ptr++ = vec;
                if (lst != empty_list) delete lst;
            }
        }
        else
            *top_ptr++ = ASTGenerator::to_obj(token);
    }
}
