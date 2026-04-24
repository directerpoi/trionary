#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "reader.h"
#include "lexer.h"
#include "parser.h"
#include "exec.h"

void print_usage(const char* prog_name) {
    fprintf(stderr, "Usage: %s run <file.tri>\n", prog_name);
    fprintf(stderr, "  Executes a Trionary v0 source file.\n");
}

int main(int argc, char* argv[]) {
    if (argc != 3) {
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
    
    int current = 0;
    while (current < token_count && tokens[current].type != TOK_EOF) {
        // Start of a new statement
        int start = current;
        
        // Parse assignment pattern: IDENT = NUMBER
        if (tokens[current].type == TOK_IDENT && 
            current + 2 < token_count && 
            tokens[current + 1].type == TOK_ASSIGN &&
            tokens[current + 2].type == TOK_NUMBER) {
            current += 3;
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
        }
        
        int stmt_len = current - start;
        if (stmt_len > 0) {
            ASTNode* ast = parse(&tokens[start], stmt_len);
            if (ast == NULL) {
                free(tokens);
                free_symtable(sym);
                return 1;
            }
            
            execute(ast, sym);
            free_ast(ast);
        }
    }

    free(tokens);
    free_symtable(sym);
    return 0;
}