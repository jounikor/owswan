// This example compiles using the new STL<ToolKit> from ObjectSpace, Inc.
// STL<ToolKit> is the EASIEST to use STL that works on most platform/compiler 
// combinations, including cfront, Borland, Visual C++, C Set++, ObjectCenter, 
// and the latest Sun & HP compilers. Read the README.STL file in this 
// directory for more information, or send email to info@objectspace.com.
// For an overview of STL, read the OVERVIEW.STL file in this directory.

#include <stl.h>
#include <iostream.h>

int numbers[6] = { 5, 2, 4, 3, 1, 6 };

int main ()
{
  partial_sort (numbers, numbers + 3, numbers + 6);
  for (int i = 0; i < 6; i++)
    cout << numbers[i] << ' ';
  cout << endl;
  return 0;
}
