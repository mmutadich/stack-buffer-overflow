#include "compile.h"

#include <stdio.h>
#include <stdlib.h>

int64_t COUNT = 0;
int64_t power_of_two(int64_t num) {
    int64_t pow = 0;
    while (num > 1 && (num % 2 == 0)) {
        num = num / 2;
        pow += 1;
    }
    if (num == 1) {
        return pow;
    }
    else {
        return 0;
    }
}

bool is_compress(node_t *node) {
    if (node->type != NUM && node->type != BINARY_OP) {
        return false;
    }
    else if (node->type == NUM) {
        return true;
    }
    else if (node->type == BINARY_OP) {
        bool left = is_compress(((binary_node_t *) node)->left);
        bool right = is_compress(((binary_node_t *) node)->right);
        if (left != true || right != true) {
            return false;
        }
        else {
            return true;
        }
    }
    return false;
}

value_t compress(node_t *node) {
    if (node->type == NUM) {
        return ((num_node_t *) node)->value;
    }
    else if (node->type == BINARY_OP) {
        value_t left = compress(((binary_node_t *) node)->left);
        value_t right = compress(((binary_node_t *) node)->right);
        switch (((binary_node_t *) node)->op) {
            case ('+'): {
                return left + right;
                break;
            }
            case ('-'): {
                return left - right;
                break;
            }
            case ('*'): {
                return left * right;
                break;
            }
            case ('/'): {
                return left / right;
                break;
            }
        }
    }
    return 0;
}

bool compile_ast(node_t *node) {
    if (node == NULL) {
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
            if (is_compress(node)) {
                value_t val = compress(node);
                printf("movq $%ld, %%rdi\n", val);
                return true;
            }
            binary_node_t *bin = (binary_node_t *) node;
            switch (bin->op) {
                case ('/'): {
                    if (!compile_ast(bin->left)) {
                        return false;
                    }
                    printf("pushq %%rdi\n");
                    if (!compile_ast(bin->right)) {
                        return false;
                    }
                    printf("popq %%rax\n");
                    printf("cqto\n");
                    printf("idivq %%rdi\n");
                    printf("movq %%rax, %%rdi\n");
                    return true;
                }
                case ('*'): {
                    int64_t k;
                    if (bin->right->type == NUM && bin->left->type == NUM) {
                        int64_t result = (((num_node_t *) bin->right)->value) *
                                         (((num_node_t *) bin->left)->value);
                        printf("movq $%ld, %%rdi\n", result);
                        return true;
                    }
                    else if (bin->right->type == NUM) {
                        k = ((num_node_t *) bin->right)->value;
                        int64_t pow = power_of_two(k);
                        if (pow > 0) {
                            compile_ast(bin->left);
                            printf("shl $%ld, %%rdi\n", pow);
                            return true;
                        }
                    }
                    else if (bin->left->type == NUM) {
                        k = ((num_node_t *) bin->left)->value;
                        int64_t pow = power_of_two(k);
                        if (pow > 0) {
                            compile_ast(bin->right);
                            printf("shl $%ld, %%rdi\n", pow);
                            return true;
                        }
                    }
                    compile_ast(bin->left);
                    printf("pushq %%rdi\n");
                    compile_ast(bin->right);
                    printf("popq %%rax\n");
                    printf("imulq %%rax, %%rdi\n");
                    return true;
                }
                case ('-'): {
                    if (!compile_ast(bin->right)) {
                        return false;
                    }
                    printf("pushq %%rdi\n");
                    if (!compile_ast(bin->left)) {
                        return false;
                    }
                    printf("popq %%rax\n");
                    printf("subq %%rax, %%rdi\n");
                    return true;
                }
                case ('+'): {
                    if (!compile_ast(bin->left)) {
                        return false;
                    }
                    printf("pushq %%rdi\n");
                    if (!compile_ast(bin->right)) {
                        return false;
                    }
                    printf("popq %%rax\n");
                    printf("addq %%rax, %%rdi\n");
                    return true;
                }
            }
        }
        case (VAR): {
            var_node_t *var_node = (var_node_t *) node;
            var_name_t name = var_node->name;
            int64_t sub = (name - 'A' + 1) * -8;
            printf("movq %ld(%%rbp), %%rdi\n", sub);
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
            int64_t local = COUNT;
            COUNT += 1;
            while_node_t *conditional = (while_node_t *) node;
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
                    printf("jne END_WHILE_LABEL%ld\n", local);
                    break;
                }
                case ('>'): {
                    printf("jle END_WHILE_LABEL%ld\n", local);
                    break;
                }
                case ('<'): {
                    printf("jge END_WHILE_LABEL%ld\n", local);
                    break;
                }
            }
            printf("WHILE_LABEL%ld:\n", local);
            compile_ast(conditional->body);
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
                    printf("je WHILE_LABEL%ld\n", local);
                    break;
                }
                case ('>'): {
                    printf("jg WHILE_LABEL%ld\n", local);
                    break;
                }
                case ('<'): {
                    printf("jl WHILE_LABEL%ld\n", local);
                    break;
                }
            }
            printf("END_WHILE_LABEL%ld:\n", local);
            return true;
        }
    }
    return false; // for now, every statement causes a compilation failure
}