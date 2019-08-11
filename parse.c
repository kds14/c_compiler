#include "comp.h"

/*
 * GRAMMAR USED
 * Expr ::= Term Expr'
 * Expr' ::= + Term Expr' | - Term Expr' | ϵ
 * Term ::= + Factor Term'
 * Term' ::= * Factor Term' | / Factor Term' | ϵ
 * Factor ::= (Expr) | NUM 
 */

enum parse_exp {P_NON = 0x0, P_IDENT = 0x1};

struct parse_ctx {
	struct vector *tokens;
};

struct ast_node *expr(struct parse_ctx *ctx);

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
	print_ast(ast->left, indent + 8);
	for (int i = 0; i < indent; ++i)
		printf(" ");
	if (ast->type == AST_INT)
		printf("%d\n", ast->int_val);
	else if (ast->type == AST_OP)
		printf("%c\n", ast->char_val);
	print_ast(ast->right, indent + 8);
}

void err_abort(struct parse_ctx *ctx) {
	struct token_node* node = vector_peek(ctx->tokens);
	printf("Parse error next token: ");
	print_token(node);
	printf("\n");
	exit(0);
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
	err_abort(ctx);
	return 1;
}

struct ast_node *factor(struct parse_ctx *ctx) {
	struct token_node *node = vector_peek(ctx->tokens);
	struct ast_node *res = NULL;
	if (node->type == TK_LPAREN) {
		if (consume(ctx, TK_LPAREN))
			err_abort(ctx);
		res = expr(ctx);
		if (consume(ctx, TK_RPAREN))
			err_abort(ctx);
	} else if (!consume(ctx, TK_INT)) {
		return make_ast_node(node, NULL, NULL);
	}
	return res;
};

struct ast_node *term_prime(struct parse_ctx *ctx,
		struct ast_node *left) {
	struct ast_node *f, *tp, *res = left;
	struct token_node *next = vector_peek(ctx->tokens);
	while (next && next->type == TK_OP &&
			(next->char_val == '*' || next->char_val == '/') && !consume(ctx, TK_OP))
	{
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
	while (next && next->type == TK_OP
			&& (next->char_val == '+' || next->char_val == '-') && !consume(ctx, TK_OP))
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

int parse(struct context *ctx) {
	vector_reset(ctx->tokens);
	struct token_node *node = NULL;
	struct parse_ctx *parse_ctx = calloc(1, sizeof(parse_ctx));
	parse_ctx->tokens = ctx->tokens;
	printf("\n\n");
	struct ast_node *root = expr(parse_ctx);
#ifdef PAR_DBG
	printf("\n\n");
	print_ast(root, 0);
#endif
	ctx->ast_root = root;
	return 0;
}
