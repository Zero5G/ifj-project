/**
 * @file sematic.c
 *
 * @author Jakub Manoušek
 * @brief Implementace modulu pro semantickou analýzu
 *
 * @details
 */

#include "semantic.h"
#include "ast.h"
#include "dstr.h"
#include "error.h"
#include "symtable.h"

#include <stddef.h>

typedef struct builtin_t {
    const char *name;
    data_type ret;
    const data_type *params;
    size_t param_count;
} builtin_t;

static const data_type one_any_p[] = {TYPE_ANY};
static const data_type one_int_p[] = {TYPE_INT};
static const data_type one_double_p[] = {TYPE_DOUBLE};
static const data_type one_str_p[] = {TYPE_STRING};
static const data_type substr_p[] = {TYPE_STRING, TYPE_INT, TYPE_INT};
static const data_type ord_p[] = {TYPE_STRING, TYPE_INT};

static const builtin_t builtins[] = {
    {"print", TYPE_UNIT, one_any_p, 1},
    {"readLine", TYPE_STRING, nullptr, 0},
    {"readInt", TYPE_INT, nullptr, 0},
    {"readDouble", TYPE_DOUBLE, nullptr, 0},
    {"readBoolean", TYPE_BOOL, nullptr, 0},
    {"toStr", TYPE_STRING, one_any_p, 1},
    {"toInt", TYPE_INT, one_double_p, 1},
    {"strLength", TYPE_INT, one_str_p, 1},
    {"subStr", TYPE_STRING, substr_p, 3},
    {"ord", TYPE_INT, ord_p, 2},
    {"chr", TYPE_STRING, one_int_p, 1},
};

typedef struct sematic_ctx {
    ast_prog_t *prog;
    symtable *stab;
    ast_func_t *current_func;
    symbol *current_sym;
    bool saw_return;
} sematic_ctx;

static void declare_builtins(sematic_ctx *ctx) {
    for (size_t i = 0; i < sizeof(builtins) / sizeof(builtins[0]); i++) {
        const builtin_t *buildin = &builtins[i];
        symbol *sym = nullptr;
        if (symtable_add_func(ctx->stab, buildin->name, &sym) != SYM_OK) {
            error_fatal_cleanup(ERR_OTHER);
        }

        sym->is_builtin = true;
        sym->decl = nullptr;
        sym->type = buildin->ret;
        sym->ret_state = RET_EXPLICIT;
        sym->param_types = buildin->params;
        sym->param_count = buildin->param_count;
    }
}

static bool declare_funcs(sematic_ctx *ctx) {
    for (ast_func_t *func = ctx->prog->first; func != nullptr;
         func = func->next) {
        symbol *sym = nullptr;
        switch (symtable_add_func(ctx->stab, dstr_cstr(func->name), &sym)) {
        case SYM_OK:
            break;
        case SYM_EXISTS: // NOLINT
            error_fatal_cleanup(ERR_SEM_REDEF);
            continue;
        case SYM_NOMEM:
            error_fatal_cleanup(ERR_OTHER);
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
static bool try_coerce_type(ast_expr_t **expr, data_type wanted);

static void analyze_block(sematic_ctx *ctx, ast_block_t *block) {
    if (!symtable_enter_scope(ctx->stab)) {
        error_fatal_cleanup(ERR_OTHER);
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
        switch (symtable_add_var(ctx->stab, dstr_cstr(stmt->as.var_def.name),
                                 &sym)) {
        case SYM_OK:
            break;
        case SYM_EXISTS:
            error_fatal_cleanup(ERR_SEM_REDEF);
        case SYM_NOMEM:
            error_fatal_cleanup(ERR_OTHER);
        }
        data_type type = stmt->as.var_def.type;
        if (type == TYPE_UNKNOWN && init != nullptr) {
            type = init->data_type;
        } else if (init != nullptr &&
                   !try_coerce_type(&stmt->as.var_def.init, type)) {
            error_fatal_cleanup(ERR_SEM_TYPE);
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

        if (ret_val != nullptr) {
            analyze_expr(ctx, ret_val);
        }

        ctx->saw_return = true;

        if (!ctx->current_func->has_ret_type) {
            error_fatal_cleanup(ERR_SEM_TYPE);
        }
        data_type wanted = ctx->current_func->ret_type;
        if (wanted == TYPE_UNIT) {
            break;
        }
        if (ret_val == nullptr || !try_coerce_type(&stmt->as.ret_val, wanted)) {
            error_fatal_cleanup(ERR_SEM_TYPE);
        }
        break;
    }
    case STMT_ASSIGN: {
        ast_expr_t *val = stmt->as.assign.val;
        analyze_expr(ctx, val);

        symbol *sym =
            symtable_find_var(ctx->stab, dstr_cstr(stmt->as.assign.name));
        if (sym == nullptr) {
            error_fatal_cleanup(ERR_SEM_UNDEF);
        }
        if (!sym->is_mut) {
            error_fatal_cleanup(ERR_SEM_ROAE);
        }

        if (!try_coerce_type(&stmt->as.assign.val, sym->type)) {
            error_fatal_cleanup(ERR_SEM_TYPE);
        }

        stmt->as.assign.var_id = sym->var_id;
        break;
    }
    case STMT_IF: {
        ast_expr_t *cond = stmt->as.if_stmt.cond;
        analyze_expr(ctx, cond);

        if ((cond->data_type != TYPE_UNKNOWN) &&
            (cond->data_type != TYPE_BOOL)) {
            error_fatal_cleanup(ERR_SEM_TYPE);
        }

        analyze_block(ctx, &stmt->as.if_stmt.then_blk);
        if (stmt->as.if_stmt.has_else) {
            analyze_block(ctx, &stmt->as.if_stmt.else_blk);
        }
        break;
    }
    case STMT_WHILE: {
        ast_expr_t *cond = stmt->as.while_stmt.cond;
        analyze_expr(ctx, cond);
        if (cond->data_type != TYPE_UNKNOWN && cond->data_type != TYPE_BOOL) {
            error_fatal_cleanup(ERR_SEM_TYPE);
        }
        analyze_block(ctx, &stmt->as.while_stmt.block);
        break;
    }
    }
}

static inline bool is_num(data_type type) {
    return (bool)(type == TYPE_DOUBLE || type == TYPE_INT);
}

/**
 * Převede Int výraz na Double. Literál se přepíše přímo, jiný výraz
 * (proměnná, volání, ...) se obalí uzlem OPU_INT2FLOAT.
 * Vrací výraz, který má nahradit původní (nebo nullptr při nedostatku paměti).
 */
static ast_expr_t *promote_int(ast_expr_t *expr) {
    if (expr->kind == E_INT_LIT) {
        expr->as.double_val = (double)expr->as.int_val;
        expr->kind = E_DOUBLE_LIT;
        expr->data_type = TYPE_DOUBLE;
        return expr;
    }
    ast_expr_t *conv = ast_expr_unary_create(expr->line, OPU_INT2FLOAT, expr);
    if (conv == nullptr) {
        return nullptr;
    }
    conv->data_type = TYPE_DOUBLE;
    return conv;
}

/** Zkusit přepsat typ na ten co chceme */
static bool try_coerce_type(ast_expr_t **expr, data_type wanted) {
    data_type got = (*expr)->data_type;
    if (got == TYPE_UNKNOWN || wanted == TYPE_UNKNOWN || wanted == TYPE_ANY ||
        got == wanted) {
        return true;
    }
    if (wanted == TYPE_DOUBLE && got == TYPE_INT) {
        ast_expr_t *promoted = promote_int(*expr);
        if (promoted == nullptr) {
            error_fatal_cleanup(ERR_OTHER);
        }
        *expr = promoted;
        return true;
    }
    return false;
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
        if (operand->data_type == TYPE_UNKNOWN) {
            break;
        }
        switch (expr->as.unary.op) {
        case OPU_NEG:
        case OPU_PLUS:
            if (!is_num(operand->data_type)) {
                error_fatal_cleanup(ERR_SEM_TYPE);
                break;
            }
            expr->data_type = operand->data_type;
            break;
        case OPU_NOT:
            if (operand->data_type != TYPE_BOOL) {
                error_fatal_cleanup(ERR_SEM_TYPE);
            }
            expr->data_type = TYPE_BOOL;
            break;
        case OPU_INT2FLOAT:
            expr->data_type = TYPE_DOUBLE;
            break;
        }
        break;
    case E_BINARY: {
        ast_expr_t *left = expr->as.binary.left;
        ast_expr_t *right = expr->as.binary.right;
        analyze_expr(ctx, left);
        analyze_expr(ctx, right);

        expr->data_type = TYPE_UNKNOWN;
        if (left->data_type == TYPE_UNKNOWN ||
            right->data_type == TYPE_UNKNOWN) {
            break;
        }

        bool numeric_op =
            (bool)(expr->as.binary.op != OPB_AND &&
                   expr->as.binary.op !=
                       OPB_OR); /**< Int na Double pokud jeden je Double */
        if (numeric_op && left->data_type == TYPE_DOUBLE &&
            right->data_type == TYPE_INT) {
            right = promote_int(right);
            if (right == nullptr) {
                error_fatal_cleanup(ERR_OTHER);
            }
            expr->as.binary.right = right;
        } else if (numeric_op && left->data_type == TYPE_INT &&
                   right->data_type == TYPE_DOUBLE) {
            left = promote_int(left);
            if (left == nullptr) {
                error_fatal_cleanup(ERR_OTHER);
            }
            expr->as.binary.left = left;
        }

        if (left->data_type != right->data_type) {
            error_fatal_cleanup(ERR_SEM_TYPE);
            break;
        }

        data_type type = left->data_type;
        switch (expr->as.binary.op) {
        case OPB_ADD:
            /**< + je i konkatenace retezcu */
            if (!is_num(type) && type != TYPE_STRING) {
                error_fatal_cleanup(ERR_SEM_TYPE);
            }
            expr->data_type = type;
            break;
        case OPB_SUB:
        case OPB_MUL:
        case OPB_DIV:
            if (!is_num(type)) {
                error_fatal_cleanup(ERR_SEM_TYPE);
            }
            expr->data_type = type;
            break;
        case OPB_AND:
        case OPB_OR:
            if (type != TYPE_BOOL) {
                error_fatal_cleanup(ERR_SEM_TYPE);
            }
            expr->data_type = TYPE_BOOL;
            break;
        case OPB_LT:
        case OPB_LTE:
        case OPB_GT:
        case OPB_GTE:
            if (!is_num(type) && type != TYPE_STRING) {
                error_fatal_cleanup(ERR_SEM_TYPE);
            }
            expr->data_type = TYPE_BOOL;
            break;
        case OPB_EQ:
        case OPB_NE:
            expr->data_type = TYPE_BOOL;
            break;
        }
    } break;
    case E_FUNC: {
        for (size_t i = 0; i < expr->as.func.argc; i++) {
            analyze_expr(ctx, expr->as.func.args[i]);
        }
        expr->data_type = TYPE_UNKNOWN;

        symbol *callee =
            symtable_find_func(ctx->stab, dstr_cstr(expr->as.func.name));
        if (callee == nullptr) {
            error_fatalf_cleanup(ERR_SEM_UNDEF, "Funckce '%s' nenalezena.",
                                 dstr_cstr(expr->as.func.name));
        }
        if (callee->ret_state == RET_UNRESOLVED && callee->decl != nullptr) {
            analyze_func(ctx, callee);
        }
        if (callee->ret_state == RET_INFERRING &&
            callee->type == TYPE_UNKNOWN) {
            error_fatal_cleanup(ERR_SEM_TYPE);
        }
        /* else {
            expr->data_type = callee->type;
        } */
        expr->data_type = callee->type;

        expr->as.func.target = callee->decl;

        if (expr->as.func.argc != callee->param_count) {
            error_fatal_cleanup(ERR_SEM_BADARGS);
            break;
        }
        for (size_t i = 0; i < expr->as.func.argc; i++) {
            data_type wanted = callee->is_builtin
                                   ? callee->param_types[i]
                                   : callee->decl->params[i].type;
            if (!try_coerce_type(&expr->as.func.args[i], wanted)) {
                error_fatal_cleanup(ERR_SEM_BADARGS);
            }
        }
    } break;
    case E_VAR:
        symbol *sym =
            symtable_find_var(ctx->stab, dstr_cstr(expr->as.var.name));
        if (sym == nullptr) {
            error_fatal_cleanup(ERR_SEM_UNDEF);
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

static data_type infer_fallthrough_type(ast_block_t *block) {
    ast_stmt_t *last = block->last;
    if (last == nullptr) {
        return TYPE_UNIT;
    }
    switch (last->kind) {
    case STMT_EXPR:
        return last->as.expr->data_type;
    case STMT_IF: {
        /* U if všechny větve musí bý tUnit  */
        data_type then_t = infer_fallthrough_type(&last->as.if_stmt.then_blk);
        data_type else_t =
            last->as.if_stmt.has_else
                ? infer_fallthrough_type(&last->as.if_stmt.else_blk)
                : TYPE_UNIT;
        if ((then_t != TYPE_UNIT && then_t != TYPE_UNKNOWN) ||
            (else_t != TYPE_UNIT && else_t != TYPE_UNKNOWN)) {
            error_fatal_cleanup(ERR_SEM_TYPE);
        }
        return TYPE_UNIT;
    }
    default:
        return TYPE_UNIT;
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

    if (!symtable_enter_func(ctx->stab)) {
        error_fatal_cleanup(ERR_OTHER);
    }

    for (size_t i = 0; i < func->param_count; i++) {
        symbol *param = nullptr;
        switch (symtable_add_var(ctx->stab, dstr_cstr(func->params[i].name),
                                 &param)) {

        case SYM_OK:
            param->kind = SYM_VAR;
            param->is_mut = false;
            param->type = func->params[i].type;
            func->params[i].var_id = param->var_id;
            break;
        case SYM_EXISTS:
            error_fatal_cleanup(ERR_SEM_REDEF);
        case SYM_NOMEM:
            error_fatal_cleanup(ERR_OTHER);
        }
    }
    analyze_block(ctx, &func->body);
    func->var_count = symtable_leave_func(ctx->stab);

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

    declare_builtins(&ctx);

    if (!declare_funcs(&ctx)) {
        return;
    }

    ast_func_t *main_func = nullptr;
    unsigned main_count = 0;
    for (ast_func_t *func = prog->first; func != nullptr; func = func->next) {
        if (func->is_main) {
            main_func = func;
            main_count++;
        }
    }
    if (main_count == 0) {
        error_fatal_cleanup(ERR_SEM_UNDEF);
    }
    if (main_count > 1) {
        error_fatal_cleanup(ERR_SEM_REDEF);
    }
    if (main_func->param_count != 0) {
        error_fatal_cleanup(ERR_SEM_BADARGS);
    }
    if (main_func->has_ret_type && main_func->ret_type != TYPE_UNIT) {
        error_fatal_cleanup(ERR_SEM_TYPE);
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
