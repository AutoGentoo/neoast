# neoast lexer

The neoast lexer will use a modified reflex executable at
compile time to extract the compiled FSM for the regular
expression to match all lexing options.

This lexer is powerful because it implements all the reflex
runtime features in a simple C library so that link time
dependencies of a normal neoast parser are kept to a minimum.

CREDITS TO Robert van Engelen - engelen@genivia.com for original
implementation of Reflex library
