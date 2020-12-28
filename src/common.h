//
// Created by tumbar on 12/22/20.
//

#ifndef NEOAST_COMMON_H
#define NEOAST_COMMON_H

#include <stdint.h>

#define ARR_LEN(arr) (sizeof(arr)) / sizeof((arr[0]))
#define STACK_PUSH(stack, i) (stack)->data[((stack)->pos)++] = (i)
#define STACK_POP(stack) (stack)->data[--((stack)->pos)]
#define STACK_PEEK(stack) (stack)->data[(stack)->pos - 1]

typedef struct LexerRule_prv LexerRule;
typedef struct GrammarParser_prv GrammarParser;
typedef struct GrammarRule_prv GrammarRule;
typedef struct Stack_prv Stack;

// Codegen
typedef struct LR_1_prv LR_1;
typedef struct GrammarState_prv GrammarState;
typedef struct CanonicalCollection_prv CanonicalCollection;

typedef uint8_t U8;
typedef uint16_t U16;
typedef uint32_t U32;
typedef uint64_t U64;
typedef int8_t I8;
typedef int16_t I16;
typedef int32_t I32;
typedef int64_t I64;

struct Stack_prv
{
    U32 pos;
    U32 data[];
};

#endif //NEOAST_COMMON_H
