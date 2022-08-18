// This example compiles using the new STL<ToolKit> from ObjectSpace, Inc.
// STL<ToolKit> is the EASIEST to use STL that works on most platform/compiler 
// combinations, including cfront, Borland, Visual C++, C Set++, ObjectCenter, 
// and the latest Sun & HP compilers. Read the README.STL file in this 
// directory for more information, or send email to info@objectspace.com.
// For an overview of STL, read the OVERVIEW.STL file in this directory.

#include <stl.h>
#include <iostream.h>

int v1[3] = { 13, 18, 23 };
int v2[4] = { 10, 13, 17, 23 };
int result[4] = { 0, 0, 0, 0 };

int main ()
{
  set_difference (v1, v1 + 3, v2, v2 + 4, result);
  int i;
  for (i = 0; i < 4; i++)
    cout << result[i] << ' ';
  cout << endl;
  set_difference (v2, v2 + 4, v1, v1 + 2, result);
  for (i = 0; i < 4; i++)
    cout << result[i] << ' ';
  cout << endl;
  return 0;
}
