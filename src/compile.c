#include "compile.h"

#include <stdio.h>
#include <stdlib.h>

bool compile_ast(node_t *node) {
    switch (node->type) {
        case (NUM): {
            printf("movq $%ld, %%rdi\n", ((num_node_t *) node)->value);
            return true;
        }
        case (PRINT): {
            if (!compile_ast(((print_node_t *) node)->expr)) {
                return false;
            }
            printf("callq print_int\n");
            return true;
        }
        case (SEQUENCE): {
            sequence_node_t *sequence = (sequence_node_t *) node;
            for (size_t i = 0; i < sequence->statement_count; i++) {
                if (!compile_ast(sequence->statements[i])) {
                    return false;
                }
            }
            return true;
        }
        case (BINARY_OP): {
            binary_node_t *bin = (binary_node_t *) node;
            if (!compile_ast(bin->left)) {
                return false;
            }
            printf("pushq %%rdi\n");
            if (!compile_ast(bin->right)) {
                return false;
            }
            printf("popq %%rax\n");
            switch (bin->op) {
                case ('/'): {
                    printf("cqto\n");
                    printf("idivq %%rdi\n");
                    printf("movq %%rax, %%rdi\n");
                    return true;
                }
                case ('*'): {
                    printf("imulq %%rax, %%rdi\n");

                    return true;
                }
                case ('-'): {
                    printf("subq %%rdi, %%rax\n");
                    printf("movq %%rax, %%rdi\n");
                    return true;
                }
                case ('+'): {
                    printf("addq %%rax, %%rdi\n");
                    return true;
                }
            }
        }
        case (VAR): {
            break;
        }
        case (LET): {
            break;
        }
        case (IF): {
            break;
        }
        case (WHILE): {
            break;
        }
    }
    return false; // for now, every statement causes a compilation failure
}
