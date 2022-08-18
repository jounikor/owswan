template <class T>
    struct Stack {
	void foo( T x )
	{
	    x.a = 1;
	}
    };

template<>
struct Stack<int> {
    int foo;
};

Stack<int> x;
Stack<double> y;

void foo()
{
    x.foo = 1;
}

template struct Stack<int>;
template struct Stack<double>;
