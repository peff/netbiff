#include "buffer.h"
#include "xlib.h"

#include <stdlib.h>
#include <string.h>

void buffer_init(Buffer *b) {
  b->size = 0;
  b->head = b->tail = NULL;
}

void buffer_finish(Buffer *b) {
  BufferChunk *bc, *next;

  for(bc = b->head; bc; bc = next) {
    next= bc->next;
    free(bc->buf);
    free(bc);
  }
  b->head = b->tail = NULL;
  b->size = 0;
}

void buffer_put(Buffer *b, const char *s, unsigned long len) {
  BufferChunk *bc;

  bc = xmalloc(sizeof(BufferChunk));
  bc->len = len;
  bc->buf = xmalloc(len + 1);
  memcpy(bc->buf, s, len);
  bc->buf[len] = '\0';
  bc->next = NULL;

  if(b->tail)
    b->tail->next = bc;
  else
    b->head = bc;
  b->tail = bc;

  b->size += len;
}

static void compact_buffer_to_lines(Buffer *b) {
  BufferChunk *old;
  int in_line_flag;

  old = b->head;
  b->head = b->tail = NULL;

  in_line_flag = 0;
  while(old) {
    BufferChunk *next = old->next;

    /* If the old chunk was a line, we steal it. */
    if(!in_line_flag && 
        memchr(old->buf, '\n', old->len) == old->buf + old->len - 1) {
      old->next = NULL;
      if(b->tail)
        b->tail->next = old;
      else
        b->head = old;
      b->tail = old;
    }
    /* Otherwise, add all lines */
    else {
      char *p = old->buf;
      while(p) {
        char *q;
        unsigned long len;

        /* If there are no more complete lines, save what we have
         * and quit. */
        q = memchr(p, '\n', old->len - (p - old->buf));
        if(!q)
          len = old->len - (p - old->buf);
        else
          len = q - p + 1;

        /* No point in adding 0 bytes */
        if(len) {
          /* Add to the last if we're continuing a line. */
          if(in_line_flag) {
            unsigned long prevlen = b->tail->len;
            b->tail->len += len;
            b->tail->buf = xrealloc(b->tail->buf, b->tail->len + 1);
            memcpy(b->tail->buf + prevlen, p, len);
            b->tail->buf[b->tail->len] = '\0';
          }
          else {
            BufferChunk *c;
            c = xmalloc(sizeof(BufferChunk));
            c->len = len;
            c->buf = xmalloc(len + 1);
            memcpy(c->buf, p, len);
            c->buf[len] = '\0';
            c->next = NULL;
            if(b->tail) 
              b->tail->next = c;
            else
              b->head = c;
            b->tail = c;
          }
        }

        p = q;
        if(!q) {
          if(len)
            in_line_flag = 1;
        }
        else {
          in_line_flag = 0;
          p++;
        }
      }
      free(old->buf);
      free(old);
    }
    old = next;
  }
}

char *buffer_get_line(Buffer *b) {
  BufferChunk *bc;

  /* If we have no data, we have no lines. */
  bc = b->head;
  if(!bc)
    return NULL;

  /* If our first chunk is a single, complete line,
   * we were probably just compacted. Return the line. */
  if(memchr(bc->buf, '\n', bc->len) == bc->buf + bc->len - 1) {
    char *buf;
    b->head = bc->next;
    if(!b->head)
      b->tail = NULL;
    buf = bc->buf;
    b->size -= bc->len;
    free(bc);
    return buf;
  }

  /* Otherwise we need to reorganize the data in lines. */
  compact_buffer_to_lines(b);

  /* Each chunk should now be either a line *or* part of a line
   * but never multiple lines. Note that bc is no longer valid
   * after compacting.  */
  bc = b->head;
  if(!bc)
    return NULL;
  if(bc->buf[bc->len - 1] == '\n') {
    char *buf;
    b->head = bc->next;
    if(!b->head)
      b->tail = NULL;
    buf = bc->buf;
    b->size -= bc->len;
    return buf;
  }

  /* If we're part of a line, we have no line. */
  return NULL;
}

int buffer_get(Buffer *b, char **dest) {
  BufferChunk *bc;
  BufferChunk *next;
  char *pos;

  if(!b->size) {
    *dest = NULL;
    return 0;
  }
  
  pos = *dest = xmalloc(b->size);
  for(bc = b->head; bc; bc = next) {
    next = bc->next;

    memcpy(pos, bc->buf, bc->len);
    pos += bc->len;
    free(bc->buf);
    free(bc);
  }
  b->size = 0;
  b->head = b->tail = NULL;
  return pos - *dest;
}


