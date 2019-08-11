#include "comp.h"

void ht_init(struct hashtable **ht, size_t buckets, float bound) {
	struct hashtable *h = calloc(1, sizeof(struct hashtable));
	h->eles = calloc(buckets, sizeof(struct ht_node *));
	h->buckets = buckets;
	h->bound = bound;
	h->count = 0;
	h->next = NULL;
	h->it_bucket = 0;
	*ht = h;
}

struct ht_node* ht_next(struct hashtable *ht) {
	struct ht_node* res;
	while (ht->it_bucket < ht->buckets) {
		res = ht->next;
		if (res != NULL) {
			ht->next = res->next;
			return res;
		} else {
			ht->next = ht->eles[++ht->it_bucket];
		}
	}
	return NULL;
}

void ht_reset(struct hashtable *ht) {
	ht->next = ht->eles[0];
	ht->it_bucket = 0;
}

void ht_destroy(struct hashtable *ht) {
	struct ht_node* node;
	struct ht_node* prev = NULL;
	while ((node = ht_next(ht)) != NULL) {
		if (prev != NULL) {
			free(prev);
		}
		prev = node;
	}
	if (prev != NULL) {
		free(prev);
	}
	free(ht->eles);
	free(ht);
}

uint8_t ht_hash(struct hashtable *ht, char* val) {
	// djb2 hash
	uint32_t hash = 5381;
	uint8_t c;
	while (c = *val++) {
		hash = ((hash << 5) + hash) + c;
	}
	return hash % ht->buckets;
}

void ht_insert(struct hashtable *ht, char* key, void* val) {
	// rehash if n/k > load factor threshold
	if (((float)ht->count + 1) / ht->buckets > ht->bound) {
		struct ht_node** old_eles  = ht->eles;
		struct ht_node* node;
		struct hashtable *new_ht;
		
		ht_reset(ht);
		ht_init(&new_ht, ht->buckets * 2, ht->bound);
		new_ht->count = ht->count;
		while ((node = ht_next(ht)) != NULL) {
			ht_insert(new_ht, node->key, node->val);
			free(node);
		}
		memcpy(ht, new_ht, sizeof(struct hashtable));
		free(new_ht);
		free(old_eles);
	}

	uint8_t hash = ht_hash(ht, key);

	struct ht_node* new_node = calloc(1, sizeof(struct ht_node));
	new_node->val = val;
	new_node->key = key;
	new_node->next = ht->eles[hash];
	ht->eles[hash] = new_node;
	ht->count += 1;
}

void* ht_find(struct hashtable *ht, char* key) {
	struct ht_node *node = ht->eles[ht_hash(ht, key)];
	int it = 0;
	while (node != NULL) {
		if (!strcmp(node->key, key)) {
			return node->val;
		}
		node = node->next;
	}
	return NULL;
}
