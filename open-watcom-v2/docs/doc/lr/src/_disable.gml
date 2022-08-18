.func _disable
.synop begin
#include <i86.h>
void _disable( void );
.ixfunc2 '&Interrupt' &funcb
.synop end
.desc begin
The
.id &funcb.
function causes interrupts to become disabled.
.np
The
.id &funcb.
function would be used in conjunction with the
.reffunc _enable
function to make sure that a sequence of instructions are executed
without any intervening interrupts occurring.
.im privity
.desc end
.return begin
The
.id &funcb.
function returns no value.
.return end
.see begin
.seelist _enable
.see end
.exmp begin
#include <stdio.h>
#include <stdlib.h>
#include <i86.h>
.exmp break
struct list_entry {
    struct list_entry *next;
    int               data;
};
volatile struct list_entry *ListHead = NULL;
volatile struct list_entry *ListTail = NULL;
.exmp break
void insert( struct list_entry *new_entry )
  {
    /* insert new_entry at end of linked list */
    new_entry->next = NULL;
    _disable();       /* disable interrupts */
    if( ListTail == NULL ) {
      ListHead = new_entry;
    } else {
      ListTail->next = new_entry;
    }
    ListTail = new_entry;
    _enable();        /* enable interrupts now */
  }
.exmp break
void main()
  {
    struct list_entry *p;
    int i;

    for( i = 1; i <= 10; i++ ) {
      p = (struct list_entry *)
          malloc( sizeof( struct list_entry ) );
      if( p == NULL ) break;
      p->data = i;
      insert( p );
    }
  }
.exmp end
.class Intel
.system
