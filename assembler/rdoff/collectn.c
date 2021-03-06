/* collectn.c	Implements variable length pointer arrays [collections]
 *
 * This file is public domain.
 */

#include "collectn.h"
#include <stdlib.h>

void collection_init(Collection * c)
{
  int i;

  for (i = 0; i < 32; i++) c->p[i] = NULL;
  c->next = NULL;
}

void ** colln(Collection * c, int index)
{
  while (index >= 32) {
    index -= 32;
    if (c->next == NULL) {
      c->next = malloc(sizeof(Collection));
      collection_init(c->next);
    }
    c = c->next;
  }
  return &(c->p[index]);
}

void collection_reset(Collection *c)
{
  int i;
  if (c->next) {
    collection_reset(c->next);
    free(c->next);
  }

  c->next = NULL;
  for (i = 0; i < 32; i++) c->p[i] = NULL;
}
