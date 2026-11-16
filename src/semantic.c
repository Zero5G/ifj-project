/**
 * @file sematic.c
 *
 * @author Jakub Manoušek
 * @brief Implementace modulu pro semantickou analýzu
 *
 * @details
 */

#include "ast.h"
#include "dstr.h"
#include "symtable.h"
#include <stddef.h>
#include "semantic.h"

typedef struct sematic_ctx {
    ast_prog_t *prog;
    symtable *stab;
    ast_func_t *current_func;
    symbol *current_sym;
    bool saw_return;
} sematic_ctx;

static bool declare_funcs(sematic_ctx *ctx) {
    for (ast_func_t *func = ctx->prog->first; func != nullptr; func = func->next) {
        symbol *sym = nullptr;
        switch (symtable_add_func(ctx->stab, dstr_cstr(func->name), &sym)) {
            case SYM_OK:
                break;
            case SYM_EXISTS: // NOLINT
                // ERR
                continue;
            case SYM_NOMEM:
                // ERR
                return false;
        }
        sym->decl = func;
        sym->param_count = func->param_count;
        sym->ret_state = func->has_ret_type ? RET_EXPLICIT : RET_UNRESOLVED;
        sym->type = func->has_ret_type ? func->ret_type : TYPE_UNKNOWN;
    }
    return true;
}

static void analyze_block(sematic_ctx *ctx, ast_block_t *block);
static void analyze_expr(sematic_ctx *ctx, ast_expr_t *expr);
static void analyze_stmt(sematic_ctx *ctx, ast_stmt_t *stmt);
static void analyze_func(sematic_ctx *ctx, symbol *sym);

static void analyze_block(sematic_ctx *ctx, ast_block_t *block) {
    if (!symtable_enter_scope(ctx->stab)) {
        return;
    }

    for (ast_stmt_t *stmt = block->first; stmt != nullptr; stmt = stmt->next) {
        analyze_stmt(ctx, stmt);
    }

    symtable_leave_scope(ctx->stab);
}

// NOLINTNEXTLINE(readability-function-cognitive-complexity)
static void analyze_stmt(sematic_ctx *ctx, ast_stmt_t *stmt) {
    switch (stmt->kind) {
        case STMT_VAR_DEF: {
            ast_expr_t *init = stmt->as.var_def.init;
            if (init != nullptr) {
                analyze_expr(ctx, init);
            }

            symbol *sym = nullptr;
            switch (symtable_add_var(ctx->stab, dstr_cstr(stmt->as.var_def.name), &sym)) {
                case SYM_OK: break;
                case SYM_EXISTS: // NOLINT
                    return; // ERR
                    break;
                case SYM_NOMEM:
                    return; // ERR
                    break;
            }
            data_type type = stmt->as.var_def.type;
            if (type == TYPE_UNKNOWN && init != nullptr) {
                type = init->data_type;
            } else if (init != nullptr && init->data_type != TYPE_UNKNOWN && type != init->data_type) {
                // ERR
            }

            sym->kind = SYM_VAR;
            sym->is_mut = stmt->as.var_def.is_mut;
            sym->type = type;
            stmt->as.var_def.type = type;
            stmt->as.var_def.var_id = sym->var_id;
            break;
        }
        case STMT_EXPR:
            analyze_expr(ctx, stmt->as.expr);
            break;
        case STMT_RET: {
            ast_expr_t *ret_val = stmt->as.ret_val;
            symbol *func = ctx->current_sym;

            if (ret_val != nullptr) {
                analyze_expr(ctx, ret_val);
            }

            data_type type = ret_val != nullptr ? ret_val->data_type : TYPE_UNIT;
            ctx->saw_return = true;

            if (type == TYPE_UNKNOWN) {
                break;
            }

            if (func->type == TYPE_UNKNOWN) {
                func->type = type;
            } else if (func->type != type) {
                // ERR
            }
            break;
        }
        case STMT_ASSIGN: {
            ast_expr_t *val = stmt->as.assign.val;
            analyze_expr(ctx, val);

            symbol *sym = symtable_find_var(ctx->stab, dstr_cstr(stmt->as.assign.name));
            if (sym == nullptr) {
                return; // ERR
            }
            if (!sym->is_mut) {
                return;; // ERR
            }

            if (sym->type != TYPE_UNKNOWN && val->data_type != TYPE_UNKNOWN && sym->type != val->data_type) {
                return; // ERR
            }

            stmt->as.assign.var_id = sym->var_id;
            break;
        }
        case STMT_IF:{
            ast_expr_t *cond = stmt->as.if_stmt.cond;
            analyze_expr(ctx, cond);

            if ((cond->data_type != TYPE_UNKNOWN) && (cond->data_type != TYPE_BOOL)) {
                // ERR
            }

            analyze_block(ctx, &stmt->as.if_stmt.then_blk);
            if (stmt->as.if_stmt.has_else) {
                analyze_block(ctx, &stmt->as.if_stmt.else_blk);
            }
            break;
        }
        case STMT_WHILE:{
            ast_expr_t *cond = stmt->as.while_stmt.cond;
            analyze_expr(ctx, cond);
            if (cond->data_type != TYPE_UNKNOWN && cond->data_type != TYPE_BOOL) {
                // ERR
            }
            analyze_block(ctx, &stmt->as.while_stmt.block);
            break;
        }
    }
}

static inline bool is_num(data_type type) {
    return (bool)(type == TYPE_DOUBLE || type == TYPE_INT);
}

// NOLINTNEXTLINE(readability-function-cognitive-complexity)
static void analyze_expr(sematic_ctx *ctx, ast_expr_t *expr) {
    if (expr == nullptr) {
        return;
    }
    switch (expr->kind) {
        case E_UNARY:
            ast_expr_t *operand = expr->as.unary.operand;
            analyze_expr(ctx, operand);
            expr->data_type = TYPE_UNKNOWN;
            switch (expr->as.unary.op) {
                case OPU_NEG:
                case OPU_PLUS:
                    if (!is_num(operand->data_type)) {
                        // ERR
                        break;
                    }
                    expr->data_type = operand->data_type;
                    break;
                case OPU_NOT:
                    if (operand->data_type != TYPE_BOOL) {
                        // ERR
                        break;
                    }
                    expr->data_type = TYPE_BOOL;
                    break;
            }
            break;
        case E_BINARY:{
                ast_expr_t *left = expr->as.binary.left;
                ast_expr_t *right = expr->as.binary.right;
                analyze_expr(ctx, left);
                analyze_expr(ctx, right);

                expr->data_type = TYPE_UNKNOWN;
                if (left->data_type == TYPE_UNKNOWN || right->data_type == TYPE_UNKNOWN) {
                    break;
                }

                if (left->data_type != right->data_type) {
                    // ERR
                    break;
                }

                data_type type = left->data_type;
                switch (expr->as.binary.op) {
                    case OPB_ADD:
                    case OPB_SUB:
                    case OPB_MUL:
                    case OPB_DIV:
                        if (!is_num(type)) {
                            // ERR
                            break;
                        }
                        expr->data_type = type;
                        break;
                    case OPB_AND:
                    case OPB_OR:
                        if (type != TYPE_BOOL) {
                            // ERR
                            break;
                        }
                        expr->data_type = TYPE_BOOL;
                        break;
                    case OPB_LT:
                    case OPB_LTE:
                    case OPB_GT:
                    case OPB_GTE:
                        if (!is_num(type)) {
                            // ERR
                            break;
                        }
                        expr->data_type = TYPE_BOOL;
                        break;
                    case OPB_EQ:
                    case OPB_NE:
                        expr->data_type = TYPE_BOOL;
                    break;
                }
            }
            break;
        case E_FUNC:{
                for (size_t i = 0; i < expr->as.func.argc; i++) {
                    analyze_expr(ctx, expr->as.func.args[i]);
                }
                expr->data_type = TYPE_UNKNOWN;

                symbol *callee = symtable_find_func(ctx->stab, dstr_cstr(expr->as.func.name));
                if (callee == nullptr) {
                    // ERR
                    break;
                }
                if (callee->ret_state == RET_UNRESOLVED && callee->decl != nullptr) {
                    analyze_func(ctx, callee);
                }
                if (callee->ret_state == RET_INFERRING && callee->type == TYPE_UNKNOWN) {
                    // ERR
                } else {
                    expr->data_type = callee->type;
                }

                expr->as.func.target = callee->decl;

                if (expr->as.func.argc != callee->param_count) {
                    // ERR
                    break;
                }
                for (size_t i = 0; i < expr->as.func.argc; i++) {
                    data_type wanted = callee->is_builtin ? callee->param_types[i] : callee->decl->params[i].type;
                    data_type gotten = expr->as.func.args[i]->data_type;
                    if (gotten != TYPE_UNKNOWN && wanted != TYPE_UNKNOWN && gotten != wanted) {
                        // ERR
                    }
                }
            }
            break;
        case E_VAR:
            symbol *sym = symtable_find_var(ctx->stab, dstr_cstr(expr->as.var.name));
            if (sym == nullptr) {
                // ERR?
                expr->data_type = TYPE_UNKNOWN;
                return;
            }

            expr->data_type = sym->type;
            expr->as.var.var_id = sym->var_id;
            break;
        case E_UNIT_LIT:
            expr->data_type = TYPE_UNIT;
            break;
        case E_INT_LIT:
            expr->data_type = TYPE_INT;
            break;
        case E_DOUBLE_LIT:
            expr->data_type = TYPE_DOUBLE;
            break;
        case E_STRING_LIT:
            expr->data_type = TYPE_STRING;
            break;
        case E_BOOLEAN_LIT:
            expr->data_type = TYPE_BOOL;
            break;
    }
}

static void analyze_func(sematic_ctx *ctx, symbol *sym) {
    ast_func_t *func = sym->decl;
    ast_func_t *saved_func = ctx->current_func;
    symbol *saved_sym = ctx->current_sym;
    bool saved_seen = ctx->saw_return;

    ctx->current_func = func;
    ctx->current_sym = sym;
    ctx->saw_return = false;

    if (sym->ret_state == RET_UNRESOLVED) {
        sym->ret_state = RET_INFERRING;
    }

    if (symtable_enter_func(ctx->stab)) {
        for (size_t i = 0; i < func->param_count; i++) {
            symbol *param = nullptr;
            if (symtable_add_var(ctx->stab, dstr_cstr(func->params[i].name), &param) == SYM_OK) {
                param->kind = SYM_VAR;
                param->is_mut = false;
                param->type = func->params[i].type;
            } else {
                // ERR
            }
        }
        analyze_block(ctx, &func->body);
        func->var_count = symtable_leave_func(ctx->stab);
    } else {
        // ERR
    }

    if (sym->ret_state == RET_INFERRING) {
        if (sym->type == TYPE_UNKNOWN && !ctx->saw_return) {
            sym->type = TYPE_UNIT;
        }
        sym->ret_state = RET_INFERRED;
    }

    ctx->current_func = saved_func;
    ctx->current_sym = saved_sym;
    ctx->saw_return = saved_seen;
}


void semantic_analyze(ast_prog_t *prog, symtable *stab) {
    if (prog == nullptr || stab == nullptr) {
        return;
    }

    sematic_ctx ctx = {
        .prog = prog,
        .stab = stab,
    };

    if (!declare_funcs(&ctx)) {
        return;
    }

    for (ast_func_t *func = prog->first; func != nullptr; func = func->next) {
        symbol *sym = symtable_find_func(stab, dstr_cstr(func->name));
        if (sym == nullptr || sym->decl != func) {
            continue;
        }
        if (sym->ret_state == RET_INFERRED) {
            continue;
        }
        analyze_func(&ctx, sym);
    }
}
