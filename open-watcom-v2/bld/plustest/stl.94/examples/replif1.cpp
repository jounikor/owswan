// This example compiles using the new STL<ToolKit> from ObjectSpace, Inc.
// STL<ToolKit> is the EASIEST to use STL that works on most platform/compiler 
// combinations, including cfront, Borland, Visual C++, C Set++, ObjectCenter, 
// and the latest Sun & HP compilers. Read the README.STL file in this 
// directory for more information, or send email to info@objectspace.com.
// For an overview of STL, read the OVERVIEW.STL file in this directory.

#include <stl.h>
#include <iostream.h>

bool odd (int a_)
{
  return a_ % 2;
}

int main ()
{
  vector <int> v1 (10);
  int i;
  for (i = 0; i < v1.size (); i++)
  {
    v1[i] = i % 5;
    cout << v1[i] << ' ';
  }
  cout << endl;
  replace_if (v1.begin (), v1.end (), odd, 42);
  for (i = 0; i < v1.size (); i++)
    cout << v1[i] << ' ';
  cout << endl;
  return 0;
}
