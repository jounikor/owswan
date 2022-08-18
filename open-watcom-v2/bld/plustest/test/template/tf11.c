extern "C" int printf( char *, ... );

template <class T,class W,int s,int *p>
    struct Stack {
	Stack *n;
	T v;
	W x;
	Stack();
    };

template <class X,class W,int s,int *p>
Stack<X,W,s,p>::Stack() : n(0)
{
}

int a1;

template <class U,class W>
    U pop( Stack<U,W,19,&a1> **p )
    {
	Stack<U,W,19,&a1> *e = *p;
	if( e ) {
	    U v = e->v;
	    *p = e->n;
	    return v;
	}
	return 0x123a;
    }

template <class U,class W>
    void push( Stack<U,W,19,&a1> **p, Stack<U,W,19,&a1> *e )
    {
	e->n = *p;
	*p = e;
    }

Stack<int,char,19,&a1> *head;
Stack<int,char,19,&a1> *elt;

int pop()
{
    return pop( &head );
}

void main()
{
    push( &head, elt );
    if( pop() == 0x123a ) {
	printf( "PASS\n" );
    } else {
	printf( "FAIL\n" );
    }
}
