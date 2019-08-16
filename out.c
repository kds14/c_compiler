#include "comp.h"
#define RCX "%rcx"
#define RAX "%rax"

enum out_err { OE_NON=0, OE_NIMP, OE_MISS_SYM, OE_ASS_GEN, OE_OP_GEN};

struct out_ctx {
	FILE* str;
	struct ast_node *last;
	enum out_err err;
	struct hashtable *syms;
	uint64_t stack;
};

void output_op(struct out_ctx *ctx, struct ast_node *node);
void output_lit(struct out_ctx *ctx, struct ast_node *node);
void output_expr(struct out_ctx *ctx, struct ast_node *node);

void out2str(struct out_ctx *ctx, const char* string) {
	fprintf(ctx->str, "%s", string);
}

void push(struct out_ctx* ctx) {
	fprintf(ctx->str, "%s", "pushq\t%rax\n");
	ctx->stack += 8;
}

void pop(struct out_ctx* ctx, char* reg) {
	fprintf(ctx->str, "popq\t%s\n", reg);
	ctx->stack -= 8;
}

void out_err(struct out_ctx *ctx) {
	char* str = calloc(100, sizeof(char));
	switch (ctx->err) {
		case OE_NIMP:
			sprintf(str, "Not implemented AST node type: %d", ctx->last->type);
			break;
		case OE_MISS_SYM:
			sprintf(str, "Missing symbol %s", ctx->last->ident);
			break;
		case OE_ASS_GEN:
			sprintf(str, "Assignment error");
			break;
		case OE_OP_GEN:
			sprintf(str, "OP error");
			break;
		case OE_NON:
		default:
			sprintf(str, "Unknown error type");
			break;
	}
	printf("Output error: %s\n", str);
	free(str);
	exit(0);
}

void output_lit(struct out_ctx *ctx, struct ast_node *node) {
	ctx->last = node;
	switch (node->type) {
		case AST_INT:
			fprintf(ctx->str, "movq\t$%d, %%rax\n", node->int_val);
			break;
		default:
			out_err(ctx);
			break;
	}
}

void output_var(struct out_ctx *ctx, struct ast_node *node) {
	struct sym_ent *se;
	ctx->last = node;
	if ((se = ht_find(ctx->syms, node->ident)) == NULL) {
		ctx->err = OE_MISS_SYM;
		ctx->last = node->left;
		out_err(ctx);
		return;
	}
	uint64_t diff = ctx->stack - se->stack;
	fprintf(ctx->str, "movq\t%lu(%%rsp), %%rax\n",diff);
}

void output_ass(struct out_ctx *ctx, struct ast_node *node) {
	output_expr(ctx, node->right);
	if (node->left->type = AST_VAR) {
		struct sym_ent *se = ht_find(ctx->syms, node->left->ident);
		if (se) {
			se->stack = ctx->stack;
		} else {
			ctx->err = OE_MISS_SYM;
			ctx->last = node->left;
			out_err(ctx);
		}
	} else {
		ctx->err = OE_ASS_GEN;
		out_err(ctx);
	}
}

void output_op(struct out_ctx *ctx, struct ast_node *node) {
	ctx->last = node;
	output_expr(ctx, node->left);
	output_expr(ctx, node->right);
	pop(ctx, RCX); // right
	pop(ctx, RAX); // left
	switch (node->char_val) {
		case '+':
			out2str(ctx, "addq\t%rcx, %rax\n");
			break;
		case '-':
			out2str(ctx, "subq\t%rcx, %rax\n");
			break;
		case '*':
			out2str(ctx, "imul\t%rcx, %rax\n");
			break;
		case '/':
			out2str(ctx, "cqto\nidivq\t\%rcx\n");
			break;
		default:
			ctx->err = OE_OP_GEN;
			out_err(ctx);
			break;
	}
}

void output_expr(struct out_ctx *ctx, struct ast_node *node) {
	ctx->last = node;
	switch (node->type) {
		case AST_INT:
			output_lit(ctx, node);
			break;
		case AST_OP:
			output_op(ctx, node);
			break;
		case AST_ASS:
			output_ass(ctx, node);
			break;
		case AST_VAR:
			output_var(ctx, node);
			break;
		default:
			ctx->err = OE_NIMP;
			out_err(ctx);
			break;
	}
	push(ctx);
}

int out(struct context *ctx) {
	struct out_ctx *out_ctx = calloc(1, sizeof(struct out_ctx));
	out_ctx->str = stdout;
	vector_reset(ctx->asts);
	struct ast_node* node;
	out_ctx->syms = ctx->syms;
	out_ctx->stack = 0;
	while ((node = vector_next(ctx->asts)) != NULL) {
		output_expr(out_ctx, node);
	}
}
