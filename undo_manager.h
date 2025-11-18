#ifndef UNDO_MANAGER_H
#define UNDO_MANAGER_H

#include <stddef.h>

typedef size_t undo_sp_t;
typedef void (*undo_fn)(void *context);

typedef struct undo_action_t {
    undo_fn undo;
    void * context;
    struct undo_action_t * next;
} undo_action_t;

typedef struct undo_manager_t {
    void (*push)(undo_fn undo, void *context);
    void (*commit)(void);
    void (*abort)(void);
    undo_sp_t (*mark)(void);
    void (*rollback_to)(undo_sp_t sp);
    void (*commit_to)(undo_sp_t sp);
} undo_manager_t;

extern undo_manager_t Undo;

#endif // UNDO_MANAGER_H
