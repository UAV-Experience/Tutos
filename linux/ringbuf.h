#ifndef RINGBUF_H
#define RINGBUF_H


typedef struct ringbuf
{
    char *buf;              //points to data array
    int length;            //length of data array
    int head, tail;        //producer and consumer indices
} ringbuffer;

//initializes the given ringbuffer with the supplied array and its length
void rbinit(ringbuffer *rb, char *array, int length);

//returns boolean true if the ringbuffer is empty, false otherwise
unsigned char rbisempty(ringbuffer *rb);

//returns boolean true if the ringbuffer is full, false otherwise
unsigned char rbisfull(ringbuffer *rb);

//consumes an element from the buffer
//returns NULL if buffer is empty or a pointer to the array element otherwise
char *rbget(ringbuffer *rb);

//puts an element into the buffer
//returns 0 if buffer is full, otherwise returns 1
int rbput(ringbuffer *rb, char c);

#endif
