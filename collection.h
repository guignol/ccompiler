#ifndef CCOMPILER_COLLECTION_H
#define CCOMPILER_COLLECTION_H

typedef struct int_stack int_stack;

int_stack *create_int_stack();

int peek_int(int_stack **stack_pointer);

int pop_int(int_stack **stack_pointer);

void push_int(int_stack **stack_pointer, int value);

#endif //CCOMPILER_COLLECTION_H
