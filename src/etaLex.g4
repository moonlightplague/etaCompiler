lexer grammar etaLex;

@lexer::members {
public:
    bool isRho = false;
}

//--------------------------------------------
//keywords, highest precedence in lexing
//--------------------------------------------
BREAK : {isRho}? 'break';
RECORD : {isRho}? 'record';
NULL_V : {isRho}? 'null';
USE : 'use';
IF : 'if';
WHILE : 'while';
ELSE : 'else';
RETURN : 'return';
LENGTH : 'length';
INT_T : 'int';
BOOL_T : 'bool';
TRUE : 'true';
FALSE : 'false';

//----------------------------------------
//symbols (punctuations and operators)
//----------------------------------------

//punctuation
DOT : '.';
LPAREN : '(';
RPAREN : ')';
COLON : ':';
SEMICOLON : ';';
COMMA : ',';
LBRACE : '{';
RBRACE : '}';
LBRACKET : '[';
RBRACKET : ']' ;
UNDERLINE : '_';

//arithmetic operators
HIGH_MULT : '*>>';
PLUS : '+';
MINUS : '-';
DIV : '/';
MULT : '*';
MOD : '%';
AND : '&';
OR : '|';
EQUAL : '==';
NOT_EQUAL : '!=';
LESS_THAN_OR_EQ : '<=';
GREATER_THAN_OR_EQ : '>=';
LESS_THAN : '<';
GREATER_THAN : '>';
BANG : '!';

//assignment
ASSIGN : '=';

//----------------------------------------
//positive integers (NUANCE: negative integer
// is not allowed for later ease of parsing).
//----------------------------------------
INTEGER : [1-9] [0-9]*
        | '0';

//----------------------------------------
//characters and strings (NUANCE: escapes using
// characters that are not r, n, or ' are not allowed. )
//----------------------------------------
STRING : '"' (ESCAPE | ~[\\\r\n\u0022])* '"';

CHARACTER : '\'' (ESCAPE | ~[\\\r\n\u0027]) '\'';

fragment ESCAPE :
          '\\\\'
        | '\\n'
        | '\\\''
        | UNICODE_ESCAPE;

fragment UNICODE_ESCAPE : '\\x{' HEX_DIGIT HEX_DIGIT? HEX_DIGIT? HEX_DIGIT? HEX_DIGIT? HEX_DIGIT? '}';

fragment HEX_DIGIT : [0-9a-fA-F];

//----------------------------------------
//identifiers
//----------------------------------------
ID : [a-zA-Z] [0-9a-zA-Z_']*;

//----------------------------------------
//comments and whitespaces (skipped)
//----------------------------------------
LINE_COMMENT
    : '//' ~[\r\n]* -> channel(HIDDEN);

WS
    : [ \t\r\n\f]+ -> channel(HIDDEN);