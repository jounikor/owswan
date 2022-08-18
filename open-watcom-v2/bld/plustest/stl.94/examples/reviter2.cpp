// This example compiles using the new STL<ToolKit> from ObjectSpace, Inc.
// STL<ToolKit> is the EASIEST to use STL that works on most platform/compiler 
// combinations, including cfront, Borland, Visual C++, C Set++, ObjectCenter, 
// and the latest Sun & HP compilers. Read the README.STL file in this 
// directory for more information, or send email to info@objectspace.com.
// For an overview of STL, read the OVERVIEW.STL file in this directory.

#include <iostream.h>
#include <stl.h>

int array [] = { 1, 5, 2, 3 };

int main ()
{
  vector<int> v (array, array + 4);
  vector<int>::reverse_iterator r;
  for (r = v.rbegin (); r != v.rend (); r++)
    cout << *r << endl;
  return 0;
}
