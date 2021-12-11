# Builtin Lexer

The builtin lexer is used for the bootstrap compiler-compiler.
It combines all the lexing patterns to generate a large
regular expression where every lexer rule is a capture group.
Then it uses reflex pattern matching at runtime to break inputs
into token.

> This method is quite a bit slower than the neoast-lexer and also
> depends on libreflex and libc++
