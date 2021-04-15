/* 
 * This file is part of the Neoast framework
 * Copyright (c) 2021 Andrei Tumbar.
 * 
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, version 3.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */


#include "lexer.h"
#include <reflex/abslexer.h>
#include <reflex/matcher.h>

class Lexer : public reflex::AbstractLexer<reflex::Matcher>
{
public:
    typedef reflex::AbstractLexer<reflex::Matcher> AbstractBaseLexer;

    explicit Lexer(
            const reflex::Input &input = reflex::Input(),
            std::ostream &os = std::cout)
            :
            AbstractBaseLexer(input, os)
    {
    }

    static const int INITIAL = 0;
    static const int SCRIPT = 1;

    virtual int lex();

    int lex(const reflex::Input &input)
    {
        in(input);
        return lex();
    }

    int lex(const reflex::Input &input, std::ostream* os)
    {
        in(input);
        if (os)
            out(*os);
        return lex();
    }
};

int Lexer::lex()
{
    while (true)
    {
        switch (start())
        {
            case INITIAL:
                matcher().pattern(PATTERN_INITIAL);
                switch (matcher().scan())
                {
                    case 0:
                        if (matcher().at_end())
                        {
                            return int();
                        }
                        else
                        {
                            lexer_error("scanner jammed");
                            return int();
                        }
                        break;
                    case 1: // rule /home/atuser/git/neoast/third_party/reflex/unicode/block_scripts.l:60: ^#.*\n :
#line 60 "/home/atuser/git/neoast/third_party/reflex/unicode/block_scripts.l"
// skip comments
                        break;
                    case 2: // rule /home/atuser/git/neoast/third_party/reflex/unicode/block_scripts.l:61: ^\h*\n :
#line 61 "/home/atuser/git/neoast/third_party/reflex/unicode/block_scripts.l"
// skip empty lines
                        break;
                    case 3: // rule /home/atuser/git/neoast/third_party/reflex/unicode/block_scripts.l:62: ^{hex}".."{hex} :
#line 62 "/home/atuser/git/neoast/third_party/reflex/unicode/block_scripts.l"
                        sscanf(text(), "%x..%x", &lo, &hi);
                        start(SCRIPT);

                        break;
                }
                break;
            case SCRIPT:
                matcher().pattern(PATTERN_SCRIPT);
                switch (matcher().scan())
                {
                case 0:
                    if (matcher().at_end())
                    {
                        return int();
                    }
                    else
                    {
                        lexer_error("scanner jammed");
                        return int();
                    }
                    break;
                case 1: // rule /home/atuser/git/neoast/third_party/reflex/unicode/block_scripts.l:64: [; ]+ :
#line 64 "/home/atuser/git/neoast/third_party/reflex/unicode/block_scripts.l"
// skip ;, <space>
                    break;
                case 2: // rule /home/atuser/git/neoast/third_party/reflex/unicode/block_scripts.l:65: [\w\-]+ :
#line 65 "/home/atuser/git/neoast/third_party/reflex/unicode/block_scripts.l"
                    key += text();
                    break;
                case 3: // rule /home/atuser/git/neoast/third_party/reflex/unicode/block_scripts.l:66: \n :
#line 66 "/home/atuser/git/neoast/third_party/reflex/unicode/block_scripts.l"
                    p[key].insert(lo, hi);
                    key.clear();
                    start(INITIAL);

                    break;
                }
                break;
            default:
                start(0);
        }
    }
}


void lexer_init(GrammarParser* self)
{
    static const char* REGEX_INITIAL = "(?m)(^#.*\\n)|(^\\h*\\n)|(^(?:[0-9A-Fa-f]+)(?:\\Q..\\E)(?:[0-9A-Fa-f]+))";
    static const reflex::Pattern PATTERN_INITIAL(REGEX_INITIAL);
    static const char* REGEX_SCRIPT = "(?m)([\\x20;]+)|([\\x2d0-9A-Z_a-z]+)|(\\n)";
    static const reflex::Pattern PATTERN_SCRIPT(REGEX_SCRIPT);

    matcher(new reflex::Matcher(PATTERN_INITIAL, reflex::stdinit(), this));
}
