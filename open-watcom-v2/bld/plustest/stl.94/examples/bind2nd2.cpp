// This example compiles using the new STL<ToolKit> from ObjectSpace, Inc.
// STL<ToolKit> is the EASIEST to use STL that works on most platform/compiler 
// combinations, including cfront, Borland, Visual C++, C Set++, ObjectCenter, 
// and the latest Sun & HP compilers. Read the README.STL file in this 
// directory for more information, or send email to info@objectspace.com.
// For an overview of STL, read the OVERVIEW.STL file in this directory.

#include <iostream.h>
#include <stl.h>

int array [3] = { 1, 2, 3 };

int main ()
{
  replace_if (array, array + 3, bind2nd (greater<int> (), 2), 4);
  for (int i = 0; i < 3; i++)
    cout << array[i] << endl;
  return 0;
}
