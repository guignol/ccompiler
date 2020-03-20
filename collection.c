#include "std_alternative.h"
#include "collection.h"

typedef struct int_stack int_stack;

struct int_stack {
    int value;
    int_stack *next;
};

int_stack *create_int_stack() {
    int_stack *stack = malloc(sizeof(int_stack *));
    return stack;
}

int peek_int(int_stack **stack_pointer) {
    int_stack *head = *stack_pointer;
    return head->value;
}

int pop_int(int_stack **stack_pointer) {
    int_stack *head = *stack_pointer;
    int i = head->value;
    int_stack *next = head->next;
    *stack_pointer = next;
    free(head);
    return i;
}

void push_int(int_stack **stack_pointer, int value) {
    int_stack *head = create_int_stack();
    head->value = value;
    head->next = *stack_pointer;
    *stack_pointer = head;
}