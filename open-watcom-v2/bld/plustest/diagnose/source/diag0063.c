typedef unsigned size_t;
struct S {
    class T {
        T();
        ~T();
        void * operator new ( size_t );
        void operator delete( void * );
        void operator delete[]( void * );
    };
    class N {
        N();
        ~N();
        void * operator new ( size_t );
    };
private:
    S();
    ~S();
   void operator delete ( void * );
   void operator delete[] ( void * );
    void * operator new ( size_t );
};

void foo( int x )
{
    S *s;
    S::T *t;
    S::N *n;

    s = new S;
    delete s;
    s = new S[x];
    delete [] s;
    t = new S::T;
    delete t;
    t = new S::T[x];
    delete [] t;
    n = new S::N;
    delete n;
    n = new S::N[x];
    delete [] n;
}
