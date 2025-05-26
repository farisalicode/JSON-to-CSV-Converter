%{
#include <string>
#include <cstdlib>
#include "ast.hpp"
#include "parser.tab.h"



// Removed extern "C" block to match parser's declaration
void yyerror(const char *);

std::string string_buffer;
%}

%option noyywrap
%option yylineno

DIGIT    [0-9]
INT      {DIGIT}+
EXP      [eE][+-]?{INT}
FRAC     "."{INT}
NUMBER   -?{INT}{FRAC}?{EXP}?

%x STRING_STATE

%%

[ \t\r\n]          { /* Skip whitespace */ }

"{"               { return '{'; }
"}"               { return '}'; }
"["               { return '['; }
"]"               { return ']'; }
":"               { return ':'; }
","               { return ','; }

"true"            { yylval.bool_val = true; return BOOL; }
"false"           { yylval.bool_val = false; return BOOL; }
"null"            { return NUL; }

{NUMBER}          { yylval.num_val = atof(yytext); return NUMBER; }

\"                { string_buffer.clear(); BEGIN(STRING_STATE); }
<STRING_STATE>{
    \"            { BEGIN(INITIAL); yylval.str_val = strdup(string_buffer.c_str()); return STRING; }
    \\\"          { string_buffer += '\"'; }
    \\\\          { string_buffer += '\\'; }
    \\b           { string_buffer += '\b'; }
    \\f           { string_buffer += '\f'; }
    \\n           { string_buffer += '\n'; }
    \\r           { string_buffer += '\r'; }
    \\t           { string_buffer += '\t'; }
    \\u[0-9A-Fa-f]{4} { 
        std::string hex = yytext + 2;
        int codepoint = std::stoi(hex, nullptr, 16);
        if (codepoint <= 0x7F) {
            string_buffer += static_cast<char>(codepoint);
        } else if (codepoint <= 0x7FF) {
            string_buffer += static_cast<char>(0xC0 | (codepoint >> 6));
            string_buffer += static_cast<char>(0x80 | (codepoint & 0x3F));
        } else {
            string_buffer += static_cast<char>(0xE0 | (codepoint >> 12));
            string_buffer += static_cast<char>(0x80 | ((codepoint >> 6) & 0x3F));
            string_buffer += static_cast<char>(0x80 | (codepoint & 0x3F));
        }
    }
    [^\\\"\n]+    { string_buffer += yytext; }
    .             { string_buffer += yytext; }
}

.                 { yyerror("Invalid character"); return ERROR; }

%%

