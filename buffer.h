#ifndef BUFFER_H
#define BUFFER_H

typedef struct _bufferchunk BufferChunk;
struct _bufferchunk {
  char *buf;
  unsigned long len;
  struct _bufferchunk *next;
};

typedef struct _buffer Buffer;
struct _buffer {
  unsigned long size;
  BufferChunk *head;
  BufferChunk *tail;
};

void buffer_init(Buffer *b);
void buffer_put(Buffer *b, const char *s, unsigned long len);
int buffer_get(Buffer *b, char **dest);
char *buffer_get_line(Buffer *b);
void buffer_finish(Buffer *b);

#define buffer_get_size(b) ((b)->size)
#define buffer_has_data(b) (!!buffer_get_size(b))

#endif /* BUFFER_H */
