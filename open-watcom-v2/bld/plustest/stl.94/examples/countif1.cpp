// This example compiles using the new STL<ToolKit> from ObjectSpace, Inc.
// STL<ToolKit> is the EASIEST to use STL that works on most platform/compiler 
// combinations, including cfront, Borland, Visual C++, C Set++, ObjectCenter, 
// and the latest Sun & HP compilers. Read the README.STL file in this 
// directory for more information, or send email to info@objectspace.com.
// For an overview of STL, read the OVERVIEW.STL file in this directory.

#include <stl.h>
#include <iostream.h>

int odd (int a_)
{
  return a_ % 2;
}

int main ()
{
  vector <int> numbers(100);
  for (int i = 0; i < 100; i++)
    numbers[i] = i % 3;
  int elements = 0;
  count_if (numbers.begin (), numbers.end (), odd, elements);
  cout << "Found " << elements << " odd elements." << endl;
  return 0;
}
