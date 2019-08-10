#include "comp.h"
#define RCX "%rcx"
#define RAX "%rax"

struct out_ctx {
	FILE* str;
	struct ast_node *last;
};

void output_op(struct out_ctx *ctx, struct ast_node *node);
void output_lit(struct out_ctx *ctx, struct ast_node *node);
void output_expr(struct out_ctx *ctx, struct ast_node *node);

void out2str(struct out_ctx *ctx, const char* string) {
	fprintf(ctx->str, "%s", string);
}

void push(struct out_ctx* ctx) {
	fprintf(ctx->str, "%s", "pushq\t%rax\n");
}

void pop(struct out_ctx* ctx, char* reg) {
	fprintf(ctx->str, "popq\t%s\n", reg);
}

void out_err(struct out_ctx *ctx) {
	printf("Output error\n");
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
		default:
			out_err(ctx);
			break;
	}
	push(ctx);
}

int out(struct context *ctx) {
	struct out_ctx *out_ctx = calloc(1, sizeof(struct out_ctx));
	out_ctx->str = stdout;
	output_expr(out_ctx, ctx->ast_root);
}
