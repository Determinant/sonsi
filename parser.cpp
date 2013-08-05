#include <cstdio>
#include <cctype>
#include <sstream>
#include "parser.h"
#include "exc.h"
#include "consts.h"
#include "builtin.h"

using std::stringstream;

static char buff[TOKEN_BUFF_SIZE];
static EvalObj *parse_stack[PARSE_STACK_SIZE];
extern Cons *empty_list;

Tokenizor::Tokenizor() : stream(stdin), buff_ptr(buff), escaping(false) {}
void Tokenizor::set_stream(FILE *_stream) {
    stream = _stream;
}

#define IS_NEWLINE(ch) \
    ((ch) == '\n')
#define IS_BRACKET(ch) \
    ((ch) == '(' || (ch) == ')') 
#define IS_SPACE(ch) \
    ((ch) == ' ' || (ch) == '\t' || IS_NEWLINE(ch))
#define IS_DELIMITER(ch) \
    (IS_BRACKET(ch) || IS_SPACE(ch))
#define IS_COMMENT(ch) \
    ((ch) == ';')
#define IS_QUOTE(ch) \
    ((ch) == '\"')
#define IS_SLASH(ch) \
    ((ch) == '\\')

#define POP \
    do { \
        *buff_ptr = 0; \
        ret = string(buff); \
        buff_ptr = buff; \
    } while (0)

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
            if (buff_ptr != buff && 
                    (IS_BRACKET(*buff) || 
                     IS_DELIMITER(ch) ||
                     IS_COMMENT(ch) ||
                     IS_QUOTE(ch)))
            {
                if (IS_COMMENT(*buff))
                {
                    if (IS_NEWLINE(ch)) buff_ptr = buff;
                    else buff_ptr = buff + 1;
                }
                else if (!in_quote)
                {
                    POP;
                    flag = true;
                }
                else if (IS_QUOTE(ch))
                {
                    *buff_ptr++ = '\"';
                    POP;
                    return true;    // discard current slash
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
    if ((res = IntNumObj::from_string(str))) return res;
    if ((res = RatNumObj::from_string(str))) return res;
    if ((res = RealNumObj::from_string(str))) return res;
    if ((res = CompNumObj::from_string(str))) return res;
    if ((res = StrObj::from_string(str))) return res;
    return new SymObj(str);
}
Cons *ASTGenerator::absorb(Tokenizor *tk) {
    EvalObj **top_ptr = parse_stack;
    for (;;)
    {
        if (top_ptr > parse_stack && *parse_stack)
            return new Cons(*(top_ptr - 1), empty_list);
        string token;
        if (!tk->get_token(token)) return NULL;
        if (token == "(")
            *top_ptr++ = NULL;  // Make the beginning of a new level
        else if (token == ")")
        {
            Cons *lst = empty_list;
            while (top_ptr >= parse_stack && *(--top_ptr))
            {
                Cons *_lst = new Cons(*top_ptr, lst); // Collect the list
                _lst->next = lst == empty_list ? NULL : lst;
                lst = _lst;
            }
            if (top_ptr < parse_stack)
                throw NormalError(READ_ERR_UNEXPECTED_RIGHT_BRACKET);
            *top_ptr++ = lst;
        }
        else *top_ptr++ = ASTGenerator::to_obj(token);
    }
}
