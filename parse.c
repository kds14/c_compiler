#include "comp.h"
// TODO : STACK POSITION
/*
 * GRAMMAR USED
 * Expr ::= Term Expr'
 * Expr' ::= + Term Expr' | - Term Expr' | ϵ
 * Term ::= + Factor Term'
 * Term' ::= * Factor Term' | / Factor Term' | ϵ
 * Factor ::= (Expr) | NUM 
 */


enum parse_exp {P_NON = 0x0, P_IDENT = 0x1};

enum parse_err {PE_NON = 0, PE_DUPE_VAR, PE_CONS, PE_VARB4ASS};

struct parse_ctx {
	struct vector *tokens;
	char **types;
	size_t types_size;
	struct hashtable *syms;
	enum parse_err err;
	char* err_ex;
};

struct ast_node *expr(struct parse_ctx *ctx);
enum var_type get_type(struct parse_ctx* ctx, struct token_node *node);

int get_ast_height(struct ast_node *ast) {
	if (ast == NULL)
		return -1;
	int lh = get_ast_height(ast->left);
	int rh = get_ast_height(ast->right);
	return lh > rh ? lh + 1 : rh + 1;
}

void print_ast(struct ast_node *ast, int indent) {
	if (ast == NULL)
		return;
	print_ast(ast->right, indent + 8);
	for (int i = 0; i < indent; ++i)
		printf(" ");
	if (ast->type == AST_INT)
		printf("%d\n", ast->int_val);
	else if (ast->type == AST_OP || ast->type == AST_ASS)
		printf("%c\n", ast->char_val);
	else if (ast->type == AST_VAR)
		printf("var:%s\n", ast->ident);
	print_ast(ast->left, indent + 8);
}

void err_abort(struct parse_ctx *ctx) {
	struct token_node* node = vector_peek(ctx->tokens);
	char* str = calloc(1000, sizeof(char));
	token_str(node, str);
	printf("Parse error next token: %s", str);
	free(str);
	switch (ctx->err) {
		case PE_DUPE_VAR:
			printf(" (Duplicate assignment) ");
			break;
		case PE_CONS:
			printf(" (Consume error) ");
			break;
		case PE_VARB4ASS:
			printf(" (Variable %s used before assignment) ", ctx->err_ex);
			break;
	}
	printf("\n");
	exit(0);
}

struct ast_node *make_ast_var_node(enum var_type type, char* ident) {
	struct ast_node* res = calloc(1, sizeof(struct ast_node));
	res->vtype = type;
	res->ident = ident;
	res->type = AST_VAR;
	res->left = NULL;
	res->right = NULL;
	return res;
}

struct ast_node *make_ast_node(struct token_node* node,
		struct ast_node *left, struct ast_node *right) {
	struct ast_node* res = calloc(1, sizeof(struct ast_node));
	if (node->type == TK_INT) {
		res->type = AST_INT;
		res->int_val = node->int_val;
	} else if (node->type == TK_OP) {
		res->type = AST_OP;
		res->char_val = node->char_val;
	} else if (node->type == TK_ASS) {
		res->type = AST_ASS;
		res->char_val = node->char_val;
	}
	res->left = left;
	res->right = right;
	return res;
}

int consume(struct parse_ctx *ctx, uint64_t val) {
	struct token_node* node = vector_peek(ctx->tokens);
	if (node == NULL)
		return -1;
	if (node->type & val) {
		vector_next(ctx->tokens);
		return 0;
	}
	ctx->err = PE_CONS;
	err_abort(ctx);
	return 1;
}


struct ast_node *factor(struct parse_ctx *ctx) {
	struct token_node *node = vector_peek(ctx->tokens);
	struct ast_node *res = NULL;
	struct sym_ent *se;
	
	if (node->type == TK_LPAREN) {
		if (consume(ctx, TK_LPAREN))
			err_abort(ctx);
		res = expr(ctx);
		if (consume(ctx, TK_RPAREN))
			err_abort(ctx);
	} else if (node->type == TK_TEXT) {
		consume(ctx, TK_TEXT);
		char* name = node->str_val;
		if ((se = ht_find(ctx->syms, name)) == NULL) {
			ctx->err = PE_VARB4ASS;
			ctx->err_ex = name;
			err_abort(ctx);
		} else {
			return make_ast_var_node(se->type, se->name);
		}
	} else if (node->type == TK_INT) {
		consume(ctx, TK_INT);
		return make_ast_node(node, NULL, NULL);
	}
	return res;
};

struct ast_node *term_prime(struct parse_ctx *ctx,
		struct ast_node *left) {
	struct ast_node *f, *tp, *res = left;
	struct token_node *next = vector_peek(ctx->tokens);
	while (next && next->type == TK_OP
			&& (next->char_val == '*' || next->char_val == '/')
			&& !consume(ctx, TK_OP)) {
		f = factor(ctx);
		if (f == NULL)
			err_abort(ctx);
		res = make_ast_node(next, res, f);
		next = vector_peek(ctx->tokens);
	}
	return res;
}

struct ast_node *term(struct parse_ctx *ctx) {
	struct ast_node *f = factor(ctx);
	if (f == NULL)
		err_abort(ctx);
	struct ast_node *tp = term_prime(ctx, f);
	if (tp == NULL)
		return f;
	return tp;
}

struct ast_node *expr_prime(struct parse_ctx *ctx,
		struct ast_node *left) {
	struct ast_node *t, *ep, *res = left;
	struct token_node *next = vector_peek(ctx->tokens);
	while (next && next->type == TK_OP && (next->char_val == '+'
				|| next->char_val == '-') && !consume(ctx, TK_OP))
	{
		t = term(ctx);
		res = make_ast_node(next, res, t);
		next = vector_peek(ctx->tokens);
	}
	return res;
}

struct ast_node *expr(struct parse_ctx *ctx) {
	struct ast_node *f = term(ctx);
	if (f == NULL)
		err_abort(ctx);
	struct ast_node *ep = expr_prime(ctx, f);
	if (ep == NULL)
		return f;
	return ep;
}

enum var_type get_type(struct parse_ctx* ctx, struct token_node *node) {
	if (node->type != TK_TEXT)
		return 0;
	for (int i = 0; i < ctx->types_size; ++i) {
		if (!strcmp(ctx->types[i], node->str_val)) {
			return i + 1;
		}
	}
	return 0;
}

struct ast_node *line(struct parse_ctx *ctx) {
	struct token_node *next, *next2, *next3;
	struct ast_node *res, *var;
	next = vector_peek(ctx->tokens);
	res = NULL;
	if (next == NULL)
		return NULL;
	enum var_type type = get_type(ctx, next);
	if (type) {
		if (consume(ctx, TK_TEXT))
			err_abort(ctx);
		next2 = vector_peek(ctx->tokens);
		if (consume(ctx, TK_TEXT))
			err_abort(ctx);
		next3 = vector_peek(ctx->tokens);
		if (consume(ctx, TK_ASS))
			err_abort(ctx);
		char* name = next2->str_val;
		var = make_ast_var_node(type, name);
		if (ht_find(ctx->syms, name)) {
			ctx->err = PE_DUPE_VAR;
			err_abort(ctx);
		}
		struct sym_ent *se = calloc(1, sizeof(struct sym_ent));
		se->type = type;
		se->name = name;
		ht_insert(ctx->syms, name, se);
		res = make_ast_node(next3, var, expr(ctx));
	}
	if (consume(ctx, TK_SEMICOL))
		err_abort(ctx);
	return res;
}

int parse(struct context *ctx) {
	vector_reset(ctx->tokens);
	struct token_node *node = NULL;
	struct parse_ctx *parse_ctx = calloc(1, sizeof(parse_ctx));
	parse_ctx->types_size = 1;
	parse_ctx->types = calloc(parse_ctx->types_size, sizeof(char *));
	parse_ctx->types[0] = "int";

	ht_init(&parse_ctx->syms, 100, 0.75f);
	vector_init(&ctx->asts, sizeof(struct ast_node), 100);
	parse_ctx->tokens = ctx->tokens;
	struct ast_node *ptr;
	do {
		ptr = line(parse_ctx);
		if (ptr != NULL)
			vector_push_back(ctx->asts, ptr);
	} while (ptr != NULL);
	ctx->syms = parse_ctx->syms;
	return 0;
}
