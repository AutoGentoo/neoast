//
// Created by tumbar on 12/22/20.
//

#ifndef NEOAST_COMMON_H
#define NEOAST_COMMON_H

#include <neoast.h>
#include <stdint.h>

#define ARR_LEN(arr) ((sizeof(arr)) / sizeof(((arr)[0])))
#define STACK_PUSH(stack, i) (stack)->data[((stack)->pos)++] = (i)
#define STACK_POP(stack) (stack)->data[--((stack)->pos)]
#define STACK_PEEK(stack) (stack)->data[(stack)->pos - 1]

#endif //NEOAST_COMMON_H
