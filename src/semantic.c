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
#include "htab.h"
#include "symtable.h"
#include "semantic.h"

typedef struct sematic_ctx {
    ast_prog_t *prog;
    symtable *stab;
    ast_func_t *currect_func;
} sematic_ctx;

static void analyze_block(sematic_ctx *ctx, ast_block_t *block);
static void analyze_expr(sematic_ctx *ctx, ast_expr_t *expr);
static void analyze_stmt(sematic_ctx *ctx, ast_stmt_t *stmt);

static void analyze_block(sematic_ctx *ctx, ast_block_t *block) {
    if (!symtable_enter_scope(ctx->stab)) {
        return;
    }

    for (ast_stmt_t *stmt = block->first; stmt != nullptr; stmt = stmt->next) {
        analyze_stmt(ctx, stmt);
    }

    symtable_leave_scope(ctx->stab);
}

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
            ast_func_t *func = ctx->currect_func;
            if (ret_val != nullptr) {
                analyze_expr(ctx, ret_val);
            }
            if (!func->has_ret_type) {
                if (ret_val != nullptr) {
                    // ERR
                }
            } else if (ret_val == nullptr) {
                // ERR
            } else if (ret_val->data_type != TYPE_UNKNOWN && ret_val->data_type != func->ret_type) {
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


void semantic_analyze(ast_prog_t *prog, symtable *stab) {
    if (prog == nullptr || stab == nullptr) {
        return;
    }

    sematic_ctx ctx = {
        .prog = prog,
        .stab = stab,
    };

    for (ast_func_t *func = prog->first; func != nullptr; func = func->next) {
        symbol *sym_ptr = nullptr;
        switch (symtable_add_func(ctx.stab, dstr_cstr(func->name), &sym_ptr)) {
            case SYM_OK:
                if (symtable_enter_func(ctx.stab)) {
                    ctx.currect_func = func;
                    analyze_block(&ctx, &func->body);
                    symtable_leave_func(ctx.stab);
                } else {
                    return; // ERR
                }
                break;
            case SYM_EXISTS: // NOLINT
                return; // ERR
                break;
            case SYM_NOMEM:
                return; // ERR
                break;
        }
    }
}
