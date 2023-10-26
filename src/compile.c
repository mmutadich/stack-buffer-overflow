#include "compile.h"

#include <stdio.h>
#include <stdlib.h>

int64_t COUNT = 0;

bool compile_ast(node_t *node) {
    if (node == NULL){
        return false;
    }
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
            var_node_t *var_node = (var_node_t *) node;
            var_name_t name = var_node->name;
            int64_t sub = (name - 'A' + 1) * -8;
            printf("movq %ld(%%rbp), %%rsi\n", sub);
            printf("movq %%rsi, %%rdi\n");
            return true;
        }
        case (LET): {
            let_node_t *let_node = (let_node_t *) node;
            var_name_t name = let_node->var;
            int64_t sub = (name - 'A' + 1) * -8;
            if (!compile_ast(let_node->value)) {
                return false;
            }
            printf("movq %%rdi, %ld(%%rbp)\n", sub);
            return true;
        }
        case (IF): {
            int64_t local = COUNT;
            COUNT += 1;
            if_node_t *conditional = (if_node_t *) node;
            if (!compile_ast(conditional->condition->left)) {
                return false;
            }
            printf("pushq %%rdi\n");
            if (!compile_ast(conditional->condition->right)) {
                return false;
            }
            printf("popq %%rax\n");
            printf("cmpq %%rdi, %%rax\n");
            switch (conditional->condition->op) {
                case ('='): {
                    printf("je IF_LABEL%ld\n", local);
                    break;
                }
                case ('>'): {
                    printf("jg IF_LABEL%ld\n", local);
                    break;
                }
                case ('<'): {
                    printf("jl IF_LABEL%ld\n", local);
                    break;
                }
            }
            compile_ast(conditional->else_branch);
            printf("jmp CODE_LABEL%ld\n", local);
            printf("IF_LABEL%ld:\n", local);
            compile_ast(conditional->if_branch);
            printf("CODE_LABEL%ld:\n", local);
            return true;
        }
        case (WHILE): {
            break;
        }
    }
    return false; // for now, every statement causes a compilation failure
}
