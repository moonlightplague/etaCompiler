////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// Grammar for the eta language. Properties of the grammar that needs to be
// additionally checked by the parse tree visitor are marked by "CHECK:" in
// front of the comments.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
parser grammar etaParse;

options {
    tokenVocab = './generated/etaLex';
}

@parser::members {
public:
    bool isRho = false;
}


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// toplevel rule of an eta program file and eta interface file (with the extension .eta).
// a program consists of a list of use clauses and definitions of methods or declaration of global variables,
// optionally seperated by semicolons.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
program :
    parsingUnit
    ;

interface:
    parsingUnit
    ;

rhoInterface:
    parsingUnit
    ;

parsingUnit :
    (useClauses+=useClause SEMICOLON?)* (definitions+=definition SEMICOLON?)+ EOF
    ;

useClause :
    USE ID;

definition:
    methodDefinition
    | methodDeclaration
    | globalDeclaration
    | recordDefinition
    ;
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// a definition of a record (only available in Rho)
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
recordDefinition:
    {isRho}? RECORD ID LBRACE (declarationLists+=sameTypeDeclarationList)* RBRACE
    ;

sameTypeDeclarationList:
    {isRho}? items+=ID (COMMA items+=ID)* COLON typeSpecifierInAssignmentOrMethod
    ;
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// a declaration of global variables which can sometimes be followed by an initialization expression.
// CHECK: if an array is declared in declaration, then declaration cannot be followed by an initialization expression.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
globalDeclaration:
    ID COLON typeSpecifierInAssignmentOrMethod (ASSIGN initialized=globalConstant)?           #etaGlobalDeclaration
    | sameTypeDeclarationList                                                                 #rhoGlobalDeclaration
    ;

globalConstant :
    MINUS INTEGER                                               #globalNegativeInteger
    | INTEGER                                                   #globalPositiveInteger
    | CHARACTER                                                 #globalCharacter
    | TRUE                                                      #globalTrue
    | FALSE                                                     #globalFalse;
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// a definition of a method, similar to a function in C. The body of a method is a list of statements, optionally
// seperated by semicolons. A method can have a comma-seperated list of return types.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
methodDefinition :
    methodDeclaration blockStatement;

methodDeclaration:
    ID LPAREN methodParamList RPAREN (COLON returnTypes=nonemptyMethodTypeList)?;

methodParamList :
    //empty
    | (paramNames+=ID COLON paramTypes+=typeSpecifierInAssignmentOrMethod)
        (COMMA paramNames+=ID COLON paramTypes+=typeSpecifierInAssignmentOrMethod)*;

nonemptyMethodTypeList :
    items+=typeSpecifierInAssignmentOrMethod (COMMA items+=typeSpecifierInAssignmentOrMethod)*;

// a type specifier of a parameter or the return type for a method. If the type specified is an array, then
// array indexes cannot appear in the specifier.
typeSpecifierInAssignmentOrMethod :
    baseType (LBRACKET brackets+=RBRACKET)*;

baseType:
    INT_T                               #intBaseType
    | BOOL_T                            #boolBaseType
    | recordTypeSpecifier               #recordBaseType
    ;

recordTypeSpecifier:
    {isRho}? ID
    ;
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// a statement (i.e. command in the eta language), mostly C-style.
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
statement :
    blockStatement
    | ifStatement
    | whileStatement
    | procedureCallStatement
    | assignmentStatement
    | declarationStatement
    | breakStatement
    ;

breakStatement:
    {isRho}? BREAK
    ;

blockStatement :
    LBRACE statementList (returnClause=returnStatement SEMICOLON?)? RBRACE;

//The optional return clause can only appear as the last statement in its block.
returnStatement :
    RETURN expressionList;

statementList :
    //empty
    | items+=statement SEMICOLON? ( items+=statement SEMICOLON?)*;

ifStatement :
    IF expression trueBranch=statement (ELSE falseBranch=statement)? ;

whileStatement :
    WHILE expression statement;

procedureCallStatement :
    ID LPAREN expressionList RPAREN;

// An assignment statement that allows multiple symbols (i.e. lvalues) to be assigned values at the same time.
// Example: y, x = x, y.
// 1. In an assignment, some of the symbols can also be declared. Example: x : int, y = 10, z.
// 2. If an array is declared and assigned in the same assignment statement, then the type specifier must not include index
// expressions. This is legal: d: int[][] = {{1, 0}, {0, 1}}; but this is not: err1:int[3]={1,2,3}.
// 3. If an array cell is assigned to, then the brackets must be indexed. Example: d[0][0] = 1;
// 4. A wildcard is an lvalue. Example: _ = calculateSomething()
// CHECK: according to page 5 of the Eta language spec, token immediately preceding assignment cannot be (, {, or "
assignmentStatement :
    lvalueList ASSIGN nonemptyExpressionList;

lvalueList :
     items+=lvalue (COMMA items+=lvalue)*;

lvalue :
    ID COLON typeSpecifierInAssignmentOrMethod                                  #declarationLvalue
    | ID                                                                        #assignmentIdLvalue
    | expression LBRACKET expression RBRACKET                                  #arrayCellAssignment
    | UNDERLINE                                                                #wildcardLvalue
    | {isRho}? expression DOT ID                                               #fieldAccessLvalue
    ;

// A declaration statement that declares multiple symbols as variables at the same time. Example: a : int, b : bool;
// If an array is declared, then any brackets in the symbol containing index expressions must preceed the brackets
// that do not contain index expressions. This is legal: a : int[3][]; But this is not: b: int[1][][3]
declarationStatement :
    items+=declaration (COMMA items+=declaration)*                            #etaDeclarationStatement
    | sameTypeDeclarationList                                        #rhoDeclarationStatement
    ;

declaration :
    ID COLON typeSpecifierInDeclaration;

typeSpecifierInDeclaration :
    baseType (LBRACKET indexes+=expression RBRACKET)*  (LBRACKET brackets+=RBRACKET)*;
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////


////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
// an expression that can appear on the right hand side of assignment statements and on the guards of if and while
// statements. Because of the way ANTLR4 resolves precedence and associativity we need to put all alternatives of
// the expression rule directly in the expression rule (without intermediary rules).
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
expression :
    LPAREN expression RPAREN                                        #parenthesisExpression
    | expression LBRACKET expression RBRACKET                       #arrayAccessExpression
    | expression {isRho}? DOT ID                                    #fieldAccessExpression
    | ID LPAREN expressionList RPAREN                    #functionApplicationExpression
    | LENGTH LPAREN expression RPAREN                               #lengthExpression
    | LBRACE expressionList COMMA? RBRACE                          #arrayInitializerExpression
    | negative=MINUS? INTEGER                                         #integerAtom
    | op=(MINUS | BANG) expression                                     #unaryExpression
    | expression op=(MULT | HIGH_MULT | DIV | MOD) expression          #multiplicativeExpression
    | expression op=(PLUS | MINUS) expression                          #additiveExpression
    | expression op=(LESS_THAN | LESS_THAN_OR_EQ | GREATER_THAN | GREATER_THAN_OR_EQ) expression
                                                                    #inequalityExpression
    | expression op=(EQUAL | NOT_EQUAL) expression                     #equalityExpression
    | expression op=AND expression                                     #logicalAndExpression
    | expression op=OR expression                                      #logicalOrExpession
    | ID                                                            #identifierAtom
    | STRING                                                        #stringAtom
    | CHARACTER                                                     #characterAtom
    | TRUE                                                          #trueAtom
    | FALSE                                                         #falseAtom
    | {isRho}? NULL_V                                               #nullAtom
    ;

expressionList :
    //empty
    | nonemptyExpressionList;

nonemptyExpressionList :
    items+=expression (COMMA items+=expression)*;
////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////