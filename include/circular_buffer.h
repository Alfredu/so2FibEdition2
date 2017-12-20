//basat en https://embeddedartistry.com/blog/2017/4/6/circular-buffers-in-cc <3
#ifndef CIRCULAR_BUFFER
#define CIRCULAR_BUFFER

typedef struct{
    unsigned int head, tail, size, num_elem;
    char *buffer;
}circular_buffer;

void circular_buffer_init(circular_buffer *cb, int size);
void circular_buffer_reset(circular_buffer *cb);

int circular_buf_empty(circular_buffer cbuf);
int circular_buf_full(circular_buffer cbuf);
int circular_buf_num_elems(circular_buffer *cb);

int circular_buf_put(circular_buffer *cbuf, char data);
int circular_buf_read(circular_buffer *cbuf, char *data);
#endif /*CIRCULAR_BUFFER*/