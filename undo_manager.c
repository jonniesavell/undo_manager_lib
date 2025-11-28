#include "undo_manager.h"
#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>

typedef struct undo_stack_t {
    undo_action_t * top;
    undo_sp_t count;
} undo_stack_t;

static pthread_key_t key;
static pthread_once_t key_once = PTHREAD_ONCE_INIT;

static void remove_one(undo_stack_t * stack, int to_delete) {

    if (stack->count > 0) {
        undo_action_t * cur = stack->top;
        stack->top = cur->next;

        if (to_delete && (cur->undo != NULL) && (cur->context != NULL)) {
            cur->undo(cur->context);
        }

        free(cur);
        stack->count--;
    }
}

static void destroy(void * memory) {
    undo_stack_t * stack = (undo_stack_t *) memory;

    while (stack->count > 0) {
        remove_one(stack, 0);
    }

    pthread_setspecific(key, NULL);
    free(stack);
}

static void make_key() {
    pthread_key_create(&key, destroy);  // Automatically frees the stack on thread exit
}

static undo_stack_t * get_stack() {
    pthread_once(&key_once, make_key);
    undo_stack_t * stack = pthread_getspecific(key);

    if (!stack) {
        stack = calloc(1, sizeof(undo_stack_t));

        if (stack == NULL) {
            fprintf(stderr, "undo stack memory allocation failed\n");
            abort();
        }

        int return_value = pthread_setspecific(key, stack);

        if (return_value != 0) {
            fprintf(stderr, "thread specific data mutation failed\n");
            abort();
        }
    }

    return stack;
}

static undo_sp_t mark_impl(void) {
    return get_stack()->count;
}

static void push_impl(undo_fn fn, void * context) {
    undo_stack_t * stack = get_stack();
    undo_action_t * action = malloc(sizeof(undo_action_t));
    action->undo = fn;
    action->context = context;
    action->next = stack->top;
    stack->top = action;
    stack->count++;
}

static void commit_impl() {
    undo_stack_t * stack = get_stack();
    undo_action_t * cur = stack->top;

    while (cur) {
        remove_one(stack, 0);
        cur = stack->top;
    }

    stack->top = NULL;
    stack->count = 0;
}

static void rollback_impl() {
    undo_stack_t * stack = get_stack();
    undo_action_t * cur = stack->top;

    while (cur) {
        remove_one(stack, 1);
        cur = stack->top;
    }

    stack->top = NULL;
    stack->count = 0;
}

static void rollback_to_impl(undo_sp_t sp) {
    undo_stack_t * stack = get_stack();

    while (stack->count > sp) {
        remove_one(stack, 1);
    }
}

static void commit_to_impl(undo_sp_t sp) {
    undo_stack_t * stack = get_stack();

    while (stack->count > sp) {
        remove_one(stack, 0);
    }
}

// Exported Envelope
undo_manager_t Undo = {
    .push = push_impl,
    .mark = mark_impl,
    .rollback_to = rollback_to_impl,
    .rollback = rollback_impl,   // rollback_to(beginning_of_time)
    .commit_to = commit_to_impl,
    .commit = commit_impl        // commit_to(beginning_of_time)
};
