#include <stddef.h>
#include "ringbuf.h"
/*
    Ring (circular) buffer implementation.
    Author: Kalim Moghul (kfm9 at cornell dot edu)
    Invariants:
        head==tail iff buffer is empty.
        If buffer is not empty, head points at next valid element to be consumed.
        tail always points at the next empty element.
            => There is always one unused element in a full buffer.
            => length must be greater than 1
    */

//initializes the given ringbuffer with the supplied array and its length
void rbinit(ringbuffer *rb, char *array, int length)
{
    rb->buf = array;
    rb->length = length;
    rb->head = rb->tail = 0;
}

//returns boolean true if the ringbuffer is empty, false otherwise
unsigned char rbisempty(ringbuffer *rb)
{
    return (rb->head == rb->tail);
}

//returns boolean true if the ringbuffer is full, false otherwise
unsigned char rbisfull(ringbuffer *rb)
{
    return (((rb->tail + 1) % rb->length) == rb->head);
}

//consumes an element from the buffer
//returns NULL if buffer is empty or a pointer to the array element otherwise
char* rbget(ringbuffer *rb)
{
    char *ret;
    if (rb->head == rb->tail)
        return NULL;
    else
    {
        ret = rb->buf + rb->head;                   //index into the array at head pointer
        rb->head = (rb->head + 1) % rb->length;     //move head pointer forward one element (with wraparound)
        return ret;
    }
}

//puts an element into the buffer
//returns 0 if buffer is full, otherwise returns 1
int rbput(ringbuffer *rb, char c)
{
    int newtail;
    newtail = (rb->tail + 1) % rb->length;     //calculate where the new tail would be
    if (newtail == rb->head)                        //if the new tail would make the buffer look empty, buffer is full
        return 0;
    else
    {
        rb->buf[rb->tail] = c;                      //store the data
        rb->tail = newtail;                         //move the tail pointer forward (with wraparound)
        return 1;
    }
}
