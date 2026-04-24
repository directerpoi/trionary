#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "reader.h"
#include "lexer.h"
#include "parser.h"
#include "exec.h"

void print_usage(const char* prog_name) {
    fprintf(stderr, "Usage: %s run <file.tri>\n", prog_name);
    fprintf(stderr, "  Executes a Trionary v0.3.0 source file.\n");
}

int main(int argc, char* argv[]) {
    if (argc < 3) {
        print_usage(argv[0]);
        return 1;
    }

    if (strcmp(argv[1], "run") != 0) {
        fprintf(stderr, "Error: Unknown command '%s'\n", argv[1]);
        print_usage(argv[0]);
        return 1;
    }

    const char* filename = argv[2];
    char* source = read_file(filename);
    
    if (source == NULL) {
        fprintf(stderr, "Error: Could not read file '%s'\n", filename);
        return 1;
    }

    int token_count = 0;
    Token* tokens = tokenise(source, &token_count);
    
    if (tokens[0].type == TOK_ERROR) {
        free(source);
        free(tokens);
        return 1;
    }

    free(source);

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
                }
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
    return 0;
}