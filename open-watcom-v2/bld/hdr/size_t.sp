::
:: POSIX, ISO C/C++ size_t typedef declaration
::
:: The C++ compiler has knowledge of this type so if it changes from
:: 'unsigned' please inform the C++ developers and tell them to check
:: their op new diagnostics. Also don't forget ssize_t.
:segment CNAME
#ifndef _STDSIZE_T_DEFINED
#define _STDSIZE_T_DEFINED
 namespace std {
   typedef unsigned size_t;
 }
:: The type __w_size_t is an alias for std::size_t or size_t as
:: appropriate. It is intended to simplify the standard headers.
 typedef std::size_t __w_size_t;
#endif
:elsesegment
:segment !CONLY
:segment !CPPONLY
#ifdef __cplusplus
:endsegment
 #ifndef _STDSIZE_T_DEFINED
 #define _STDSIZE_T_DEFINED
  namespace std {
    typedef unsigned size_t;
  }
  typedef std::size_t __w_size_t;
 #endif
 #ifndef _SIZE_T_DEFINED
 #define _SIZE_T_DEFINED
  #define _SIZE_T_DEFINED_
  using std::size_t;
 #endif
:segment !CPPONLY
#else  /* __cplusplus not defined */
:endsegment
:endsegment
:segment !CPPONLY
 #ifndef _SIZE_T_DEFINED
 #define _SIZE_T_DEFINED
  #define _SIZE_T_DEFINED_
  typedef unsigned size_t;
  typedef size_t   __w_size_t;
 #endif
:segment !CONLY
#endif /* __cplusplus not defined */
:endsegment
:endsegment
:endsegment
