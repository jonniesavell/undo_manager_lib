# undo_manager_lib

have you ever wondered how to write a C project properly
without try/catch. suppose that you get an error from a
system call whose result you needed in order to make
progress. what do you do?

use of the undo-manager requires that you supply a
pointer of a cleanup function prior to each operation.
in the event that the operation fails, you either
commit (unwind the stack of undo operations without
performing any) or rollback (unwind the stack of undo
operations, executing each in turn). this results in a
lot of functions, but it also makes failures easy to
handle.

```
typedef struct {
    char * filename;
    int file_descriptor;
} file_context_t;

void undo_create_file(void * ctx) {
    file_context_t * fctx = (file_context_t *) ctx;
    close(fctx->file_descriptor);
    unlink(fctx->filename);
    free(fctx->filename);
    free(fctx);
}

void create_temp_file(const char * name) {
    char contents[] = "temporary data";
    int file_descriptor = open(name, O_RDWR | O_CREAT | O_TRUNC, S_IRWXU);
    int number_of_bytes = 0;

    if (file_descriptor == -1) {
        perror("create file");
        Undo.abort();
        exit(1);
    }

    while ((number_of_bytes = write(file_descriptor, contents + number_of_bytes, strlen(contents) - number_of_bytes)) > 0) {
    }

    if (number_of_bytes == -1) {
        perror("write file");
    }

    file_context_t * ctx = malloc(sizeof(file_context_t));
    ctx->filename = strdup(name);
    ctx->file_descriptor = file_descriptor;
    Undo.push(undo_create_file, ctx);
}

int main() {
    undo_sp_t save_point = Undo.mark();
    create_temp_file("tmp1.txt");

    int to_rollback = 1;

    if (to_rollback) {
        fprintf(stderr, "Error occurred, rolling back\n");
        Undo.rollback_to(save_point);
    } else {
        fprintf(stdout, "delight occurred\n");
        Undo.commit_to(save_point);
    }

    undo_sp_t save_point2 = Undo.mark();
    create_temp_file("tmp2.txt");

    to_rollback = 1;

    if (to_rollback) {
        fprintf(stderr, "Error occurred, rolling back\n");
        Undo.rollback_to(save_point);
    } else {
        fprintf(stdout, "delight occurred\n");
        Undo.commit_to(save_point);
    }

    return 0;
}
'''
