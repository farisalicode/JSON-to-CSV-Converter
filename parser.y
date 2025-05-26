%{
#include <iostream>
#include <string>
#include <vector>
#include <map>
#include <cstdlib>
#include <cstring>
#include "ast.hpp"



// Changed the declaration to match how it will be defined in the generated lexer
extern int yylex(void);
extern void yyerror(const char *);

ASTNode* root = NULL;
%}

%union {
    double num_val;
    bool bool_val;
    char* str_val;
    struct ASTNode* node;
    struct {
        char* key;
        struct ASTNode* value;
    } pair;
}

%token <num_val> NUMBER
%token <bool_val> BOOL
%token <str_val> STRING
%token NUL
%token ERROR

%type <node> json value object array
%type <pair> pair
%type <node> pair_list value_list

%%

json
    : value { root = $1; }
    ;

value
    : object { $$ = $1; }
    | array { $$ = $1; }
    | STRING { 
        $$ = new ASTNode(NODE_STRING); 
        $$->str_val = $1;
    }
    | NUMBER { 
        $$ = new ASTNode(NODE_NUMBER); 
        $$->num_val = $1;
    }
    | BOOL { 
        $$ = new ASTNode(NODE_BOOL); 
        $$->bool_val = $1;
    }
    | NUL { 
        $$ = new ASTNode(NODE_NULL); 
    }
    ;

object
    : '{' '}' { 
        $$ = new ASTNode(NODE_OBJECT); 
    }
    | '{' pair_list '}' { $$ = $2; }
    ;

pair_list
    : pair { 
        $$ = new ASTNode(NODE_OBJECT); 
        (*($$->object_val))[$1.key] = $1.value;
        free($1.key);
    }
    | pair_list ',' pair { 
        $$ = $1;
        (*($$->object_val))[$3.key] = $3.value;
        free($3.key);
    }
    ;

pair
    : STRING ':' value { 
        $$.key = $1;
        $$.value = $3;
    }
    ;

array
    : '[' ']' { 
        $$ = new ASTNode(NODE_ARRAY); 
    }
    | '[' value_list ']' { $$ = $2; }
    ;

value_list
    : value { 
        $$ = new ASTNode(NODE_ARRAY); 
        $$->array_val->push_back($1);
    }
    | value_list ',' value { 
        $$ = $1;
        $$->array_val->push_back($3);
    }
    ;

%%

void yyerror(const char *s) {
    std::cerr << "Error: " << s << std::endl;
}

