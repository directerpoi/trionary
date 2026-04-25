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

#define TRI_VERSION "Trionary v0.3.2"

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

/* Run all test_*.tri files found in 'dir' against their .expected files.
   'exe' is the path to the interpreter (argv[0] from main).
   Returns 0 if every test passes, 1 if any test fails.

   Uses fork/execvp instead of system() to avoid shell-injection risks:
   all arguments are passed as separate execvp array entries, never
   concatenated into a shell command string. */
static int run_tests(const char* dir, const char* exe) {
    DIR* d = opendir(dir);
    if (!d) {
        fprintf(stderr, "Error: Cannot open test directory '%s'\n", dir);
        return 1;
    }

    /* Collect paths of test_*.tri files.
       Minimum valid name is "test_?.tri" (9 chars, one char between _ and .tri). */
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

    /* Sort alphabetically for a consistent, deterministic run order. */
    if (nfiles > 0)
        qsort(files, nfiles, sizeof(char*), cmp_strptr);

    int pass = 0, fail = 0;

    for (int i = 0; i < nfiles; i++) {
        const char* tri_path = files[i];
        size_t tri_len       = strlen(tri_path);

        /* Derive the base path (strip the ".tri" suffix). */
        char base[1024];
        if (tri_len - 4 >= sizeof(base)) { fail++; continue; }
        memcpy(base, tri_path, tri_len - 4);
        base[tri_len - 4] = '\0';

        /* Short display name (basename only). */
        const char* tname = strrchr(tri_path, '/');
        tname = tname ? tname + 1 : tri_path;

        /* Skip if there is no .expected file. */
        char expected[1024];
        snprintf(expected, sizeof(expected), "%s.expected", base);
        {
            FILE* ef = fopen(expected, "r");
            if (!ef) { printf("SKIP: %s (no .expected file)\n", tname); continue; }
            fclose(ef);
        }

        /* Read optional extra CLI arguments from a .args file (one line).
           Arguments are split on whitespace and passed to execvp individually,
           so they are never interpreted by a shell. */
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

        /* Determine stdin source (a .stdin file or /dev/null). */
        char stdin_path[1024];
        snprintf(stdin_path, sizeof(stdin_path), "%s.stdin", base);
        const char* stdin_src;
        {
            FILE* sf = fopen(stdin_path, "r");
            stdin_src = sf ? stdin_path : "/dev/null";
            if (sf) fclose(sf);
        }

        /* Create a temporary file to capture combined stdout + stderr.
           The mkstemp fd is kept open; the child inherits it via dup2 so
           the OS serialises writes and there is no TOCTOU window. */
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

        /* Open stdin source file before forking. */
        int stdin_fd = open(stdin_src, O_RDONLY);
        if (stdin_fd < 0) stdin_fd = open("/dev/null", O_RDONLY);

        /* Build the argv array for the child: exe run tri_path [extra args...] */
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
            /* Child: wire up stdin/stdout/stderr and exec the interpreter. */
            dup2(stdin_fd, STDIN_FILENO);
            dup2(tmp_fd, STDOUT_FILENO);
            dup2(tmp_fd, STDERR_FILENO);
            close(stdin_fd);
            close(tmp_fd);
            execvp(exe, (char* const*)child_argv);
            _exit(1); /* exec failed */
        }

        /* Parent: wait for the child then close our copies of the fds. */
        close(stdin_fd);
        close(tmp_fd);
        waitpid(pid, NULL, 0);

        /* Compare output against the expected file. */
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
    printf("\nExamples:\n");
    printf("  %s run script.tri\n", prog_name);
    printf("  %s run script.tri 10 20\n", prog_name);
    printf("  %s test\n", prog_name);
    printf("  %s test ./mytests\n", prog_name);
}

int main(int argc, char* argv[]) {
    /* Handle no-argument invocation and global flags */
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
        size_t cmd_len = strlen(argv[1]);
        if (cmd_len > 4 && strcmp(argv[1] + cmd_len - 4, ".tri") == 0) {
            fprintf(stderr, "  Did you mean 'tri run %s'?\n", argv[1]);
        }
        print_help(argv[0]);
        return 1;
    }

    if (argc < 3) {
        fprintf(stderr, "Error: 'run' requires a file argument.\n");
        fprintf(stderr, "  Usage: %s run <file.tri> [arg0 arg1 ...]\n", argv[0]);
        return 1;
    }

    const char* filename = argv[2];
    char* source = read_file(filename);
    
    if (source == NULL) {
        fprintf(stderr, "Error: %s\n", get_reader_error());
        return 1;
    }

    /* Make the raw source available to error_at() for context-line printing. */
    set_error_source(source);

    int token_count = 0;
    Token* tokens = tokenise(source, &token_count);
    
    if (tokens[0].type == TOK_ERROR) {
        free(source);
        free(tokens);
        return 1;
    }

    /* source is freed after the execution loop so that error_at() can still
       print context lines during parse and runtime errors. */

    SymTable* sym = create_symtable();
    FuncTable* ft = create_functable();

    /* Register CLI arguments as built-in variables.
       argv[3..] become arg0, arg1, … in the script.
       All values are auto-coerced to double via atof().
       argc holds the count of script arguments (not counting tri/run/<file>). */
    int script_argc = argc - 3;
    sym_set(sym, "argc", (double)script_argc);
    for (int i = 0; i < script_argc; i++) {
        char argname[32];
        snprintf(argname, sizeof(argname), "arg%d", i);
        sym_set(sym, argname, atof(argv[3 + i]));
    }

    /* Multi-pipeline loop: each iteration extracts one statement (assignment,
       arithmetic emit, full lst…emt pipeline, or fn…end function definition)
       and dispatches it through parse() + execute().  The symbol table and
       function table are shared across all iterations so variables and
       functions defined earlier remain visible to subsequent statements.
       TOK_NEWLINE tokens are emitted by the lexer and are used only inside
       fn definitions (to separate the parameter list from the body); all
       other statement parsers receive a NEWLINE-free token slice. */
    int current = 0;
    while (current < token_count && tokens[current].type != TOK_EOF) {
        /* Skip any leading newlines between statements. */
        if (tokens[current].type == TOK_NEWLINE) {
            current++;
            continue;
        }

        // Start of a new statement
        int start = current;
        
        // Parse assignment pattern: IDENT = NUMBER | IDENT | inpt
        if (tokens[current].type == TOK_IDENT && 
            current + 2 < token_count && 
            tokens[current + 1].type == TOK_ASSIGN &&
            (tokens[current + 2].type == TOK_NUMBER ||
             tokens[current + 2].type == TOK_IDENT  ||
             tokens[current + 2].type == TOK_INPT)) {
            current += 3;
        }
        // Parse function definition: fn NAME params... NEWLINE body NEWLINE end
        else if (tokens[current].type == TOK_FN) {
            current++; // consume fn
            while (current < token_count &&
                   tokens[current].type != TOK_EOF &&
                   tokens[current].type != TOK_END) {
                current++;
            }
            if (tokens[current].type == TOK_END) current++;
        }
        // Parse use directive: use MODULE_NAME
        else if (tokens[current].type == TOK_USE) {
            current++; // consume use
            if (current < token_count && tokens[current].type == TOK_IDENT)
                current++; // consume module name
        }
        // Parse standalone inpt statement: inpt IDENT [STRING]
        else if (tokens[current].type == TOK_INPT) {
            current++; // consume inpt
            if (current < token_count && tokens[current].type == TOK_IDENT)
                current++; // consume identifier
            if (current < token_count && tokens[current].type == TOK_STRING)
                current++; // consume optional prompt string
        }
        // Parse pattern starting with lst: lst [ ... ] | ... emt
        else if (tokens[current].type == TOK_LST) {
            current++; // consume lst
            while (current < token_count && 
                   tokens[current].type != TOK_EOF &&
                   tokens[current].type != TOK_EMT &&
                   !(tokens[current].type == TOK_ARROW && 
                     current + 1 < token_count && 
                     tokens[current+1].type == TOK_EMT)) {
                current++;
            }
            if (tokens[current].type == TOK_EMT) current++;
            else if (tokens[current].type == TOK_ARROW && 
                     current + 1 < token_count && 
                     tokens[current+1].type == TOK_EMT) {
                current += 2;
            }
            /* Consume optional label string after emt (U1) */
            if (current < token_count && tokens[current].type == TOK_STRING)
                current++;
            /* Consume optional sep "string" after emt [label] (U10) */
            if (current < token_count &&
                tokens[current].type == TOK_IDENT &&
                strcmp(tokens[current].lexeme, "sep") == 0) {
                current++; /* consume "sep" */
                if (current < token_count && tokens[current].type == TOK_STRING)
                    current++; /* consume separator string */
            }
        }
        // Parse arithmetic followed by -> emt or variable starting new statement
        else if (tokens[current].type == TOK_NUMBER || tokens[current].type == TOK_IDENT) {
            while (current < token_count && 
                   tokens[current].type != TOK_EOF &&
                   tokens[current].type != TOK_ARROW) {
                current++;
            }
            if (tokens[current].type == TOK_ARROW) {
                current++; // consume ->
                if (current < token_count && tokens[current].type == TOK_EMT) {
                    current++; // consume emt
                    /* Consume optional label string after emt (U1) */
                    if (current < token_count && tokens[current].type == TOK_STRING)
                        current++;
                }
            }
        }
        // Standalone emt ["label"] expr  (U1)
        else if (tokens[current].type == TOK_EMT) {
            current++; // consume emt
            if (current < token_count && tokens[current].type == TOK_STRING)
                current++; // consume optional label
            // consume the expression tokens up to the next statement boundary
            while (current < token_count &&
                   tokens[current].type != TOK_EOF &&
                   tokens[current].type != TOK_NEWLINE) {
                current++;
            }
        } else {
            current++; // skip unknown token
            continue;
        }
        
        int stmt_len = current - start;
        if (stmt_len > 0) {
            ASTNode* ast;
            if (tokens[start].type == TOK_FN) {
                /* fn slices keep NEWLINE tokens — the parser uses them as
                   the separator between the parameter list and the body. */
                ast = parse(&tokens[start], stmt_len);
            } else {
                /* All other statement types: strip NEWLINE tokens so the
                   existing parsers do not encounter unexpected token types. */
                Token* buf = malloc(stmt_len * sizeof(Token));
                if (!buf) {
                    free(tokens);
                    free_symtable(sym);
                    free_functable(ft);
                    return 1;
                }
                int bc = 0;
                for (int k = start; k < current; k++) {
                    if (tokens[k].type != TOK_NEWLINE)
                        buf[bc++] = tokens[k];
                }
                ast = parse(buf, bc);
                free(buf);
            }

            if (ast == NULL) {
                free(tokens);
                free_symtable(sym);
                free_functable(ft);
                return 1;
            }
            
            execute(ast, sym, ft);
            free_ast(ast);
        }
    }

    free(tokens);
    free_symtable(sym);
    free_functable(ft);
    free(source);
    return 0;
}