//basat en https://embeddedartistry.com/blog/2017/4/6/circular-buffers-in-cc <3
#ifndef CIRCULAR_BUFFER
#define CIRCULAR_BUFFER

typedef struct{
    unsigned int head, tail, size, num_elem;
    char *buffer;
}circular_buffer;

void circular_buffer_init(circular_buffer *cb, int size)
{
    char buff[size];
    cb->head = 0;
    cb->tail = 0;
    cb->size = size;
    cb->num_elem = 0;
    cb->buffer = buff;
}

void circular_buffer_reset(circular_buffer *cb)
{
    cb->head = 0;
    cb->tail = 0;
}

int circular_buf_empty(circular_buffer cbuf)
{
    //Si apunten al mateix esque esta buit
    return (cbuf.head == cbuf.tail);
}

int circular_buf_full(circular_buffer cbuf)
{
    // Full vol dir que el head esta una posicio per darrere del tail
    return ((cbuf.head + 1) % cbuf.size) == cbuf.tail;
}

int circular_buf_num_elems(circular_buffer *cb) {
	return cb->num_elem;
}

int circular_buf_put(circular_buffer *cbuf, char data)
{
    if(circular_buf_full(*cbuf)) return -1;
    else{
        cbuf->buffer[cbuf->tail] = data;
        cbuf->tail = (cbuf->tail +1)%cbuf->size; //En veritat no caldria perque no sobrescrivim
        cbuf->num_elem++;
        return 0;
    }
}
int circular_buf_read(circular_buffer *cbuf, char *data)
{
    if(circular_buf_empty(*cbuf)) return -1;
    else{
        *data = cbuf->buffer[cbuf->head];
        cbuf->head = (cbuf->head +1)%cbuf->size;
        return 0;
    }
}
#endif