// The Unilog grammar. See docs/language.md.
//
// Names are classified after parsing (src/parser.ts): in a proposition, a name starting with an uppercase
// letter or '_' is a variable, anything else is an atom.
grammar Unilog;

file
    : decl* EOF
    ;

// A single proposition on its own, e.g. a query: `unilog find "wrong X"`.
query
    : prop EOF
    ;

decl
    : IMPORT NAME terminator                       # importDecl
    | NAME COLON prop (EQUALS proof)? terminator   # claimDecl       // axiom, or inference with a stated claim
    | NAME EQUALS proof terminator                 # inferenceDecl   // inference, claim inferred
    ;

// Declarations end with ';'. A '.' is accepted here only to report the old terminator.
terminator
    : SEMI
    | DOT
    ;

// '=>' is right-associative; application (juxtaposition) binds tighter.
prop
    : propApp (ARROW prop)?
    ;

propApp
    : propAtom+
    ;

propAtom
    : NAME              # propName
    | LPAREN prop RPAREN  # propParens
    ;

proof
    : LAMBDA proof                   # assume        // \P. body, or \P Q. body for \P. \Q. body
    | BACKSLASH NAME+ MAPS_TO proof  # arrowAssume   // \P -> body: an old syntax, parsed only to report it
    | BACKSLASH NAME*                # badAssume     // \P without the '.': parsed only to report it
    | NAME ARROW proof               # oldAssume     // P => body: an old syntax, parsed only to report it
    | proofAtom+                     # apply         // b a, applied left to right
    ;

proofAtom
    : NAME                 # proofName   // may be qualified: Mod.Sub.name
    | LPAREN proof RPAREN  # proofParens
    ;

IMPORT : 'import' ;
ARROW  : '=>' ;
MAPS_TO : '->' ;
BACKSLASH : '\\' ;
SEMI   : ';' ;

// The head of an assumption, '\F G X.', is one token: otherwise '\X.F' would read as the qualified name X.F.
LAMBDA : '\\' [ \t\r\n]* NAME_PART ([ \t\r\n]+ NAME_PART)* [ \t\r\n]* '.' ;
COLON  : ':' ;
EQUALS : '=' ;
LPAREN : '(' ;
RPAREN : ')' ;

// A name, optionally qualified (Mod.Sub.name): '.' joins the parts, with no spaces around it.
// A '.' anywhere else is DOT, which only appears in old-style code and is reported.
NAME : NAME_PART ('.' NAME_PART)* ;
DOT  : '.' ;

fragment NAME_PART : [A-Za-z0-9_']+ ;

COMMENT : '#' ~[\r\n]* -> skip ;
WS      : [ \t\r\n]+ -> skip ;

// Anything else is an error token, so the parser reports it with a location.
UNEXPECTED : . ;
