#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/wait.h>
#include "reader.h"
#include "lexer.h"
#include "parser.h"
#include "exec.h"
#include "error.h"

#define TRI_VERSION "Trionary v0.4.0"

/* Returns 1 if the two files have identical contents, 0 otherwise. */
static int files_equal(const char* path1, const char* path2) {
    FILE* f1 = fopen(path1, "rb");
    FILE* f2 = fopen(path2, "rb");
    if (!f1 || !f2) { if (f1) fclose(f1); if (f2) fclose(f2); return 0; }
    int equal = 1;
    int c1, c2;
    while (1) {
        c1 = fgetc(f1);
        c2 = fgetc(f2);
        if (c1 != c2) { equal = 0; break; }
        if (c1 == EOF) break;
    }
    fclose(f1);
    fclose(f2);
    return equal;
}

/* qsort comparator for an array of char* pointers. */
static int cmp_strptr(const void* a, const void* b) {
    return strcmp(*(const char* const*)a, *(const char* const*)b);
}

static int run_tests(const char* dir, const char* exe) {
    DIR* d = opendir(dir);
    if (!d) {
        fprintf(stderr, "Error: Cannot open test directory '%s'\n", dir);
        return 1;
    }

    char** files = NULL;
    int nfiles = 0, cap = 0;
    struct dirent* entry;
    while ((entry = readdir(d)) != NULL) {
        const char* name = entry->d_name;
        size_t namelen   = strlen(name);
        if (namelen < 9 ||
            strncmp(name, "test_", 5) != 0 ||
            strcmp(name + namelen - 4, ".tri") != 0)
            continue;
        size_t plen = strlen(dir) + 1 + namelen + 1;
        char* path = malloc(plen);
        if (!path) continue;
        snprintf(path, plen, "%s/%s", dir, name);
        if (nfiles >= cap) {
            cap = cap ? cap * 2 : 16;
            char** tmp = realloc(files, cap * sizeof(char*));
            if (!tmp) { free(path); continue; }
            files = tmp;
        }
        files[nfiles++] = path;
    }
    closedir(d);

    if (nfiles > 0)
        qsort(files, nfiles, sizeof(char*), cmp_strptr);

    int pass = 0, fail = 0;

    for (int i = 0; i < nfiles; i++) {
        const char* tri_path = files[i];
        size_t tri_len       = strlen(tri_path);
        char base[1024];
        if (tri_len - 4 >= sizeof(base)) { fail++; continue; }
        memcpy(base, tri_path, tri_len - 4);
        base[tri_len - 4] = '\0';
        const char* tname = strrchr(tri_path, '/');
        tname = tname ? tname + 1 : tri_path;
        char expected[1024];
        snprintf(expected, sizeof(expected), "%s.expected", base);
        {
            FILE* ef = fopen(expected, "r");
            if (!ef) { printf("SKIP: %s (no .expected file)\n", tname); continue; }
            fclose(ef);
        }
        char args_buf[512] = "";
        char* arg_tokens[32];
        int   arg_token_count = 0;
        char args_path[1024];
        snprintf(args_path, sizeof(args_path), "%s.args", base);
        {
            FILE* af = fopen(args_path, "r");
            if (af) {
                if (fgets(args_buf, sizeof(args_buf), af)) {
                    size_t alen = strlen(args_buf);
                    while (alen > 0 &&
                           (args_buf[alen - 1] == '\n' || args_buf[alen - 1] == '\r'))
                        args_buf[--alen] = '\0';
                    char* tok = strtok(args_buf, " \t");
                    while (tok && arg_token_count < 30) {
                        arg_tokens[arg_token_count++] = tok;
                        tok = strtok(NULL, " \t");
                    }
                }
                fclose(af);
            }
        }
        char stdin_path[1024];
        snprintf(stdin_path, sizeof(stdin_path), "%s.stdin", base);
        const char* stdin_src;
        {
            FILE* sf = fopen(stdin_path, "r");
            stdin_src = sf ? stdin_path : "/dev/null";
            if (sf) fclose(sf);
        }
        const char* tmpdir = getenv("TMPDIR");
        if (!tmpdir || tmpdir[0] == '\0') tmpdir = "/tmp";
        char tmp_path[256];
        snprintf(tmp_path, sizeof(tmp_path), "%s/tri_test_XXXXXX", tmpdir);
        int tmp_fd = mkstemp(tmp_path);
        if (tmp_fd < 0) {
            fprintf(stderr, "Error: mkstemp failed\n");
            fail++;
            continue;
        }
        int stdin_fd = open(stdin_src, O_RDONLY);
        if (stdin_fd < 0) stdin_fd = open("/dev/null", O_RDONLY);
        const char* child_argv[36];
        int ci = 0;
        child_argv[ci++] = exe;
        child_argv[ci++] = "run";
        child_argv[ci++] = tri_path;
        for (int k = 0; k < arg_token_count; k++)
            child_argv[ci++] = arg_tokens[k];
        child_argv[ci] = NULL;
        pid_t pid = fork();
        if (pid < 0) {
            fprintf(stderr, "Error: fork failed\n");
            close(tmp_fd);
            close(stdin_fd);
            fail++;
            continue;
        }
        if (pid == 0) {
            dup2(stdin_fd, STDIN_FILENO);
            dup2(tmp_fd, STDOUT_FILENO);
            dup2(tmp_fd, STDERR_FILENO);
            close(stdin_fd);
            close(tmp_fd);
            execvp(exe, (char* const*)child_argv);
            _exit(1);
        }
        close(stdin_fd);
        close(tmp_fd);
        waitpid(pid, NULL, 0);
        if (files_equal(tmp_path, expected)) {
            printf("PASS: %s\n", tname);
            pass++;
        } else {
            printf("FAIL: %s\n", tname);
            fail++;
        }
        remove(tmp_path);
    }
    for (int i = 0; i < nfiles; i++) free(files[i]);
    free(files);
    printf("\nResults: %d passed, %d failed\n", pass, fail);
    return fail > 0 ? 1 : 0;
}

void print_help(const char* prog_name) {
    printf("Usage: %s <command> [arguments]\n\n", prog_name);
    printf("Commands:\n");
    printf("  run <file.tri> [arg0 arg1 ...]   Execute a Trionary source file\n");
    printf("  test [dir]                       Run tests in dir (default: ./tests)\n");
    printf("  help                             Show this help message\n");
    printf("  version                          Print the interpreter version\n");
}

int main(int argc, char* argv[]) {
    if (argc < 2 ||
        strcmp(argv[1], "help") == 0 ||
        strcmp(argv[1], "--help") == 0 ||
        strcmp(argv[1], "-h") == 0) {
        print_help(argv[0]);
        return 0;
    }

    if (strcmp(argv[1], "version") == 0) {
        printf("%s\n", TRI_VERSION);
        return 0;
    }

    if (strcmp(argv[1], "test") == 0) {
        const char* test_dir = (argc >= 3) ? argv[2] : "./tests";
        return run_tests(test_dir, argv[0]);
    }

    if (strcmp(argv[1], "run") != 0) {
        fprintf(stderr, "Error: Unknown command '%s'\n", argv[1]);
        print_help(argv[0]);
        return 1;
    }

    if (argc < 3) {
        fprintf(stderr, "Error: 'run' requires a file argument.\n");
        return 1;
    }

    const char* filename = argv[2];
    char* source = read_file(filename);
    if (source == NULL) {
        fprintf(stderr, "Error: %s\n", get_reader_error());
        return 1;
    }
    set_error_source(source);

    int token_count = 0;
    Token* tokens = tokenise(source, &token_count);
    if (tokens[0].type == TOK_ERROR) {
        free(source);
        free(tokens);
        return 1;
    }

    SymTable* sym = create_symtable();
    FuncTable* ft = create_functable();

    int script_argc = argc - 3;
    Value argc_val; argc_val.type = VAL_NUMBER; argc_val.as.number = (double)script_argc; argc_val.is_immutable = 0;
    sym_set(sym, "argc", argc_val);
    for (int i = 0; i < script_argc; i++) {
        char argname[32];
        snprintf(argname, sizeof(argname), "arg%d", i);
        Value arg_val; arg_val.type = VAL_NUMBER; arg_val.as.number = atof(argv[3 + i]); arg_val.is_immutable = 0;
        sym_set(sym, argname, arg_val);
    }

    /* Parse and execute top-level statements one by one to maintain
       compatibility with tests that expect partial execution. */
    extern int current_pos;
    extern Token* global_tokens;
    extern int global_token_count;
    
    global_tokens = tokens;
    global_token_count = token_count;
    current_pos = 0;
    
    while (current_pos < global_token_count && global_tokens[current_pos].type != TOK_EOF) {
        if (global_tokens[current_pos].type == TOK_NEWLINE) {
            current_pos++;
            continue;
        }
        ASTNode* stmt = parse_statement();
        if (stmt) {
            execute(stmt, sym, ft);
            free_ast(stmt);
        } else {
            /* If it didn't match any statement, skip one token and try again,
               matching the original slicer behavior. */
            current_pos++;
        }
    }

    free(tokens);
    free_symtable(sym);
    free_functable(ft);
    free(source);
    return 0;
}
