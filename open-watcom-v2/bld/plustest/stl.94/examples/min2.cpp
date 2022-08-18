// This example compiles using the new STL<ToolKit> from ObjectSpace, Inc.
// STL<ToolKit> is the EASIEST to use STL that works on most platform/compiler 
// combinations, including cfront, Borland, Visual C++, C Set++, ObjectCenter, 
// and the latest Sun & HP compilers. Read the README.STL file in this 
// directory for more information, or send email to info@objectspace.com.
// For an overview of STL, read the OVERVIEW.STL file in this directory.

#include <stl.h>
#include <iostream.h>
#include <string.h>

bool str_compare (const char* a_, const char* b_)
{
  return ::strcmp (a_, b_) < 0 ? 1 : 0;
}

int main ()
{
  cout << min ((char const*)"shoe", (char const*)"shine", str_compare) << endl;
  return 0;
}
