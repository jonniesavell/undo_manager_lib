# undo_manager_lib

have you ever wondered how to write a C program properly
without try/catch or try/finally? suppose that you get an
error from a system call whose result you needed in order
to make progress. what do you do? your failure blocks grow
larger and larger as you maintain more and more work to
undo. the following example shows only two activities to
undo but you can see how each successive failure block
accrues additional undo activities.

```
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

typedef struct {
    char * filename;
} undo_file_context_t;

void undo_file_creation(void * ctx) {
    undo_file_context_t * fctx = (undo_file_context_t *) ctx;
    unlink(fctx->filename);
    free(fctx->filename);
    free(fctx);
}

int main() {
    // previous work ...

    char contents[] = "temporary data";
    char name[] = "/tmp/tmp1.txt";
    int file_descriptor = open(name, O_RDWR | O_CREAT | O_TRUNC, S_IRWXU);

    if (file_descriptor == -1) {
        perror("create file");
        exit(1);
    }

    undo_file_context_t * ctx = malloc(sizeof(undo_file_context_t));
    ctx->filename = strdup(name);

    ssize_t number_of_bytes = 0;

    do {
        number_of_bytes = write(file_descriptor, contents + number_of_bytes, strlen(contents) - number_of_bytes);
    } while (number_of_bytes > 0);

    if (number_of_bytes == -1) {
        perror("write file");
        undo_file_creation(ctx);
        exit(1);
    }

    char contents2[] = "more temporary data";
    char name2[] = "/tmp/tmp2.txt";
    file_descriptor = open(name2, O_RDWR | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
    number_of_bytes = 0;

    if (file_descriptor == -1) {
        perror("create file");
        undo_file_creation(ctx);
        exit(1);
    }

    undo_file_context_t * ctx2 = malloc(sizeof(undo_file_context_t));
    ctx2->filename = strdup(name2);

    number_of_bytes = 0;

    do {
        number_of_bytes = write(file_descriptor, contents2 + number_of_bytes, strlen(contents2) - number_of_bytes);
    } while (number_of_bytes > 0);

    if (number_of_bytes == -1) {
        perror("write file");
        undo_file_creation(ctx2);
        undo_file_creation(ctx);
        exit(1);
    }

    // all resources have been created but an error is encountered. rollback_to the point in time before we started
    //   creating resources. you can verify that no files created here remain in /tmp.

    undo_file_creation(ctx2);
    undo_file_creation(ctx);
    exit(1);
}
```

use of the undo-manager requires that you supply a
pointer of a cleanup function prior to each operation.
in the event that the operation fails, you either
commit (unwind the stack of undo operations without
performing any) or rollback (unwind the stack of undo
operations, executing each in turn). this results in
a lot of functions, but it also makes failures easier
to handle. at the very least, you have fewer error
handling instructions within each successive failure
block. the tradeoff is between incremental
maintenance of state and on demand maintenance of
state.

```
#include "undo_manager.h"
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

typedef struct {
    char * filename;
} undo_file_context_t;

void undo_file_creation(void * ctx) {
    undo_file_context_t * fctx = (undo_file_context_t *) ctx;
    unlink(fctx->filename);
    free(fctx->filename);
    free(fctx);
}

int main() {
    // previous work ... save the point to which we rollback or commit.
    undo_sp_t original_save_point = Undo.mark();

    char contents[] = "temporary data";
    char name[] = "/tmp/tmp1.txt";
    int file_descriptor = open(name, O_RDWR | O_CREAT | O_TRUNC, S_IRWXU);

    if (file_descriptor == -1) {
        perror("create file");
        Undo.rollback_to(original_save_point);
        exit(1);
    }

    undo_file_context_t * ctx = malloc(sizeof(undo_file_context_t));
    ctx->filename = strdup(name);
    Undo.push(undo_file_creation, ctx);

    ssize_t number_of_bytes = 0;

    do {
        number_of_bytes = write(file_descriptor, contents + number_of_bytes, strlen(contents) - number_of_bytes);
    } while (number_of_bytes > 0);

    if (number_of_bytes == -1) {
        perror("write file");
        Undo.rollback_to(original_save_point);
        exit(1);
    }

    char contents2[] = "more temporary data";
    char name2[] = "/tmp/tmp2.txt";
    file_descriptor = open(name2, O_RDWR | O_CREAT | O_TRUNC, S_IRUSR | S_IWUSR);
    number_of_bytes = 0;

    if (file_descriptor == -1) {
        perror("create file");
        Undo.rollback_to(original_save_point);
        exit(1);
    }

    ctx = malloc(sizeof(undo_file_context_t));
    ctx->filename = strdup(name2);
    Undo.push(undo_file_creation, ctx);

    number_of_bytes = 0;

    do {
        number_of_bytes = write(file_descriptor, contents2 + number_of_bytes, strlen(contents2) - number_of_bytes);
    } while (number_of_bytes > 0);

    if (number_of_bytes == -1) {
        perror("write file");
        Undo.rollback_to(original_save_point);
        exit(1);
    }

    // all resources have been created but an error is encountered. rollback_to the point in time before we started
    //   creating resources. you can verify that no files created here remain in /tmp.

    Undo.rollback_to(original_save_point);
    exit(1);
}
'''