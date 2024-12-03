#ifndef ERROR_HPP
#define ERROR_HPP

// swipl errors
#define ERR_MSG_UNIFY "Error: failed to unify terms"
#define ERR_MSG_CONS_LIST "Error: failed to cons list"
#define ERR_MSG_GET_ATOM_CHARS "Error: failed to get atom chars"
#define ERR_MSG_PUT_ATOM_CHARS "Error: failed to put atom chars"
#define ERR_MSG_PUT_NIL "Error: failed to put nil"

// lexer errors
#define ERR_MSG_CLOSING_QUOTE "Error: no closing quote"
#define ERR_MSG_INVALID_LEXEME "Error: invalid lexeme"

// parser errors
#define ERR_MSG_NO_LIST_CLOSE "Error: no list close"
#define ERR_MSG_MALFORMED_TERM "Error: malformed term"
#define ERR_MSG_INVALID_COMMAND "Error: invalid command"
#define ERR_MSG_MALFORMED_STMT "Error: malformed statement"
#define ERR_MSG_NO_EOL "Error: expected end-of-line (;)"

// executor errors
#define ERR_MSG_NOT_A_FILE "Error: not a file"
#define ERR_MSG_FILE_OPEN "Error: file failed to open"
#define ERR_MSG_DECL_THEOREM "Error: failed to declare theorem"
#define ERR_MSG_DECL_REDIR "Error: failed to declare redirect"
#define ERR_MSG_INFER "Error: inference failed"

#endif
