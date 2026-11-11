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
    : IMPORT NAME DOT                       # importDecl
    | NAME COLON prop (EQUALS proof)? DOT   # claimDecl     // axiom, or theorem with a stated claim
    | NAME EQUALS proof DOT                 # theoremDecl   // theorem, claim inferred
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
    : BACKSLASH NAME+ MAPS_TO proof  # assume      // \P -> body, or \P Q -> body for \P -> \Q -> body
    | NAME ARROW proof               # oldAssume   // P => body: the old syntax, parsed only to report it
    | proofAtom+                     # apply       // b a, applied left to right
    ;

proofAtom
    : NAME                 # proofName   // may be qualified: Mod.Sub.name
    | LPAREN proof RPAREN  # proofParens
    ;

IMPORT : 'import' ;
ARROW  : '=>' ;
MAPS_TO : '->' ;
BACKSLASH : '\\' ;
COLON  : ':' ;
EQUALS : '=' ;
LPAREN : '(' ;
RPAREN : ')' ;

// A name, optionally qualified (Mod.Sub.name). A '.' only joins two names when a name character follows it
// directly; otherwise it is DOT, which ends a declaration. The lexer's longest match does the rest.
NAME : NAME_PART ('.' NAME_PART)* ;
DOT  : '.' ;

fragment NAME_PART : [A-Za-z0-9_']+ ;

COMMENT : '#' ~[\r\n]* -> skip ;
WS      : [ \t\r\n]+ -> skip ;

// Anything else is an error token, so the parser reports it with a location.
UNEXPECTED : . ;
