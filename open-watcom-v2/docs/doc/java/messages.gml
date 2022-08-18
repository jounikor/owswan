:cmt    Word usage:
:cmt
:cmt    'parameter' is used for function parameters ( int a )
:cmt    'argument' is used for function arguments ( 0, 1 )

:cmt    GML Macros used (see MSGMACS.GML):

:cmt            :ansi <n>       warning if extensions enabled, error otherwise
:cmt            :ansierr        ignored if extensions enabled, error otherwise
:cmt            :ansiwarn <n>   ignored if extensions enabled, warn otherwise
:cmt            :warning <n>    message is a warning with a specific level
:cmt            :info           informational message
:cmt
:cmt            :msggrp         2-character code
:cmt            :msggrptxt      text describing above
:cmt
:cmt            :msgsym <sym>   internal symbolic name for message
:cmt            :msgtxt <text>  text for message
:cmt
:cmt            :msglvl         start of header title for a group of messages
:cmt            :emsglvl        end of header title for a group of messages
:cmt            :errbad         start of an example that generates an error msg
:cmt            :eerrbad        end of an example that generates an error msg
:cmt            :errgood        start of an example that compiles clean
:cmt            :eerrgood       end of an example that compiles clean
:cmt
:cmt            .kw             highlight a keyword
:cmt            .id             highlight an identifier
:cmt            .ev             highlight an environment variable
:cmt            .us             italicize a phrase
:cmt            .np             start a new paragraph

:cmt    The following substitutions are made:
:cmt            &incvarup       environment variable for include path
:cmt            &wclname        Compile and Link utility name

:cmt    Note for translators:

:cmt    Japanese error messages are supported via the :MSGJTXT tag.
:cmt    If there is no :MSGJTXT. for a particular :MSGSYM. then the
:cmt    message will come out in English.  Translation may proceed
:cmt    by translating error messages that do not have the :MSGJTXT.
:cmt    tag present and adding the :MSGJTXT. tag with the Japanese
:cmt    text after the :MSGTXT. tag.

:cmt    We will also need :MSGJGRPTXT

:cmt    Translators can search for empty tags in order to find messages
:cmt    that need translation.

:cmt -------------------------------------------------------------------
:MSGGRP. GN
:MSGGRPTXT. General Messages
:MSGJGRPTXT. �S�ʓI��ү����
:cmt -------------------------------------------------------------------

:MSGSYM. ERR_CALL_WATCOM
:MSGTXT. internal compiler error
:MSGJTXT. �������߲״װ
If this message appears, please report the problem directly to Sybase.

:MSGSYM. ERR_EXCEEDED_LIMIT
:MSGTXT. too many errors: compilation aborted
:MSGJTXT. �װ���������܂�: ���߲ق͒��~����܂���
The Sybase Java compiler sets a limit to the number of error messages
it will issue.
Once the number of messages reaches the limit the above message is issued.
This limit can be changed via the "/e" command line option.

:MSGSYM. ERR_FATAL
:MSGTXT. internal compiler error
:MSGJTXT. �������߲״װ
If this message appears, please report the problem directly to Sybase.

:MSGSYM. INF_FATAL
:MSGTXT. %s
:MSGJTXT. %s
:INFO.
The text of this message gives the internal reason for the internal
compiler error.

:MSGSYM. INF_PRESS_TO_CONTINUE_Q_TO_QUIT
:MSGTXT. *** press any key to continue or q to quit ****
:MSGJTXT. *** ���~����ꍇ�ɂ� q ���A���s����ɂ͉������������Ă������� ****
:INFO.
This message is displayed on the console while waiting for a user response
between groups of options.

:MSGSYM. INF_IDENTIFIER
:MSGTXT. identifier is '%i'
:MSGJTXT. ���ʎq��'%i'�ł�
:INFO.
This informational message indicates the name of an identifier for which
an error or warning message was diagnosed.

:MSGSYM. ERR_BREAK_KEY_HIT
:MSGTXT. keyboard interrupt detected
:MSGJTXT. �L�[�{�[�h���荞�݂����o����܂���
The compile has been aborted with Ctrl/C or Ctrl/Break.

:MSGSYM. ERR_EX_CANNOT_MODIFY_LENGTH
:MSGTXT. array length member cannot be modified
:MSGJTXT. �z��̒��������ނ�ύX���邱�Ƃ͂ł��܂���
The length member of an array object cannot be
modified since it is a read only quantity.
:errbad.
class example {
    int [] fn( int a[] ) {
      a.length += 2;
      return a;
    }
}
:eerrbad.

:eMSGGRP. GN
:cmt -------------------------------------------------------------------
:MSGGRP. CL
:MSGGRPTXT. Messages related to command-line processing
:MSGJGRPTXT. �����ײݏ����Ɋւ���ү����
:cmt -------------------------------------------------------------------

:MSGSYM. ERR_CL_OPTION
:MSGTXT. command line option is invalid or incomplete
:MSGJTXT. �����ײ� ��߼�݂��������s���S�ł�
The indicated option was invalid.

:MSGSYM. ERR_CL_NO_FILE
:MSGTXT. file name required on command line
:MSGJTXT. �����ײ݂�̧�ٖ����K�v�ł�
The name of the file to compile was missing.

:MSGSYM. ERR_CL_NEST
:MSGTXT. too many indirection levels on command line
:MSGJTXT. �����ײ݂ɊԐ����ق��������܂�
There were too many (maximum 3 is allowed) levels of "@" indirection on the
command line.

:MSGSYM. ERR_CL_ID
:MSGTXT. invalid identifier in command-line option
:MSGJTXT. �����ײ� ��߼�݂ɖ����Ȏ��ʎq������܂�
The identifier was missing or in an invalid format.

:MSGSYM. ERR_CL_INDIRECT
:MSGTXT. invalid environment variable or file name
:MSGJTXT. �����Ȋ��ϐ��܂���̧�ٖ����s���ł�
The identifier following the "@" symbol in the command line is either
missing or is neither the name of an environment variable nor the name of
a file.

:MSGSYM. ERR_CL_NUMBER
:MSGTXT. invalid number on command line
:MSGJTXT. �����ײ݂ɖ����Ȑ��l������܂�
The number was missing or in an invalid format.

:MSGSYM. ERR_CL_PATH
:MSGTXT. file path has invalid characters within it
:MSGJTXT. ̧�� �߽�ɖ����ȕ���������܂�
The indicated file path was invalid.

:MSGSYM. INF_CL_SWITCH
:MSGTXT. processing %s
:MSGJTXT. %s��������
:INFO.
This informational message indicates where an error or warning was detected
while processing the switches specified on the command line, in environment
variables, in command files (using the '@' notation), or in the batch command
file (specified using the -fc option).

:MSGSYM. INF_CL_ENV_VAR
:MSGTXT. processing environment variable: %s
:MSGJTXT. �ȉ��̊��ϐ����������F%s
:INFO.
This informational message indicates where an error or warning was detected
while processing the indicated environment variable.

:MSGSYM. INF_CL_FILENAME
:MSGTXT. processing command file: %s, line %u
:MSGJTXT. �ȉ��̺����̧�ق��������F%s, �s %u
:INFO.
This informational message indicates where an error or warning was detected
while processing the indicated file at the indicated line.

:MSGSYM. ERR_CL_DIR
:MSGTXT. directory has invalid characters within it
:MSGJTXT. �ިڸ�؂ɖ����ȕ���������܂�
The indicated directory was invalid.

:MSGSYM. INF_CL_CLASSPATH
:MSGTXT. using class path: %s
:MSGJTXT. �ȉ��̸׽ �߽���g�p���F%s
:INFO.
This informational message prints the search path of the
the classpath.

:MSGSYM. ERR_TGT_COMPILES
:MSGTXT. can specify at most one of -zcs, -zjs, -zcm, -zjm options
:MSGJTXT. -zcs, -zjs, -zcm, -zjm��߼�݂Ŏw��ł���͈̂�����ł�
At most one of the indicated options can be specified on the command line.

:MSGSYM. ERR_CL_INVALID_WARNING
:MSGTXT. invalid warning referenced in -wcd option
:MSGJTXT. �����Ȍx����-wcd��߼�݂ɎQ�Ƃ���Ă��܂�
Either the syntax of the warning identity is incorrect or
the warning does not exist.
For compilers with group indices (e.g., GR-023), the proper
syntax to use is
.kw -wcd=GR-023.
For compilers without group indices (e.g., 0234), the proper
syntax to use is
.kw -wcd=0234.

:MSGSYM. ERR_CL_CANNOT_CREATE_DIR
:MSGTXT. cannot create directory '%s'
:MSGJTXT. �ިڸ��'%s'���쐬�ł��܂���
The root directory for class output is either invalid or
it could not be created.  Correct the error or specify another directory
to ensure the directory can be used for classfile output.

:MSGSYM. ERR_CL_FILE
:MSGTXT. file name has invalid characters within it
:MSGJTXT. ̧�ٖ��ɖ����ȕ���������܂�
The indicated file name was invalid.

:eMSGGRP. CL
:cmt -------------------------------------------------------------------
:MSGGRP. CT
:MSGGRPTXT. Messages related to compiler context
:MSGJGRPTXT. ���߲� ��÷�ĂɊւ���ү����
:cmt -------------------------------------------------------------------


:MSGSYM. INF_CT_INIT
:MSGTXT. compiler initialization
:MSGJTXT. ���ׂ߲̏�����
:INFO.
This informational message indicates that an error or warning was detected
while the compiler was being initialized.

:MSGSYM. INF_CT_FINI
:MSGTXT. compiler completion
:MSGJTXT. ���߲ي���
:INFO.
This informational message indicates that an error or warning was detected
while the compiler was completing after compilation.

:MSGSYM. INF_CT_CMDLINE
:MSGTXT. validation of options from command line
:MSGJTXT. �����ײ݂���̵�߼�݂̊m�F
:INFO.
This informational message indicates that an error or warning was detected
while the compiler was validating the options specified on the command line.

:MSGSYM. INF_CT_ANAL_FILE
:MSGTXT. file: %f
:MSGJTXT. ̧�فF%f
:INFO.
This informational message indicates the file in which
an error or warning was detected.

:MSGSYM. INF_CT_ANAL_INCLUDED
:MSGTXT. included from '%f', line %u
:MSGJTXT. '%f'����ݸٰ��, �s %u
:INFO.
This informational message indicates inclusion point of the file in
the preceding diagnostic message.


:eMSGGRP. CT
:cmt -------------------------------------------------------------------
:MSGGRP. SC
:MSGGRPTXT. Messages related to scanning
:MSGJGRPTXT. ���݂Ɋւ���ү����
:cmt -------------------------------------------------------------------

:MSGSYM. ERR_SC_BAD_FLOAT
:MSGTXT. illegal character in floating point number
:MSGJTXT. ���������_���ɕs���ȕ���������܂�
Attempting to scan a floating point number.

:MSGSYM. ERR_SC_BAD_HEX
:MSGTXT. illegal character in hexadecimal number
:MSGJTXT. 16�i���ɕs���ȕ�������܂�
Attempting to scan a hexadecimal number.

:MSGSYM. ERR_SC_BAD_OCT
:MSGTXT. illegal character in octal number
:MSGJTXT. 8�i���ɕs���ȕ���������܂�
Attempting to scan a octal number.

:MSGSYM. ERR_SC_INT_OVER
:MSGTXT. integer literal is too large
:MSGJTXT. �������ق��傫�����܂�
Integer larger than max int.

:MSGSYM. ERR_SC_ESC_OVER
:MSGTXT. octal escape sequence > 0x00ff
:MSGJTXT. 8�i������ ���ݽ > 0x00ff
Octal escape sequence has too many digits or is greater than 0x00ff.

:MSGSYM. ERR_SC_BAD_ESC
:MSGTXT. bad escape sequence
:MSGJTXT. �s���ȴ����� ���ݽ
Unrecognized escape sequence.

:MSGSYM. ERR_SC_BAD_UNI_ESC
:MSGTXT. bad Unicode escape sequence
:MSGJTXT. �s����Unicode ������ ���ݽ
A Unicode escape sequence must be of the form \uxxxx.
Where 'u' is one or more 'u's and 'xxxx' are exactly four hexadecimal characters.

:MSGSYM. ERR_SC_LT_CHRLIT
:MSGTXT. line terminator in character literal
:MSGJTXT. �������قɍs��؂�q������܂�
Line terminators not permitted in character literals.

:MSGSYM. ERR_SC_LT_STRLIT
:MSGTXT. line terminator in string literal
:MSGJTXT. ���������قɍs��؂�q������܂�
Line terminators not permitted in strings.

:MSGSYM. ERR_SC_BAD_CHRLIT
:MSGTXT. illegal character literal
:MSGJTXT. �s���ȕ�������
Only one escaped or character value is permitted in ''.

:MSGSYM. ERR_SC_BAD_CHAR
:MSGTXT. unrecognized character '%c' in file
:MSGJTXT. �m�F�ł��Ȃ�����'%c'��̧�قɂ���܂�
A character not defined for Java was found in the file.

:MSGSYM. ERR_SC_UNCLOSED_STRLIT
:MSGTXT. missing closing '"' in a string literal
:MSGJTXT. ���������قɏI����'"'������܂���
End of file was read before the end of a string

:MSGSYM. ERR_SC_UNCLOSED_CHRLIT
:MSGTXT. missing closing "'" in a character literal
:MSGJTXT. �������قɏI����"'"������܂���
End of file was read before the end of a character literal

:MSGSYM. ERR_SC_UNCLOSED_COMMENT
:MSGTXT. missing closing "*" and "/" in a comment
:MSGJTXT. ���Ă�"*"��"/"������܂���
End of file was read before the end of a comment

:MSGSYM. ERR_SC_CANNOT_CONVERT_DBCS_TO_UNICODE
:MSGTXT. cannot convert multi-byte character '%s' to Unicode
:MSGJTXT. ����޲ĕ��� '%s' ��Unicode�ɕϊ��ł��܂���
The Java language requires that all input be specified in Unicode
and has no concept of multi-byte character representations.
An extension to the input format allows multi-byte characters
to be used anywhere in the source file with the assumption that
the host operating system provides a conversion from multi-byte
characters to Unicode since all Java output must be in Unicode.
.np
The multi-byte character in the message could not be converted
to a Unicode representation by the host operating system using
the default code page.  Use the appropriate Unicode escape
(e.g.,
.kw \u007A
represents the letter 'z')
to specify exactly what Unicode character representation should be used.

:MSGSYM. ERR_SC_TOKEN_TOO_LONG
:MSGTXT. too many characters in token
:MSGJTXT. �g�[�N���̒��̕������������܂�
The compiler found a token that could not be represented
because there were too many characters in it.
An example would be a string literal that produces more
than 64k bytes after UTF8 encoding.
Such a string cannot be represented in the current
classfile format.

:eMSGGRP. SC
:cmt -------------------------------------------------------------------
:MSGGRP. IO
:MSGGRPTXT. Messages related to input/output operations
:MSGJGRPTXT. ���o�͑���Ɋւ���ү����
:cmt -------------------------------------------------------------------

:MSGSYM. INF_IO_FILENAME
:MSGTXT. file is '%f'
:MSGJTXT. ̧�ق�'%f'�ł�
:INFO.
This informational message indicates the name of a file for which an error
or warning message was diagnosed.

:MSGSYM. INF_IO_SYSERROR
:MSGTXT. system error is '%s'
:MSGJTXT. ���� �װ��'%s'�ł�
:INFO.
This informational message indicates the operating-system supplied message
which corresponds to the indicated input/output error.

:MSGSYM. ERR_IO_OPEN
:MSGTXT. i/o error opening file
:MSGJTXT. ̧�ق��J���ۂɓ��o�ʹװ���������܂���
The file indicated by the following informational message could not be
opened.
Check that the file name is correctly spelled.

:MSGSYM. ERR_IO_READ
:MSGTXT. i/o error reading file
:MSGJTXT. ̧�ق̓ǂݍ��ݎ��ɓ��o�ʹװ���������܂���
The file indicated by the following informational message caused an
input/output error to be detected during an attempt to read that file.

:MSGSYM. ERR_IO_WRITE
:MSGTXT. i/o error writing file
:MSGJTXT. ̧�ق̏o�͎��ɓ��o�ʹװ���������܂���
The file indicated by the following informational message caused an
input/output error to be detected during an attempt to write that file.

:MSGSYM. ERR_IO_CLOSE
:MSGTXT. i/o error closing file
:MSGJTXT. ̧�ق����ۂɓ��o�ʹװ���������܂���
The file indicated by the following informational message caused an
input/output error to be detected during an attempt to close that file.

:MSGSYM. INF_STR_FILENAME
:MSGTXT. file is '%s'
:MSGJTXT. ̧�ق�'%s'�ł�
:INFO.
This informational message indicates the name of a file for which an error
or warning message was diagnosed.


:eMSGGRP. IO
:cmt -------------------------------------------------------------------
:MSGGRP. SY
:MSGGRPTXT. Messages related to syntax
:MSGJGRPTXT. �\���Ɋւ���ү����
:cmt -------------------------------------------------------------------

:MSGSYM. ERR_SY_SYNTAX
:MSGTXT. syntax error: found '%s'
:MSGJTXT. �\���װ�F'%s'��������܂���
A syntax error was detected at the indicated item.

:MSGSYM. ERR_SY_BAD_CAST
:MSGTXT. invalid cast expression type syntax
:MSGJTXT. �����ȷ��Ď�
The Java language grammar allows some constructs to be parsed
that are semantically meaningless.  In the case of cast expressions,
the syntax allows some invalid sentences to be accepted that
are rejected through non-syntax analysis.  In this case, the
Java compiler processed the expression as a cast but the
expression in the type expression was not a type name.
:errbad.
class example {
    void fn( int v ) {
        int x = (Vector+1)v;
    }
}
:eerrbad.

:MSGSYM. ERR_SY_EMPTY_MEMBER_DECL
:MSGTXT. empty member declaration not allowed
:MSGJTXT. ������ސ錾�͂ł��܂���
The Java language does not allow extra semicolons
to be used in a class definition.  Most Java compilers
will accept this but strictly speaking, new Java code
should not use extra semicolons.
:errbad.
class example {
    void foo() {
    };  // error
    ;   // error
}
:eerrbad.

:MSGSYM. ERR_SY_SYNTHETIC
:MSGTXT. symbol is synthetic name to support inner classes
:MSGJTXT. ����ق͓����׽���߰Ă��鍇�����ł�
The symbol has the same format as that of a synthetic name which
was made up to support inner classes.
To avoid this problem absolutely, avoid specifying names with '$'
characters within them.
The exact specification for these names can be obtained by
consulting the Javasoft documentation for inner classes.
:errbad.
class example {
    Class m = bug56.class;
    class I {
        I() {
            bug56 p = this$bug56;       // synthetic local
        }
        void foo() {
            bug56 p = this$bug56;       // synthetic member
        }
    };
    void foo() {
        m = class$bug56;        // synthetic member
        class$( "asdf" );       // synthetic method
    }
}
:eerrbad.
The example contains several cases (marked by comments) where
synthetic names have been used.

:eMSGGRP. SY
:cmt -------------------------------------------------------------------
:MSGGRP. SM
:MSGGRPTXT. Messages related to symbols
:MSGJGRPTXT. ����قɊւ���ү����
:cmt -------------------------------------------------------------------

:MSGSYM. ERR_SM_ALREADY_DEFINED
:MSGTXT. symbol already defined
:MSGJTXT. ����ق͊��ɒ�`����Ă��܂�
Two declarations with the same name are not allowed in certain
Java language contexts.  One such context is a parameter list.
In a parameter list, the names of the parameters must be unique.
:errbad.
class example {
    void fn( int v, int v ) {
        ++v;
    }
}
:eerrbad.

:MSGSYM. INF_SM_PREVIOUS_SYMBOL
:MSGTXT. conflicts with symbol: '%S'
:MSGJTXT. �ȉ��̼���قƏՓ˂��܂��F'%S'
:INFO.
This informational message indicates which symbol caused
the problem with the current symbol.

:MSGSYM. ERR_SM_NOT_FOUND
:MSGTXT. no declaration for name '%i'
:MSGJTXT. '%i'�Ƃ������O�͐錾����Ă��܂���
The indicated name has not been declared.
:errbad.
class example {
    void fn() {
        ++var;
    }
}
:eerrbad.
In the example, the name
.id var
has not been declared.

:MSGSYM. ERR_SM_PACKAGE_CLASS_CONFLICT
:MSGTXT. package and class with same name
:MSGJTXT.  �߯���ނƸ׽���������O�ł�
A package name cannot be in the same scope as a class
with the same name.
:errbad.
package example;
import example.example.*;
class example {
    void fn( int v, int v ) {
        ++v;
    }
}
:eerrbad.

:MSGSYM. INF_SM_SYMBOL_DECLARED
:MSGTXT. symbol name: '%S'
:MSGJTXT. ����ٖ��F'%S'
:INFO.
This informational message indicates the
symbol referenced in the preceding diagnostic message.

:MSGSYM. INF_SM_SYMBOL_TYPE
:MSGTXT. symbol type: %T
:MSGJTXT. ����ق̌^�F%T
:INFO.
This informational message indicates the
type of the symbol referenced in the preceding diagnostic message.

:MSGSYM. INF_SM_SYMBOL_LOCATION
:MSGTXT. symbol defined %L
:MSGJTXT. ����ق͎��̍s�ɒ�`����Ă��܂�: %L
:INFO.
This informational message indicates the source location
of the symbol referenced in the preceding diagnostic message.

:MSGSYM. ERR_SM_INACCESSIBLE_PACKAGE
:MSGTXT. package '%S' is inaccessible
:MSGJTXT. �߯����'%S'�ɱ����ł��܂���
The package in the error message could not be located
or accessed.  Typically, this happens when a default
type import declaration contains an unknown package
through a spelling mistake or file access problems.
:errbad.
import java.lang.unknown.*;
:eerrbad.

:MSGSYM. WARN_SM_UNREFD_SINGLE
:MSGTXT. single type import '%T' never used
:MSGJTXT. �P��^import'%T'���g���Ă��܂���
:WARNING. 1
This warning indicates that the single type import
was not used in the compilation unit
(i.e., source file).
The single type import
declaration can be deleted with no change to the
current semantics of the compilation unit.
:errbad.
import java.util.Vector;
import java.util.Stack;
class example {
    Stack s;
}
:eerrbad.

:MSGSYM. WARN_SM_UNREFD_DEMAND
:MSGTXT. type import on demand '%T' never used
:MSGJTXT.  ���߰Ă��ꂽ '%T' ���g�p����Ă��܂���
:WARNING. 1
This warning indicates that the type import on demand
declaration was not used in the compilation unit
(i.e., source file).
The type import on demand
declaration can be deleted with no change to the
current semantics of the compilation unit.
:errbad.
import java.math.*;
import java.awt.*;
class example {
    Color b;
}
:eerrbad.

:MSGSYM. WARN_SM_UNREFD_LOCAL_VAR
:MSGTXT.  local variable '%S' never referenced
:MSGJTXT. ۰�ٕϐ�'%S'�͎Q�Ƃ���Ă��܂���
:WARNING. 4
The local variable indicated was declared but
never referenced.  The local variable can be
deleted with no change to the current semantics
of the method.
:errbad.
class example {
    void foo( ) {
       int i;
    }
}
:eerrbad.

:MSGSYM. WARN_SM_UNREFD_PARM
:MSGTXT.  parameter '%S' never referenced
:MSGJTXT. ���Ұ�'%S'�͎Q�Ƃ���Ă��܂���
:WARNING. 4
The parameter indicated was declared but
never referenced.  The parameter can be
deleted with no change to the current semantics
of the method.
:errbad.
class example {
    void foo( float f ) {
    }
}
:eerrbad.

:MSGSYM. WARN_SM_DEMAND_IMPORT_REDUNDANT
:MSGTXT. redundant type import on demand '%S'
:MSGJTXT.  �s�v������ ���߰� '%S'
:WARNING. 1
This warning indicates that the type import on demand
declaration was already specified in the compilation unit
(i.e., source file).
The type import on demand
declaration can be deleted with no change to the
current semantics of the compilation unit.
The type import on demand
.kw java.lang.*
will be diagnosed since it is declared by the compiler.
:errbad.
import java.math.*;
import java.math.*;
import java.lang.*;
class example {
}
:eerrbad.

:MSGSYM. WARN_SM_IMPORT_REFS_OWN_CU
:MSGTXT. import directive references type in its own compilation unit
:MSGJTXT. ���߰Ė��߂����̺��߲� �ƯĎ��g���Q�Ƃ��Ă��܂�
:WARNING. 1
This warning indicates that the import directive does not
reference an external package or class, but rather it
references a type defined within the same compilation unit.
As such, the import directive can be removed and any
references to types can be recoded as direct references
since they are in the same compilation unit.
:errbad.
package a.b;
import a.b.c.*;
class c {
    class Q {
    }
    class Z {
    }
};
class d {
    // finds Z in class c!
    Z m;
    // finds same Z
    c.Z n;
};
:eerrbad.

:MSGSYM. ERR_SM_TOO_MANY
:MSGTXT. too many declarations
:MSGJTXT.
This error indicates that there are too many variables
defined in a scope.
The current classfile format cannot represent classes
that have too many variables due to file format constraints.
The class must be split into separate smaller classes.
:errbad.
class c {
    // 65536 member var decls
};
:eerrbad.

:eMSGGRP. SM
:cmt -------------------------------------------------------------------
:MSGGRP. MD
:MSGGRPTXT. Messages related to modifiers
:MSGJGRPTXT. �C���q�Ɋւ���ү����
:cmt -------------------------------------------------------------------

:MSGSYM. ERR_MD_DUPLICATE_MODIFIER
:MSGTXT. duplicate modifier '%s' specified
:MSGJTXT. �C���q'%s'�͊��Ɏw�肳��Ă��܂�
Modifiers cannot be repeated in the Java language.
Remove the extra modifiers to correct the source code.
:errbad.
final final class example {
}
:eerrbad.

:MSGSYM. ERR_MD_MULTIPLE_ACCESS
:MSGTXT. extra access modifier '%s' specified
:MSGJTXT. �g�������C���q'%s'���w�肳��܂���
Access modifiers (i.e.,
.kw private,
.kw protected,
and
.kw public
) are used to adjust the access of a name in the Java language.
A particular name can have at most one access modifier specified.
Remove the extra access modifiers to correct the source code.
:errbad.
public private class example {
}
:eerrbad.

:MSGSYM. INF_MD_PREVIOUS_ACCESS
:MSGTXT. previous access modifier was '%s'
:MSGJTXT. �O��̱����C���q��'%s'�ł�
:INFO.
This informational message indicates the
previous access modifier that conflicts with the current modifier.

:MSGSYM. ERR_MD_INVALID_CLASS_MOD
:MSGTXT. '%s' is not a valid class modifier
:MSGJTXT. '%s'�͖����ȸ׽�C���q�ł�
The allowable modifiers for a class are
.kw public,
.kw abstract,
or
.kw final.
:errbad.
transient class example {
}
:eerrbad.

:MSGSYM. ERR_MD_INVALID_INTERFACE_MOD
:MSGTXT. '%s' is not a valid interface modifier
:MSGJTXT. '%s'�͖����Ȳ��̪���C���q�ł�
The allowable modifiers for an interface are
.kw public,
or
.kw abstract.
:errbad.
transient class example {
}
:eerrbad.

:MSGSYM. ERR_MD_INVALID_CTOR_MOD
:MSGTXT. '%s' is not a valid constructor modifier
:MSGJTXT. '%s'�͖����Ⱥݽ�׸��C���q�ł�
The allowable modifiers for a constructor are
access modifiers (i.e., one of
.kw private,
.kw protected,
or
.kw public
).
:errbad.
class example {
    volatile example() {
    }
}
:eerrbad.

:MSGSYM. ERR_MD_INVALID_FIELD_MOD
:MSGTXT. '%s' is not a valid field modifier
:MSGJTXT. '%s'�͖�����̨���ޏC���q�ł�
The allowable modifiers for a field are
access modifiers (i.e., one of
.kw private,
.kw protected,
or
.kw public
),
.kw static,
.kw transient,
or
.kw volatile.
:errbad.
class example {
    synchronized int f;
}
:eerrbad.

:MSGSYM. ERR_MD_INVALID_METHOD_MOD
:MSGTXT. '%s' is not a valid method modifier
:MSGJTXT. '%s'�͖�����ҿ��ޏC���q�ł�
The allowable modifiers for a method are
access modifiers (i.e., one of
.kw private,
.kw protected,
or
.kw public
),
.kw abstract,
.kw static,
.kw final,
.kw synchronized,
or
.kw native.
:errbad.
class example {
    transient int f() {
        return 0;
    };
}
:eerrbad.

:MSGSYM. ERR_MD_INVALID_INTERFACE_METHOD_MOD
:MSGTXT. '%s' is not a valid abstract method modifier
:MSGJTXT. '%s'�͖����Ȓ���ҿ��ޏC���q�ł�
The allowable modifiers for an abstract method are
.kw public
or
.kw abstract.
:errbad.
interface example {
    transient int f() {
        return 0;
    };
}
:eerrbad.

:MSGSYM. ERR_MD_INVALID_INTERFACE_CONSTANT_MOD
:MSGTXT. '%s' is not a valid constant field modifier
:MSGJTXT. '%s'�͖����Ȓ萔̨���ޏC���q�ł�
The allowable modifiers for a constant field are
.kw public,
.kw static,
or
.kw final.
:errbad.
interface example {
    transient int x = 3;
}
:eerrbad.

:MSGSYM. WARN_MD_OUT_OF_ORDER
:MSGTXT. modifier '%s' is out of order
:MSGJTXT. �C���q'%s'�͖����ɂȂ�܂���
:WARNING. 1
:STYLE.
The Java Language Specification contains suggestions
for the ordering of modifiers.
The ordering rules are purely style conventions
and so can be safely ignored.
:errbad.
final public class example {
}
:eerrbad.

:MSGSYM. INF_MD_ORDER_BEFORE
:MSGTXT. should come before '%s'
:MSGJTXT. '%s'�̑O�ɕK�v�ł�
:INFO.
This informational message indicates the modifier
that should precede the modifier in question.

:MSGSYM. WARN_MD_OBSOLETE
:MSGTXT. modifier '%s' is redundant
:MSGJTXT. �C���q'%s'�͗]���ł�
:WARNING. 1
:STYLE.
The Java Language Specification contains suggestions
for the use of modifiers.
Certain modifiers should not be used in new Java
source code and these modifiers are highlighted by this
warning.
The suggestions are purely style conventions
and so can be safely ignored.
:errbad.
abstract public interface example {
    abstract void f();
    public static final int x = 3;
}
:eerrbad.

:MSGSYM. ERR_MD_ABSTRACT_EXCLUDE
:MSGTXT. modifier '%s' conflicts with 'abstract'
:MSGJTXT. �C���q'%s'��'abstract'�ƏՓ˂��܂�
An
.kw abstract
method cannot have the certain modifiers since
the method must be overridden in a subclass.
warning.
:errbad.
public class example {
    abstract private void f1();
    abstract static void f2();
    abstract final void f3();
    abstract native void f4();
    abstract synchronized void f5();
}
:eerrbad.

:MSGSYM. ERR_MD_INVALID_LOCAL_VAR_MOD
:MSGTXT. '%s' is not a valid local variable modifier
:MSGJTXT. '%s'�͖�����۰�ٕϐ��C���q�ł�
The only allowable modifier for a local variable is
.kw final.
:errbad.
class example {
    void f( ) {
        synchronized int f;
    }
}
:eerrbad.

:MSGSYM. ERR_MD_INVALID_PARM_MOD
:MSGTXT. '%s' is not a valid formal parameter modifier
:MSGJTXT. '%s'�͖����ȉ����Ұ��C���q�ł�
The only allowable modifier for a formal parameter is
.kw final.
:errbad.
class example {
    void f( synchronized int f ) {
    }
}
:eerrbad.

:eMSGGRP. MD
:cmt -------------------------------------------------------------------
:MSGGRP. TY
:MSGGRPTXT. Messages related to types
:MSGJGRPTXT. �^�Ɋւ���ү����
:cmt -------------------------------------------------------------------

:MSGSYM. INF_TY_FOUND
:MSGTXT. found type '%T'
:MSGJTXT. �^'%T'��������܂���
:INFO.
Found type reported.

:MSGSYM. INF_TYPE
:MSGTXT. type is '%T'
:MSGJTXT. �^��'%T'�ł�
:INFO.
This informational message indicates the type mentioned in the diagnostic
message.

:MSGSYM. INF_TYPE_LEFT
:MSGTXT. left type is '%T'
:MSGJTXT. ���̌^��'%T'�ł�
:INFO.
This informational message indicates the type mentioned in the diagnostic
message.

:MSGSYM. INF_TYPE_RIGHT
:MSGTXT. right type is '%T'
:MSGJTXT. �E�̌^��'%T'�ł�
:INFO.
This informational message indicates the type mentioned in the diagnostic
message.

:MSGSYM. INF_TYPE_SRC
:MSGTXT. source type is '%T'
:MSGJTXT. ����̌^��'%T'�ł�
:INFO.
This informational message indicates the source type related to the
diagnostic message.

:MSGSYM. INF_TYPE_TGT
:MSGTXT. target type is '%T'
:MSGJTXT. ���ޯĂ̌^��'%T'�ł�
:INFO.
This informational message indicates the target type related to the
diagnostic message.

:MSGSYM. ERR_TY_OBJ_NOT_REF_OR_NULL
:MSGTXT. object does not have reference or null type
:MSGJTXT. ��޼ު�Ă͎Q�Ƃ�null�̌^�������Ă��܂���
The indicated object does not a reference type (or a
.kw null
type).
:errbad.
class example {
    boolean foo( int i ) {
        return i instanceof example;
    }
}
:eerrbad.
The example is erroneous because
.kw instanceof
can only operate upon an object with reference or
.kw null
type.

:MSGSYM. ERR_TY_NOT_REFERENCE
:MSGTXT. type is not a reference type
:MSGJTXT. �^�͎Q�ƌ^�ł͂���܂���
The indicated type is not a reference type.
:errbad.
class example {
    boolean foo( int i ) {
        return this instanceof int;
    }
}
:eerrbad.
The example is erroneous because the type,
.kw int,
 specified with the
.kw instanceof
operator can only be a reference type.

:MSGSYM. INF_TYPE_ARRAY_BASE
:MSGTXT. array base type is '%T'
:MSGJTXT. �z��̊�{�^��'%T'�ł�
:INFO.
This informational message indicates the array base type mentioned in the
diagnostic message.

:MSGSYM. INF_TYPE_RANGE_MAX
:MSGTXT. maximum possible value is %d
:MSGJTXT. �ő�\�l��%d�ł�
:INFO.
This informational message indicates the maximum value possible for the type.

:MSGSYM. INF_TYPE_RANGE_MIN
:MSGTXT. minimum possible value is %d
:MSGJTXT. �ŏ��\�l��%d�ł�
:INFO.
This informational message indicates the minimum value possible for the type.

:MSGSYM. INF_TYPE_EXC
:MSGTXT. exception type is '%T'
:MSGJTXT. ��O�^��'%T'�ł�
:INFO.
This informational message indicates the type of the exception mentioned in the
diagnostic message.

:eMSGGRP. TY
:cmt -------------------------------------------------------------------
:MSGGRP. LB
:MSGGRPTXT. Messages related to labels
:MSGJGRPTXT. ���قɊւ���ү����
:cmt -------------------------------------------------------------------

:MSGSYM. INF_LABEL
:MSGTXT. label is '%s'
:MSGJTXT. ���ق�'%s'�ł�
:INFO.
This informational message indicates the label that was erroneous.

:MSGSYM. ERR_LB_NOT_CONTAINED
:MSGTXT. label not found on containing statement
:MSGJTXT. ���قɗL���ȕ����񂪎w�肳��Ă��܂���
The label referenced in a
.kw break
or
.kw continue
statement was not found on a statement which contains the referencing
statement.
An informational message with the name of the label follows this
diagnostic
:errbad.
int foo()
{
  lab: {}
    for( ; ; ) {
        break lab;
        continue lab;
        break not_defined;
        continue not_defined;
    }
}
:eerrbad.
In the example, all the
.kw break
and
.kw continue
statements are erroneous.

:MSGSYM. ERR_LB_BAD_BREAK
:MSGTXT. unlabeled break statement contained in neither switch nor loop
:MSGJTXT. �����܂���ٰ�߂Ɋ܂܂�Ă��Ȃ����قȂ���ڰ���������܂�
An unlabeled
.kw break
statement was not enclosed by either a loop or
.kw switch
statement.
:errbad.
int foo()
{
    {
        break;
    }
}
:eerrbad.
The
.kw break
statement will be diagnosed.

:MSGSYM. ERR_LB_BAD_CONTINUE
:MSGTXT. unlabeled continue statement not contained in loop
:MSGJTXT. ٰ�߂Ɋ܂܂�Ȃ����قȂ�continue��������܂�
An unlabeled
.kw continue
statement was not enclosed by either a loop.
:errbad.
int foo()
{
    {
        continue;
    }
}
:eerrbad.
The
.kw continue
statement will be diagnosed.

:MSGSYM. ERR_LB_CONTINUE
:MSGTXT. labeled continue statement not contained in loop
:MSGJTXT. ٰ�߂Ɋ܂܂�Ȃ����ٕt��continue��������܂�
A labeled
.kw continue
statement was not enclosed by either a loop.
.kw switch
statement.
:errbad.
int foo()
{
    label:
    {
        continue label;
    }
}
:eerrbad.
The
.kw continue
statement will be diagnosed because the labeled statement was not a loop.

:MSGSYM. ERR_LB_DUPLICATE
:MSGTXT. labeled statement contained in labelled block with same name
:MSGJTXT. ���ٕt�������A�������O�����������ٕt����ۯ����܂�ł��܂�
A labeled statement cannot contain another labeled statement with the
same name.
For example:
:errbad.
int foo()
{
    int i;
    duplabel:
    {
        duplabel:
            i = 1;
    }
}
:eerrbad.
The code fragment is erroneous because the second labeled statement is
contained within the first and both labels have the same name,
.id duplabel,
specified.

:MSGSYM. INF_PREVIOUS_LABEL
:MSGTXT. containing label defined %L
:MSGJTXT. ���ق�'%L'�ł�
:INFO.
This informational message indicates the location of the containing label.

:eMSGGRP. LB
:cmt -------------------------------------------------------------------
:MSGGRP. FD
:MSGGRPTXT. Messages related to fields
:MSGJGRPTXT. ̨���ނɊւ���ү����
:cmt -------------------------------------------------------------------

:MSGSYM. ERR_FD_FINAL_NEEDS_INIT
:MSGTXT. 'final' field requires initializer
:MSGJTXT. 'final'̨���ނɂͲƼ�ײ�ނ��K�v�ł�
A
.kw final
field must be initialized since it cannot be modified after
the class is initialized.
:errbad.
class example {
    static final int x;
};
:eerrbad.

:MSGSYM. ERR_FD_FINAL_VOLATILE
:MSGTXT. 'final' field cannot be 'volatile'
:MSGJTXT. 'final'̨���ނ�'volatile'�͎w��ł��܂���
A
.kw final
field is not allowed to also be a
.kw volatile
field.
:errbad.
class example {
    volatile final int x = 3;
};
:eerrbad.

:MSGSYM. ERR_FD_FWD_STATIC_INIT
:MSGTXT. illegal forward reference to static member
:MSGJTXT. ��è�� ���ނ̕s���ȑO���Q��
Within a static initializer or a class variable initializer, only static
members of the current class that occur textually before the static member
being initialized can be referenced.
:errbad.
class test {
    static {
        fwd = 4;
    }
    static int var = fwd + 2;
    static int fwd;
}
:eerrbad.
The static member
.id fwd
cannot be referenced in the first static initializer since that member is
declared later.
Similarly, the class variable initialization of
.id var
is erroneous because of a forward reference.

:MSGSYM. ERR_FD_FWD_MEMBER_INIT
:MSGTXT. illegal forward reference to instance member
:MSGJTXT. �ݽ�ݽ ���ް�̕s���ȑO���Q��
Initialization expressions for instance members cannot
forward reference instance members.
The reason for this is that the member may not be initialized
properly when its value is used in the initialization expression.
:errbad.
class example {
    int m = f;
    int f = 2;
}
:eerrbad.

:eMSGGRP. FD
:cmt -------------------------------------------------------------------
:MSGGRP. MT
:MSGGRPTXT. Messages related to methods
:MSGJGRPTXT. ҿ��ނɊւ���ү����
:cmt -------------------------------------------------------------------

:MSGSYM. ERR_MT_NO_BODY_ALLOWED
:MSGTXT. method cannot have body
:MSGJTXT. ҿ��ނ͖{�̂����Ă܂���
A
.kw native
or
.kw abstract
method cannot have a body of code specified.
:errbad.
class example {
    native void f() {
    }
};
:eerrbad.

:MSGSYM. ERR_MT_NO_BODY_SPECIFIED
:MSGTXT. method must have body
:MSGJTXT. ҿ��ނɖ{�̂��K�v�ł�
Only
.kw native
or
.kw abstract
methods are allowed to specified
without a body of code.
:errbad.
class example {
    public void f();
};
:eerrbad.

:MSGSYM. ERR_MT_ABSTRACT_ON_NON_ABSTRACT
:MSGTXT. 'abstract' method must be in an 'abstract' class
:MSGJTXT. 'abstract'ҿ��ނ�'abstract'�׽�ɂȂ��Ă͂Ȃ�܂���
A
.kw abstract
method can only be defined within an
.kw abstract
class.
Add the
.kw abstract
modifier to the class definition.
:errbad.
class example {
    public abstract void f();
};
:eerrbad.

:MSGSYM. ERR_MT_CTOR_NAME_MISMATCH
:MSGTXT. constructor name must match class name
:MSGJTXT. �ݽ�׸����Ƹ׽���͓����łȂ��Ă͂Ȃ�܂���
A constructor is a special method with
the same name as the containing class.
The compiler can distinguish a constructor
definition without reference to the name
of the constructor but it checks to make
sure that the name matches the class.
:errbad.
class example {
    constructor() {
    }
};
:eerrbad.

:MSGSYM. INF_MT_CTOR_NAME
:MSGTXT. constructor name is '%i'
:MSGJTXT. �ݽ�׸�����'%i'�ł�
:INFO.
This informational message indicates the name found in the
constructor definition.

:MSGSYM. INF_MT_CLASS_NAME
:MSGTXT. class name is '%i'
:MSGJTXT. �׽����'%i'�ł�
:INFO.
This informational message indicates the name of the
class definition.

:MSGSYM. ERR_MT_MISPLACED_RETURN
:MSGTXT. return statement allowed only in method and constructor bodies
:MSGJTXT. return����ҿ��ނƺݽ�׸��{�݂̂̂Ŏg�p�ł��܂�
A
.kw return
statement was encountered outside the body of a method or a
constructor.
A
.kw return
statement cannot appear within the body of a static constructor.

:MSGSYM. ERR_MT_RETURN_VOID_EXPR
:MSGTXT. return statement cannot have expression
:MSGJTXT. return���͎������Ă܂���
Because the return statement is with a constructor or a method whose
return type is
.kw void,
the
.kw return
statement cannot return a value.
:errbad.
class example {
    example() {
        return 3;
    }
};
:eerrbad.

:MSGSYM. ERR_MT_RETURN_REQD_EXPR
:MSGTXT. return statement requires expression
:MSGJTXT. return���Ɏ����K�v�ł�
The
.kw return
statement occurs within a method whose return type is not
.kw void.
Every
.kw return
statement within the method must specify an expression for a value to
be returned.
:errbad.
class example {
    int fun() {
        return; // needs return value
    }
};
:eerrbad.

:MSGSYM. ERR_MT_DEAD_CODE
:MSGTXT. unreachable statement
:MSGJTXT. ���̕��͎��s����܂���
The indicated location contains a statement which is unreachable. It was
preceded by a
.kw return,
.kw break,
or
.kw continue
statement.
:errbad.
class example {
    int fun() {
        return 5;
        int x = 3;
    }
};
:eerrbad.
The declaration following the
.kw return
statement is unreachable.

:MSGSYM. INF_MT_METHOD_NAME
:MSGTXT. method name is '%S'
:MSGJTXT. ҿ��ޖ���'%S'�ł�
:INFO.
This informational message indicates the name of the method.

:MSGSYM. ERR_MT_DEAD_CATCH
:MSGTXT. 'catch' cannot be reached; preceding block catches all
:MSGJTXT. ����'catch'�͎��s����܂���;�O����ۯ������ׂķ������܂�
The
.kw catch
block cannot be reached.
This is because any exception that could be thrown from within the
corresponding
.kw try
block would be assignable to the
.kw catch
parameter for a preceding
.kw catch
 block for the corresponding
.kw try.

:MSGSYM. INF_MT_CATCH_TYPE
:MSGTXT. catch type is '%T'
:MSGJTXT. catch�^��'%T'�ł�
:INFO.
This informational message indicates the type used in the catch
statement.

:MSGSYM. ERR_MT_UNIMPLEMENTED_METHOD
:MSGTXT. no implementation for method '%D'
:MSGJTXT. ҿ���'%D'����������Ă��܂���
A non-abstract class has an unimplemented interface method.
:errbad.
interface I{
    int foo( int i );
};
class B implements I{
    int a;            //Error no public foo()
};
:eerrbad.

:MSGSYM. ERR_MT_THROWS_MUST_BE_THROWABLE
:MSGTXT. method throws must be a subclass of Throwable
:MSGJTXT. ҿ���throws��Throwable�̻�޸׽�łȂ���΂Ȃ�܂���
Since the only classes that can be thrown in Java
must be subclasses of Throwable,
every type in the
.kw throws
clause of a method must be a subclass of Throwable.
:errbad.
class B {
    void fn() throws B {
    }
};
:eerrbad.

:MSGSYM. INF_MT_THROWS_PROBLEM
:MSGTXT. throws '%T'
:MSGJTXT. throws��'%T'�ł�
:INFO.
This informational message indicates the type
that is not a subclass of Throwable.

:MSGSYM. WARN_THROWS_UNUSED
:MSGTXT. unused throws specifications for method
:MSGJTXT.  ҿ��ނɑ΂��Ďg�p����Ȃ�throws���w�肳��Ă��܂�
:WARNING. 4
The indicated method has types in the throw specification which cannot
be thrown from within the method.
:errbad.
class exc_1 extends java.lang.Exception {}
class exc_2 extends java.lang.Exception {}

class _j1 {
    void foo() throws exc_1, exc_2
    {
        throw new exc_1();
    }
}
:eerrbad.
The method
.id foo
has a throws specification for the type
.id exc_2
which cannot be thrown within the method.

:MSGSYM. INF_THROW_SPEC
:MSGTXT. unused throw type is '%T'
:MSGJTXT. �g�p����Ȃ�throw�̌^�� '%T' �ł�
:INFO.
This informational message indicates the type of the exception
which cannot be thrown from within a method.

:MSGSYM. ERR_MT_NEVER_THROWN
:MSGTXT. 'catch' cannot be reached; exception not thrown
:MSGJTXT. 'catch'�͎��s����܂���;��O�͑��o����܂���
The
.kw catch
block cannot be reached.
This is because any exception that could be thrown from within the
corresponding
.kw try
block is not assignable to the associated
.kw catch
parameter.

:MSGSYM. INF_CATCH_PREVIOUS
:MSGTXT. previous catch is located %L
:MSGJTXT. �O�� 'catch' �� %L �ɂ���܂�
:INFO.
This informational message indicates the location of the previous
.kw catch
block.

:eMSGGRP. MT
:cmt -------------------------------------------------------------------
:MSGGRP. OV
:MSGGRPTXT. Messages related to overloading
:MSGJGRPTXT. ���ް۰�ނɊւ���ү����
:cmt -------------------------------------------------------------------

:MSGSYM. ERR_OV_ATTEMPT_TO_OVERLOAD_RETURN
:MSGTXT. method redefinition has different return type than '%D'
:MSGJTXT. ҿ��ނ̍Ē�`��'%D'�ƈقȂ���return�^�������Ă��܂�
A method cannot be redefine with identical arguments but different return
types.  Return types are not considered in overloading.
:errbad.
class A {
    int foo( int i ) {}
};
class B{
    long foo( int j ) {}
};
:eerrbad.

:MSGSYM. ERR_OV_METHOD_NO_OVERLOAD
:MSGTXT. overloaded method cannot be selected for arguments used in call
:MSGJTXT. �����̌^����v���鵰�ް۰�� ҿ��ނ�����܂���
All of the overloaded methods either have the wrong number of arguments,
or conversions were not possible for an argument to type required in the
prototype.
:errbad.
class A { }
class B {
    void foo( A a ) { }
    void foo( int i ) { }

    void bar ( B b ) {
        foo( b );
    }
}
:eerrbad.
This example is erroneous because class B cannot be converted to either
class A or to int.

:MSGSYM. ERR_OV_METHOD_AMBIGUOUS
:MSGTXT. overloaded method is ambiguous for arguments used in call
:MSGJTXT. �����̌^����v���鵰�ް۰�� ҿ��ނ�����܂���
There is not an unambiguous choice for the method being called.
:errbad.
class B {
    void foo( int i, byte b ) {}
    void foo( byte b, int i ) {}
    void bar ( byte b1, byte b2 ) {
        foo( b1, b2 );
    }
}
:eerrbad.

:MSGSYM. INF_OV_AMBIGUOUS_METHOD
:MSGTXT. ambiguous method is '%D'
:MSGJTXT. �B����ҿ��ނ�'%D'�ł�
:INFO.
This informational message indicates the name of an identifier for which
an error or warning message was diagnosed.

:MSGSYM. ERR_OV_NO_METHOD
:MSGTXT. method not found
:MSGJTXT. ҿ��ނ�������܂���
The indicated method cannot be invoked because there was no declaration of
a method with that name in the context.
:errbad.
class example {
    int foo( ) {
        poo();
    }
};
:eerrbad.
The method
.id poo
cannot be invoked since there was no method by that name declared.

:MSGSYM. INF_OV_METHOD_IDENTIFIER
:MSGTXT. method is '%i'
:MSGJTXT. ҿ��ނ�'%i'�ł�
:INFO.
This informational message indicates the name of a method for which
an error or warning message was diagnosed.

:MSGSYM. ERR_OV_METHOD_NO_MATCH
:MSGTXT. method argument(s) do not match those in prototype
:MSGJTXT. ҿ��ނ̈������������߂ƈ�v���܂���
In the only method with the correct number of arguments, at least
one of the arguments in the call was not able to be converted to the
type required by the prototype.
:errbad.
class A { }
class B {
    void foo( A a ) { }
    void bar ( B b ) {
        foo( b );
    }
}
:eerrbad.
The example is erroneous because the is no conversion from class B to
class A.

:MSGSYM. INF_OV_ARGUMENT_TYPES
:MSGTXT. argument types: '%i%R'
:MSGJTXT. �����̌^�F'%i%R'
:INFO.
This informational message displays the id and type of the arguments
used in the method call.

:MSGSYM. INF_OV_NO_ARGUMENTS_USED
:MSGTXT. there were no arguments used in the call
:MSGJTXT. �Ăяo���Ɏg������������܂���
:INFO.
This informational message indicates that there were no arguments
used in the method call.

:MSGSYM. ERR_OV_METHOD_WRONG_NUM_ARGS
:MSGTXT. number of arguments do not match those in prototype(s)
:MSGJTXT. �����̐����������߂ƈ�v���܂���
No prototype was found with the same number of arguments as was used
in the call.
:errbad.
class A {
    void foo( int i, int j ) {}
    void foo( void ) {}
}
class B {
    void bar ( A a, int i ) {
        a.foo( i );
    }
}
:eerrbad.
The example is erroneous because no method foo in class A which has one
argument can be found.

:MSGSYM. ERR_OV_NOT_ACCESSIBLE
:MSGTXT. method(s) not accessible
:MSGJTXT. ҿ��ނɱ����ł��܂���
Any possible prototype is not accessible
in the call.
:errbad.
class A {
    private void foo( int i, int j ) {}
}
class B {
    void bar ( A a, int i ) {
        a.foo( i,j );
    }
}
:eerrbad.
Although foo matches the call the access is private.

:MSGSYM. ERR_OV_ATTEMPT_TO_HIDE_INSTANCE
:MSGTXT. static method cannot hide instance method '%D'
:MSGJTXT. ��è�� ҿ��ނ��ݽ�ݽ ҿ���'%D'���B�����Ƃ��ł��܂���
A instance method cannot be hidden with a
.kw static
method.
:errbad.
class A {
    int foo( int i ) {}
};
class B extends A{
    static int foo( int i ) {}  //error
};
:eerrbad.

:MSGSYM. ERR_OV_ATTEMPT_TO_OVERRIDE_STATIC
:MSGTXT. instance method cannot override static method '%D'
:MSGJTXT. �ݽ�ݽ ҿ��ނ���è�� ҿ���'%D'���ްײ�ނł��܂���
A instance method cannot be override a
.kw static
method.
:errbad.
class A {
    static int foo( int i ) {}
};
class B extends A{
    int foo( int i ) {}   //error
};
:eerrbad.

:MSGSYM. ERR_OV_ATTEMPT_TO_REDUCE_ACCESS
:MSGTXT. method redefinition has less access than '%D'
:MSGJTXT. ҿ��ނ̍Ē�`��'%D'��豸�������Ȃ��ł�
The access modifier of an overriding or hiding method
must have at least as much access as the overridden or hidden method.
:errbad.
class A {
    protected int foo( int i ) {}
};
class B extends A{
    int foo( int i ) {}   //error
};
:eerrbad.

:MSGSYM. ERR_OV_MORE_THROWS
:MSGTXT. method redefinition throws must be a subclass of '%D'
:MSGJTXT. ҿ��ނ̍Ē�`throws��'%D'�̻�޸׽�łȂ��Ă͂Ȃ�܂���
The throw types of an overriding or hiding method
must be an unchecked exception or a subclass of the overridden or hidden method
:errbad.
class my_exp extends Exception
{
    my_exp(){ super(); }
    my_exp(String s ){ super(s); }
}
class A
{
    int x, y;
    void move( int dx, int dy )
    {
        x+=dx;
        y+=dy;
    }
}
class B extends A
{
    void move( int dx, int dy ) throws my_exp  //error not in A.move
    {
        x+=dx;
        y+=dy;
        if( x < 0 ) throw new my_exp();
    }
}
:eerrbad.

:MSGSYM. ERR_OV_EXTENDS_DIFFERENT_RETURNS
:MSGTXT. methods '%D' and '%D' have different return types
:MSGJTXT. ҿ���'%D'��'%D'�͈قȂ����^��Ԃ��܂�
A class or interface  has inherited more than one method
with the same signature but different return types.
:errbad.
interface B1 { void f(); }
interface B2 { int  f(); }
interface A : extends B1, B2 {
   void a(); // Error void f()  and int f()
}
:eerrbad.

:MSGSYM. INF_OV_OVERIDE_PUBLIC
:MSGTXT. access must be 'public'
:MSGJTXT. ������'public'�łȂ��Ă͂Ȃ�܂���
:INFO.
The symbol must have the indicated access.

:MSGSYM. INF_OV_OVERIDE_PROTECTED
:MSGTXT. access must be 'protected' or 'public'
:MSGJTXT. ������'protected' �� 'public'�łȂ��Ă͂Ȃ�܂���
:INFO.
The symbol must have the indicated access.

:MSGSYM. INF_OV_OVERIDE_DEFAULT
:MSGTXT. access must not be 'private'
:MSGJTXT. ������'private'���g�p�ł��܂���
:INFO.
The symbol must have the indicated access.

:MSGSYM. ERR_OV_ATTEMPT_TO_OVERRIDE_FINAL
:MSGTXT. cannot override final method '%D'
:MSGJTXT. finalҿ���'%D'���ްײ�ނł��܂���
A method cannot be override or hide a
.kw final
method.
:errbad.
class A {
    final int foo( int i ) {}
};
class B extends A{
    int foo( int i ) {}  //error
};
:eerrbad.

:MSGSYM. WARN_OV_NOT_AN_OVERRIDE
:MSGTXT. Method does not override '%D'
:MSGJTXT. ҿ��ނ�'%D'���ްײ�ނ��܂���
:WARNING. 1
A method does not override a method with default access from another package.
:errbad.
package a;
class A {
    int foo( int i ) {}
};
package b;
class B extends A.a {
    int foo( int i ) {}  //warning
};
:eerrbad.

:MSGSYM. WARN_OV_DEPRECATED
:MSGTXT. Override of deprecated method '%D' with non-deprecated
:MSGJTXT.  ��߰Ă���Ă��Ȃ�ҿ��� '%D' ���߰Ă���Ă���ҿ��ނŵ��ްײ�ނ��܂�
:WARNING. 1
The method will not be supported in the next release.
Consult documentation for class on the update
procedure.

:eMSGGRP. OV
:cmt -------------------------------------------------------------------
:MSGGRP. EX
:MSGGRPTXT. Messages related to expressions
:MSGJGRPTXT. ���Ɋւ���ү����
:cmt -------------------------------------------------------------------

:MSGSYM. ERR_EX_NOT_NUMERIC
:MSGTXT. expression is not numeric
:MSGJTXT. �������l�ł͂���܂���
The indicated expression does not have a numeric type.
An informational message indicates the type of the expression.
:errbad.
class A {
    int foo( boolean b ) {
        ++ b;
        return 63;
    }
};
:eerrbad.
The example is erroneous because a
.kw boolean
variable cannot be incremented, since it is not a numeric type.

:MSGSYM. ERR_EX_NOT_BOOLEAN
:MSGTXT. expression is not boolean
:MSGJTXT. ����boolean�ł͂���܂���
The indicated expression does not have a
.kw boolean
type.
An informational message indicates the type of the expression.
:errbad.
class A {
    boolean foo( boolean b, int var ) {
        return b && var;
    }
};
:eerrbad.
The example is erroneous because the variable
.id var
does not have
.kw boolean
type.

:MSGSYM. ERR_EX_NOT_INTEGRAL
:MSGTXT. expression is not integral
:MSGJTXT. ���������ł͂���܂���
The indicated expression does not have an integral type.
An informational message indicates the type of the expression.
:errbad.
class A {
    int foo( boolean b ) {
        return b << 2;
    }
};
:eerrbad.
The example is erroneous because a
.kw boolean
variable cannot be shifted, since it is not an integral type.

:MSGSYM. ERR_EX_BAD_EQUALITY_TYPES
:MSGTXT. invalid types for == or != operator
:MSGJTXT. '=='�܂���'!='���Z�q�ł̌^���s���ł�
The indicated expression does not have valid types for == or !=
comparison.
Informational messages indicate the types of the expressions on the left
and right of the comparison operator.
.np
Both types must be
.kw boolean,
numeric, or reference types.
:errbad.
class A {
    boolean foo( boolean b, int i ) {
        return b > i;
    }
};
:eerrbad.
The example is erroneous because a
.kw boolean
expression cannot be compared with an integral one.

:MSGSYM. ERR_EX_BAD_BITWISE_TYPES
:MSGTXT. bit-wise operand types must be both boolean or both numeric
:MSGJTXT. �ޯĉ��Z�͗�����boolean�܂��͗��������l�łȂ���΂Ȃ�܂���
The indicated expression does not have valid types for
.kw &,
.kw |,
.kw ^,
.kw &=,
.kw |=,
or
.kw ^=
bit-wise operations.
Informational messages indicate the types of the expressions on the left
and right of the bit-wise operator.
.np
Both types must be either
.kw boolean,
or numeric types.
:errbad.
class A {
    int foo( float f1, float f2 ) {
        return f1 & f2;
    }
};
:eerrbad.
The example is erroneous because
.kw float
expression cannot be used with the & operator.

:MSGSYM. ERR_EX_BAD_COLON_TYPES
:MSGTXT. invalid types for ?: operator
:MSGJTXT.  ?:�ɑ΂���^���s���ł�
The indicated expression does not have valid types for the second and third
expressions of the ?: operation.
Informational messages indicate the types of the expressions on the left
and right of the ':' operator.
.np
Both types must be
.kw boolean,
numeric, or reference types.
:errbad.
class A {
    boolean foo( boolean b, int i, boolean b2 ) {
        return b ? i : b2;
    }
};
:eerrbad.
The example is erroneous because
.kw int
and
.kw boolean
types are used with the ':' operation

:MSGSYM. ERR_EX_ASSIGN
:MSGTXT. expression cannot be converted for assignment
:MSGJTXT. ���͑���ɕϊ��ł��܂���
The indicated expression cannot be converted to the type of the
assignment target.
Informational messages indicate the source and target types.
.np
Both types must be
.kw boolean,
numeric, or reference types.
:errbad.
class A {
    boolean foo( int i ) {
        boolean b;
        b = i;
        return b;
    }
};
:eerrbad.
The example is erroneous because an
.kw int
expression cannot be assigned to a
.kw boolean
variable.

:MSGSYM. ERR_EX_RETURN
:MSGTXT. expression cannot be converted to return type
:MSGJTXT. ����return�^�ɕϊ��ł��܂���
The indicated expression cannot be converted to the type of the
assignment target.
Informational messages indicate the source and target types.
.np
Both types must be
.kw boolean,
numeric, or reference types.
:errbad.
class A {
    boolean foo( int i ) {
        return i;
    }
};
:eerrbad.
The example is erroneous because an
.kw int
expression cannot be returned from a
.kw boolean
function.

:MSGSYM. ERR_EX_MISSING_INIT
:MSGTXT. missing initialization value
:MSGJTXT. �����l��������܂���
The initialization value was missing.
:errbad.
int i = { };
:eerrbad.

:MSGSYM. ERR_EX_TOO_MANY_INITS
:MSGTXT. too many initialization values
:MSGJTXT. �����l���������܂�
The initialization value was missing.
:errbad.
int i = { 1, 2 };
:eerrbad.
Only one initialization value is permitted for variable with a primitive type.

:MSGSYM. ERR_EX_MODIFY_FINAL
:MSGTXT. modification of final variable
:MSGJTXT. final�ϐ��̏C��
An attempt to modify a final variable was detected.
:errbad.
class A {
    boolean final var = true;
    void foo() {
        var = false;
    }
};
:eerrbad.
The attempt to set
.id var
to
.kw false
is erroneous since that variable was declared with the
.kw final
modifier.

:MSGSYM. ERR_EX_NOT_DEF_ASSIGNED
:MSGTXT. symbol is not definitely assigned
:MSGJTXT.  ����ق� 'definitely assigned'�ł͂���܂���
At the location indicated, the specified variable has not had a value
"definitely assigned", as specified by the rules for the java language.
These rules are designed to prohibit use of a variable which does not
have a value assigned to it.
.np
The rules state that the all possible execution paths must be examined
to verify that the variable has a value assigned on each path.  The rules
closely approximate a pure flow-of-control analysis, although they are more
conservative.  Exact rules may be obtained by consulting the official
language specification.
:errbad.
class A {
    void foo() {
        int undef;
        int bad = undef;
    }
}
:eerrbad.
In the example, the initialization of variable
.id bad
is erroneous since the initialization expression uses the variable
.id undef
which has not been assigned a value.

:MSGSYM. ERR_EX_CAST
:MSGTXT. expression cannot be converted to type of cast
:MSGJTXT. ���ͷ��Ă̌^�ɕϊ��ł��܂���
The indicated expression cannot be converted to the type of the
cast target.
Informational messages indicate the source and target types.
.np
Both types must be
.kw boolean,
numeric, or reference types.
:errbad.
class A {
    void foo( int i ) {
        boolean b = (boolean)i;
    }
};
:eerrbad.
The example is erroneous because an
.kw int
expression cannot be cast to a
.kw boolean
expression.

:MSGSYM. ERR_EX_METHOD
:MSGTXT. argument expression cannot be converted to type of method parameter
:MSGJTXT. ��������ҿ��� ���Ұ��̌^�ɕϊ��ł��܂���
The indicated argument expression cannot be converted to the type of the
corresponding method parameter.
Informational messages indicate the source and target types.
.np
Both types must be
.kw boolean,
numeric, or reference types.
:errbad.
class A {
    int foo( int i ) {
        return i;
    }
    void bar( boolean b ) {
        foo( b );
    }
};
:eerrbad.
The example is erroneous because an
.kw int
expression cannot be cast to a
.kw boolean
expression.

:MSGSYM. ERR_EX_BAD_DOT_TYPE
:MSGTXT. invalid type for '.' operator
:MSGJTXT.  '.'�̌^���s���ł�
The type of the expression left of the '.' operation must be a
.kw reference
type.
:errbad.
class K {
    int a;
    int b;
};
class A {
    boolean foo() {
        int k;
        return k.b;
    }
};
:eerrbad.
The example is erroneous because
k is an
.kw int
not a and
.kw class.

:MSGSYM. ERR_EX_VALUE_AUTO_INC_DEC
:MSGTXT. ++ and -- cannot be applied to values
:MSGJTXT.  ���̒l��++�܂���--���g�p���邱�Ƃ͂ł��܂���
The indicated
.kw ++
or
.kw --
operator (prefix or postfix) operator was applied to a value.
:errbad.
class example {
    void foo( int i ) {
        ++( i + 2 );
        ( i + 2 )++;
        --( i + 2 );
        ( i + 2 )--;
    }
}
:eerrbad.
All the
.kw ++
and
.kw --
operations in the example are erroneous.

:MSGSYM. ERR_EX_BARE_INSTANCE
:MSGTXT. qualification required for instance variable access
:MSGJTXT.  �ݽ�ݽ�ϐ��ւ̱����������K�v�ł�
An instance variable was used without qualification within a
.kw static
method, a static initializer, or an initializer for a static variable.
:errbad.
class example {
    int inst_var;
    static int foo() {
        return inst_var;
    }
}
:eerrbad.
The
.kw return
statement cannot reference the instance variable
.id inst_var
since the method
.id foo
is
.kw static.

:MSGSYM. ERR_EX_NOT_DATA
:MSGTXT. data expression required in this context
:MSGJTXT. ���̺�÷�Ăɂ��ް������K�v�ł�
A non-data expression was encountered in a context in which data is required.
:errbad.
class example {
    int foo() {
        return foo;
    }
}
:eerrbad.
The
.kw return
statement references the method
.id foo
which is not data.

:MSGSYM. ERR_EX_DIM_EXPR_TYPE
:MSGTXT. promoted type of dimension expression is not 'int'
:MSGJTXT.  �z��̓Y����'int'�ł͂���܂���
The type (after promotion) of a dimension expression must be
.kw int.
:errbad.
class MyClass {
    int myvar;
}
class example {
    void foo( long size ) {
        MyClass[] array = new array[ size ];
    }
}
:eerrbad.
In the example, the dimension expression is erroneous since it has a
.kw long
type.

:MSGSYM. ERR_EX_NEW_ABSTRACT
:MSGTXT. cannot create an instance of an abstract type
:MSGJTXT. ���ی^�̲ݽ�ݽ���쐬�ł��܂���
The type specified following the
.kw new
keyword is an abstract type.
It is illegal to attempt to create an instance of an abstract type.
:errbad.
class myAbstract {
    static myAbstract create() {
        return new myAbstract();
    }
}
:eerrbad.
In the example, the
.kw return
statement contains a
.kw new
expression for an abstract type and so is erroneous.

:MSGSYM. ERR_EX_SUPER_OF_OBJECT
:MSGTXT. cannot use 'super' with class 'Object'
:MSGJTXT. 'super'�Ƹ׽'Object'�𓯎��Ɏg�p���邱�Ƃ͂ł��܂���
This error can occur only if the
.kw super
keyword is used in the
.id Object
class.
It is not expected that a user would compile this class.

:MSGSYM. ERR_EX_BAD_SUPER
:MSGTXT. 'super' cannot be used in this context
:MSGJTXT. 'super'�͂��̺�÷�Ăɂ͎g�p�ł��܂���
.kw super
can be used only within an instance method, a constructor, or the
initializer of an instance variable.
:errbad.
class example {
    static example foo() {
        return super;
    }
}
:eerrbad.
The example is erroneous since the
.kw return
statement expression for a
.kw static
method contains the
.kw super
keyword.

:MSGSYM. ERR_EX_BAD_THIS
:MSGTXT. 'this' cannot be used in this context
:MSGJTXT. 'this'�͂��̺�÷�Ăɂ͎g�p�ł��܂���
.kw this
can be used only within an instance method, a constructor, or the
initializer of an instance variable.
:errbad.
class example {
    static example foo() {
        return this;
    }
}
:eerrbad.
The example is erroneous since the
.kw return
statement expression for a
.kw static
method contains the
.kw this
keyword.

:MSGSYM. ERR_EX_UNCAUGHT_EXC
:MSGTXT. no error handling for exception
:MSGJTXT. ��O�̴װ����������܂���
A checked exception can be thrown at the indicated location and no error
handling was specified for the exception.
Error handling can be specified in two ways.
When the exception can be thrown from within a
.kw try
block, the exception can be caught when there is a
.kw catch
block whose
.kw catch
parameter has a type which is assignable from the type of the exception.
The exception can also be handled if there is a type in a throws clause,
for the current method or constructor,
which is assignable from the type of the exception.
:errbad.
class my_exc extends( Exception ) {
};
class my_examp {
    void foo() {
        throw my_exc();
    }
}
:eerrbad.

:MSGSYM. ERR_EX_STATIC_NEEDED
:MSGTXT. field '%P' must be static within this context
:MSGJTXT. ̨����'%P' �͂��̺�÷�ē��ł͐ÓI�łȂ��Ă͂Ȃ�܂���
This result left of the '.' is a type name
so only a static field may follow.

:MSGSYM. ERR_EX_NOT_ARRAY
:MSGTXT. array reference expression is not an array
:MSGJTXT. �z��Q�Ǝ��͔z��ł͂���܂���
The expression to the left of the opening bracket is not an array.
:errbad.
class my_examp {
    void foo() {
        int var = 4;
        int j = var[0]; // var is not an array
    }
}
:eerrbad.
The attempt to use
.id var
as the array reference expression is an error because it is not an array.

:MSGSYM. ERR_EX_MUST_BE_INT
:MSGTXT. promoted type of expression must be 'int'
:MSGJTXT. ���̌^��'int'�łȂ��Ă͂Ȃ�܂���
The indicated expression does not have a promoted type of
.kw int.
For example, the value used as the index expression in an array access
expression must have this promoted type.
:errbad.
class my_examp {
    float foo( float arr[], long index ) {
        return arr[ index ];
    }
}
:eerrbad.
The
.kw array
.id arr
cannot be indexed using the variable
.id index
since it has a type
.kw float.

:MSGSYM. ERR_EX_ARRAY_INIT_TYPE
:MSGTXT. array initialization expression is not assignable to array base type
:MSGJTXT. �z��̏������Ŏg�p����Ă��鎮���z��̌^�ƈقȂ�܂�
The indicated initialization expression is not assignable to an array element.
:errbad.
class bad {
    static void init_fun() {
        int[] arr = { true, false };
    }
}
:eerrbad.
In the example,
.kw boolean
initialization values are given to initialize an array of
.kw int
values.
This is erroneous because a
.kw boolean
value cannot be assigned to an
.kw int
item.

:MSGSYM. ERR_EX_INT_DIVIDE_BY_ZERO
:MSGTXT. integer division by zero
:MSGJTXT. �돜�Z���������܂���
Division by zero is not allowed in an integer expression.
:errbad.
class bad {
    static void foo(int i) {
        int j = i / 0;
        int k = 1 / 0;
        int m = i % 0;
        int n = 1 % 0;
    }
}
:eerrbad.
In the example, division by zero errors occur in all four expressions.

:MSGSYM. ERR_EX_ARRAY_TO_SIMPLE
:MSGTXT. array initialization expression is not assignable to non-array type
:MSGJTXT.  �z��̏������Ɏg�p����Ă��鎮���s���ł�
The indicated array initialization expression cannot be assigned to the
indicated non-array type.
Except for the Object type, an array initialization expression must be
assigned to an object with an array type.
:errbad.
class bad {
    static int foo() {
        int[] array = { {1,2}, 3 };
        return array[1];
    }
}
:eerrbad.
In the contrived example, the inner array initialization expression is
erroneous because it is being assigned to an object whose type is
.kw int.

:MSGSYM. WARN_EX_EXCESS_SHIFT
:MSGTXT. actual shift amount will be %u
:MSGJTXT. ����ėʂ�%u�ł�
:WARNING. 3
This warning indicates the actual shift amount which was specified as a
either a negative  constant or a positive constant which is different
than the actual shift amount.
The actual shift amount is determined to be N low-order bits from the
given shift amount.
The value of N is determined by the promoted type of the left operand and
will be 5 for an
.kw int
operand and 6 for a
.kw long
operand.
:errbad.
class warned {
    void foo()
    {
        int iv = 3;
        long lv = 5;
        int w1 = iv << 33;
        int w2 = iv << -1;
        long w3 = lv << 69;
        long w4 = lv << -2;
    }
}
:eerrbad.
All the shift operations in the example will generate warnings.

:MSGSYM. ERR_EX_BARE_TYPE_CALL
:MSGTXT. method '%P' must be static when qualified with <TypeName>
:MSGJTXT. <TypeName>�t���Ő�������Ă���ꍇҿ���'%P'�͐ÓI�łȂ���΂Ȃ�܂���
A qualified name of the form TypeName.Identifier()
was used on a method that is not
.kw static
which is illegal since a class instance is needed.
:errbad.
interface k {
    void f()
    {
    }
}
class example {
    int foo() {
        k.f();              //illegal call f is abstract
        example.foo();      // no instance variable
    }
}
:eerrbad.

:MSGSYM. ERR_EX_BARE_CALL
:MSGTXT. method '%P' must be static within this context
:MSGJTXT. ҿ���'%P'�͂��̺�÷�ē��ł͐ÓI�łȂ��Ă͂Ȃ�܂���
A method call was used without qualification within a
.kw static
method, a static initializer, or an initializer for a static variable.
:errbad.
class example {
    int inst_func(){
        return( 0 );
    }
    static int foo() {
        return inst_func();
    }
}
:eerrbad.

:MSGSYM. ERR_EX_BARE_ABSTRACT_CALL
:MSGTXT. attempt to directly invoke abstract method '%P'
:MSGJTXT.  ����ҿ��� '%P' �𒼐ڌĂяo���܂���
A direct method call was used on an abstract function.
.kw static
method, a static initializer, or an initializer for a static variable.
:errbad.
class a {
    abstract void foo();
}
class example {
     void foo() {
        super.foo();
    }
}
:eerrbad.

:MSGSYM. WARN_EX_STRING_COMPARES_REFERENCES
:MSGTXT. reference equality used: string contents will not be compared
:MSGJTXT. �Q�Ɠ������g�p�F������̓��e�͔�r����܂���
:WARNING. 1
The compiler has detected an equality comparison involving the type
.kw java.lang.String
that will only compare the
.kw String
references for equality.
The references may not be equal even though the
.kw String
contents may be identical (see the Java Language Specification 15.20.3).
If this was the intent, the warning can be ignored or disabled.
If a full comparison of the
.kw String
contents was required, the method
.kw equals
can be used.
:errbad.
class example {
    boolean reference_compare( String x ) {
        return x == "asdf";
    }
    boolean content_compare( String x ) {
        return x.equals( "asdf" );
    }
};
:eerrbad.

:MSGSYM. WARN_EX_MEANINGLESS_COMPARISON_FALSE
:MSGTXT. comparison is meaningless for type involved (always false)
:MSGJTXT. �قȂ�^�̔�r�ł�(���false�ɂȂ�܂�)
:WARNING. 4
The compiler has detected a comparison that can never be true for
any value of the type involved.
:errbad.
class example {
    void foo( byte b ) {
        if( b > 127 ) {
            System.out.println("Byte value too large");
        }
    }
}
:eerrbad.
The comparison in the example will always yield false because the maximum
value for byte is 127.

:MSGSYM. WARN_EX_MEANINGLESS_COMPARISON_TRUE
:MSGTXT. comparison is meaningless for type involved (always true)
:MSGJTXT.  �قȂ�^�̔�r�ł�(���true�ɂȂ�܂�)
:WARNING. 4
The compiler has detected a comparison that can never be false for
any value of the type involved.
:errbad.
class example {
    void foo( byte b ) {
        if( b <= 127 ) {
            System.out.println("Byte value not too large");
        }
    }
}
:eerrbad.
The comparison in the example will always yield true because the maximum
value for byte is 127.

:MSGSYM. ERR_EX_TYPE_EXPECTING_DATA
:MSGTXT. expecting data but type was specified
:MSGJTXT. �ް���K�v�Ƃ��Ă��܂����^���w�肳��܂���
A type was used at a place where data was expected.
:errbad.
class Test {
    foo() {
        int i = Test;
    }
}
:eerrbad.
In the example, the initialization of the variable
.id i
is erroneous because a type was specified to the right of the assignment
operator.

:MSGSYM. ERR_EX_PACKAGE_EXPECTING_DATA
:MSGTXT. expecting data but package was specified
:MSGJTXT. �ް���K�v�Ƃ��Ă��܂����߯���ނ��w�肳��܂���
A package name was used at a place where data was expected.
:errbad.
package MyPackage;
class Test {
    foo() {
        int i = MyPackage;
    }
}
:eerrbad.
In the example, the initialization of the variable
.id i
is erroneous because a package was specified to the right of the assignment
operator.

:MSGSYM. ERR_EX_EXPECTING_DATA
:MSGTXT. expecting data value
:MSGJTXT. �ް��̒l���K�v�ł�
A data value was expected but something else was encountered.

:MSGSYM. ERR_EX_DECIMAL_LITERAL_INVALID
:MSGTXT. decimal literal must be operand of unary '-' operator
:MSGJTXT.  �޼�� ���ْP�� '-'���Z�q�̂ݎg�p�ł��܂�
The literals 2147483648 and 9223372036854775807L represent
the magnitude of the maximum negative value that can be
represented in the
.kw int
and
.kw long
types.
The Java language requires that these literals only be used
as the operand of a unary minus ('-') operator.
:errbad.
class example {
    int x = 0-2147483648; // error
    int y = -2147483648;  // OK
};
:eerrbad.

:MSGSYM. ERR_EX_FLOAT_LITERAL_TOO_LARGE
:MSGTXT. floating point literal is too large
:MSGJTXT. �����������ق��傫�����܂�
The largest allowed floating point literal is 3.40282346638528860e+38f for
the
.kw float
type and 1.79769313486231570e+308 for the
.kw double
type.  Any literal larger than these
values is in error.
:errbad.
class example {
    float f = 3.40282346638528860e+38f; // ok
    f = 3.5e+38f; // error

    double d = 1.79769313486231570e+308; // ok
    d = 1.8798e+308; // error

};
:eerrbad.

:MSGSYM. ERR_EX_FLOAT_LITERAL_TOO_SMALL
:MSGTXT. floating point literal is too small
:MSGJTXT. �������������������܂�
The smallest allowed floating point literal is 1.40129846432481707e-45f
for the
.kw float
type and 4.94065645841246544e-324 for the
.kw double
type.  Any literal smaller than these values is in error.
:errbad.
class example {
    float = 1.40129846432481707e-45f;
    f = 1.4e-45f; // error

    double = 4.94065645841246544e-324; // ok
    d = 4.9e-324; // error

};
:eerrbad.

:MSGSYM. ERR_EX_INSTANCEOF
:MSGTXT. instanceof expression cannot be converted
:MSGJTXT. instanceof���͕ϊ��ł��܂���
The indicated expression cannot be converted to the
.kw instanceof
target type.
Informational messages indicate the source and target types.
The message is often the result of using a type that is
not related to the type of the expression.
:errbad.
class B {
}
class A {
    boolean fn( A p ) {
        return( p instanceof B );
    }
}
:eerrbad.

:MSGSYM. ERR_EX_FINAL_LOOP_INIT
:MSGTXT. final symbol cannot be assigned value in loop
:MSGJTXT. final����ق�ٰ�߂Ɏg�p���邱�Ƃ͂ł��܂���
The indicated symbol was assigned a value within a loop, but was
declared outside the loop.
This can cause the final symbol to be assigned a value twice or more,
which is not permitted.
:errbad.
class A {
    int foo( int value ) {
        final int val;
        while( value ) {
            val = 1997;
            -- value;
        }
    }
}
:eerrbad.

:MSGSYM. ERR_EX_FINAL_INIT_TWICE
:MSGTXT. final symbol cannot be assigned value more than once
:MSGJTXT. final����قւ̑���͈�x�����ł��܂���
The indicated symbol was assigned a value twice or more,
which is not permitted.
:errbad.
class A {
    int foo( int value ) {
        final int val;
        val = 10;
        val = 11;
    }
}
:eerrbad.

:MSGSYM. INF_FINAL_PREVIOUS
:MSGTXT. previous assignment is located %L
:MSGJTXT. �O��̑����%L�ɂ���܂�
:INFO.
This informational message indicates the location of the previous
assignment to the final variable.

:MSGSYM. ERR_EX_FINAL_INIT_INCOMPLETE
:MSGTXT. final symbol is neither definitely assigned nor unassigned
:MSGJTXT. final����ق͑������邩����Ȃ����ǂ��炩�������Ƃ�܂�
The indicated blank
.kw final
variable is neither definitely assigned nor definitely unassigned at
the end of the initialization block.
.np
A blank
.kw final class
variable must be definitely assigned within a
.kw static
initialization block and so a subsequent assignment will be required.
This assignment will be erroneous because there is a potential execution
path with more than one assignment to the variable.
.np
Similar considerations apply to blank
.kw final instance
variables, which must be definitely assigned by an instance initialization
or by the end of each constructor.
:errbad.
class A {
    public static final boolean dbg = true;
    public static final int stat_var;
    static {
        if( dbg ) {
            stat_var = 19;
        }
    }
    public final int inst_var;
    {
        if( dbg ) {
            inst_var = 23;
        }
    }
    public A( int a )
    {
        super();
        inst_var = 16;
    }
}
:eerrbad.
The variables,
.id stat_var
and
.id inst_var
will be diagnosed at the end of the initialization blocks in which they
are assigned values.
The assignment within the constructor will also be diagnosed as a second
assignment to a
.kw final
variable.

:MSGSYM. ERR_EX_FINAL_NEVER
:MSGTXT. final symbol was never initialized
:MSGJTXT. final����ق�����������Ă��܂���
The indicated
.kw final
variable was never assigned a value.
This is required for
.kw class
and
.kw instance
variables that were declared to be
.kw final.
:errbad.
class A {
    public static final int stat_var;
    public final int inst_var;
}
:eerrbad.
In the example, both
.kw final
variables require initialization.

:eMSGGRP. EX
:cmt -------------------------------------------------------------------
:MSGGRP. LK
:MSGGRPTXT. Messages related to symbol lookup
:MSGJGRPTXT. ����ٌ����Ɋւ���ү����
:cmt -------------------------------------------------------------------

:MSGSYM. ERR_LK_AMBIGUOUS_LOOKUP
:MSGTXT. ambiguous lookup for '%i'
:MSGJTXT. '%i'�̞B����ٯ�����
This error indicates that the lookup found more than
one symbol when it should have found exactly one symbol.
The lookup must be disambiguated in order for the code
to be accepted.
:errbad.
interface ii {
    int i = 3;
};
class base implements ii {
    int i;
};
public class test extends base implements ii {
    int foo() {
        return i;
    }
};
:eerrbad.

:MSGSYM. INF_LK_AMBIGUOUS_CANDIDATE
:MSGTXT. possible candidate: '%D'
:MSGJTXT. �\���̂�����F'%D'
:INFO.
This informational message indicates one of the candidate
symbols that could have satisfied the lookup.


:MSGSYM. ERR_LK_STATIC_NEEDED
:MSGTXT. member '%S' must be static in this context
:MSGJTXT. ����'%S'�͂��̺�÷�Ăł͐ÓI�łȂ��Ă͂Ȃ�܂���
This informational message indicates one of the candidate
symbols that could have satisfied the lookup.

:MSGSYM. INF_LK_REJECTED_CANDIDATE
:MSGTXT. rejected candidate: '%D'
:MSGJTXT. ���ۂ��ꂽ���F'%D'
:INFO.
This informational message indicates one of the candidate
symbols that did not satisfied the lookup due to having the wrong number
of arguments.

:MSGSYM. WARN_LK_DEPRECATED
:MSGTXT. method '%D' has been deprecated
:MSGJTXT. ҿ���'%D'�ͻ�߰Ă���Ă��܂���
:WARNING. 1
The method will not be supported in the next release.
Consult documentation for class on the update
procedure.

:eMSGGRP. LK
:cmt -------------------------------------------------------------------
:MSGGRP. CI
:MSGGRPTXT. Messages related to class and interface declarations
:MSGJGRPTXT. �׽�Ʋ��̪����`�Ɋւ��Ă�ү����
:cmt -------------------------------------------------------------------

:MSGSYM. ERR_CI_JAVA_LANG_OBJECT_EXTENDS
:MSGTXT. 'java.lang.Object' cannot extend a type
:MSGJTXT. 'java.lang.Object'�͌^���g���ł��܂���
The class 'java.lang.Object' cannot have an
.kw extends
clause because it is the root of the Java object hierarchy.
:errbad.
package java.lang;
public class Object extends foo {
}
:eerrbad.

:MSGSYM. ERR_CI_JAVA_LANG_OBJECT_IMPLEMENTS
:MSGTXT. 'java.lang.Object' cannot implement an interface
:MSGJTXT. 'java.lang.Object'�Ͳ��̪���������ł��܂���
The class 'java.lang.Object' cannot have an
.kw implements
clause because it is the root of the Java object hierarchy.
:errbad.
package java.lang;
public class Object implements foo {
}
:eerrbad.

:MSGSYM. ERR_CI_FINAL_CANT_BE_SUBCLASS
:MSGTXT. class extends a 'final' class
:MSGJTXT. �׽��'final'�׽���g�����܂�
A
.kw final
class cannot be used in an
.kw extends
clause because the
.kw final
modifier indicates to the Java compiler
that the class will never be extended.
:errbad.
final class ff {
}
class example extends ff {
}
:eerrbad.

:MSGSYM. ERR_CI_INTERFACE_CANT_BE_SUBCLASS
:MSGTXT. class extends an interface
:MSGJTXT. �׽�Ͳ��̪�����g�����܂�
An
.kw interface
name cannot be used in an
.kw extends
clause, only a
.kw class
name can be used.
:errbad.
interface i {
}
class example extends i {
}
:eerrbad.

:MSGSYM. INF_CI_SUPERCLASS
:MSGTXT. superclass is '%T'
:MSGJTXT. ���߰�׽��'%T'�ł�
:INFO.
This informational message indicates what type
is being erroneously used in the
.kw extends
clause.

:MSGSYM. ERR_CI_CLASS_CANT_BE_SUBINTERFACE
:MSGTXT. class implements a class
:MSGJTXT. �׽�͸׽���������܂�
A
.kw class
name cannot be used in an
.kw implements
clause, only an
.kw interface
name can be used.
:errbad.
class c {
}
class example implements c {
}
:eerrbad.

:MSGSYM. ERR_CI_DUPLICATE_SUPERINTERFACE
:MSGTXT. interface directly implemented more than once
:MSGJTXT. ���̪�����Q��ȏ㒼�ړI�Ɏ������Ă��܂�
A type
cannot directly implement an
.kw interface
more than once.
:errbad.
package wrong;
interface i {
}
interface j {
}
class example implements i, j, i, wrong.j {
}
:eerrbad.

:MSGSYM. INF_CI_SUPERINTERFACE
:MSGTXT. superinterface is '%T'
:MSGJTXT. ���߰���̪����'%T'�ł�
:INFO.
This informational message indicates what type
is being erroneously used in the
.kw implements
clause.

:MSGSYM. ERR_CI_OWN_SUBCLASS
:MSGTXT. class extends itself
:MSGJTXT. �׽���������g���g�����Ă��܂�
A type cannot extend itself
producing a cyclic inheritance relationship.
:errbad.
class a : extends c {
}
class b : extends a {
}
class c : extends b {
}
:eerrbad.

:MSGSYM. ERR_CI_COULD_NOT_LOCATE
:MSGTXT. class '%S' could not be found
:MSGJTXT. �׽'%S'��������܂���
The class in question could not be defined through
locating either a Java source file, a Java Class File,
or a Java Class File in an archive.
:errbad.
class example : extends unknown {
}
:eerrbad.

:MSGSYM. WARN_CI_NO_TYPES_DEFINED
:MSGTXT. no types defined in source file
:MSGJTXT. ��� ̧�قɌ^����`����Ă��܂���
:WARNING. 1
The source file did not contain any type
definitions so no output file can be generated.
:errbad.
/*
class example {
}
*/
:eerrbad.

:MSGSYM. ERR_CI_OWN_SUBINTERFACE
:MSGTXT. interface extends itself
:MSGJTXT. ���̪�����������g���g�����Ă��܂�
An interface cannot extend itself
producing a cyclic inheritance relationship.
:errbad.
interface a extends c {
}
interface b extends a {
}
interface c extends b {
}
:eerrbad.

:eMSGGRP. CI
:cmt -------------------------------------------------------------------
:MSGGRP. CF
:MSGGRPTXT. Messages related to Class File processing
:MSGJGRPTXT. �׽ ̧�ُ����Ɋւ���ү����
:cmt -------------------------------------------------------------------

:MSGSYM. ERR_CF_UNEXPECTED_CLASS
:MSGTXT. file contains unexpected class
:MSGJTXT. ̧�قɗ\�����ʸ׽������܂�
Upon processing of a class file or source file, the compiler
detected that the class that it was expecting could
not be found.

:MSGSYM. INF_CF_WANTED_CLASS
:MSGTXT. wanted '%T'
:MSGJTXT. '%T'���K�v�ł�
:INFO.
This informational message indicates what class
was expected in the file.

:MSGSYM. INF_CF_FOUND_CLASS
:MSGTXT. found '%T'
:MSGJTXT. '%T'��������܂���
:INFO.
This informational message indicates what class
was found in the file.

:MSGSYM. INF_CF_INPUT_FILE
:MSGTXT. file '%s'
:MSGJTXT. ̧��'%s'
:INFO.
This informational message indicates the
name of the file.

:MSGSYM. ERR_CF_COULD_NOT_MAP
:MSGTXT. cannot produce valid file name from type
:MSGJTXT. �^����L����̧�ٖ����쐬�ł��܂���
The compiler detected that it could not produce
a file system name for the indicated type.
The type name combined with the package name
produced a file name that exceeds the limits
imposed by the host operating system.
Try to reduce the size of package names and
type names to prevent this error.

:MSGSYM. INF_CF_MAP_TOO_LONG
:MSGTXT. '%S' too long
:MSGJTXT. '%S'�͒������܂�
:INFO.
This information message indicates the
type symbol that produces a file system
name that exceeds the limits imposed by
the host operating system.

:MSGSYM. INF_CF_COMBINED_WITH_PATH
:MSGTXT. combined with '%s'
:MSGJTXT. '%s'�ƌ�������܂���
:INFO.
This informational message indicates the
path name that caused a problem when it
was used to construct a valid file name.

:MSGSYM. ERR_CF_COULD_NOT_CREATE_CLASSFILE
:MSGTXT. cannot open class file for output
:MSGJTXT. �o�͗p�׽ ̧�ق��J���܂���
The compiler could not create the file
when it attempted to write the class file.

:MSGSYM. INF_CF_CLASSFILE_PATH
:MSGTXT. class file '%s'
:MSGJTXT. �׽ ̧�� '%s'
:INFO.
This informational message indicates the
path name of the class file that could
not be created.  Verify that all subdirectories
are valid and that the class file can
be written.

:MSGSYM. ERR_CF_METHOD_GENS_TOO_MUCH
:MSGTXT. method body is too large for classfile
:MSGJTXT. ҿ��ޖ{�̂��傫�����ĸ׽ ̧�ق��쐬�ł��܂���
The compiler found a method that resulted in more than 65535
bytes of Java Virtual Machine code.  Shorten the method or
break it up into multiple methods.

:MSGSYM. INF_CF_METHOD_SYMBOL
:MSGTXT. method: %S
:MSGJTXT. ҿ���:%S
:INFO.
This informational message indicates the method that caused
the problem in the original error message related to classfile
output.

:MSGSYM. INF_CF_METHOD_LENGTH
:MSGTXT. size of byte codes: %u
:MSGJTXT. �޲� ���ނ̻���:%u
:INFO.
This informational message indicates how many bytes
of Java Virtual Machine code was generated for the method.

:MSGSYM. ERR_CF_METHOD_BAD_EH_INFO
:MSGTXT. classfile format cannot represent exception handling for method
:MSGJTXT. �׽ ̧�� ̫�ϯĂ�ҿ��ނ̗�O������\���܂���
The compiler found a method that resulted in more than 65534
bytes of Java Virtual Machine code combined with the use of
exception handling through a
.kw try
or
.kw synchronized
statement.
The classfile format cannot represent the exception handling
information for this particular method.
The current classfile format uses small integers for the
exception handling information thus limiting the size of methods that
require exception handling tables to 65534 bytes.

:MSGSYM. ERR_CF_ZIP_LOAD_PROBLEM
:MSGTXT. could not load class definition from ZIP file
:MSGJTXT. ZIP ̧�ق���׽��`��ǂݍ��߂܂���
The compiler encountered a problem loading a
class file from a ZIP archive.

:MSGSYM. INF_CF_LOADING_CLASS
:MSGTXT. trying to load '%S'
:MSGJTXT. '%S'��ǂݍ������Ƃ��Ă��܂�
:INFO.
This informational message indicates the type that
was being loaded when the problem occurred.

:MSGSYM. WARN_CF_WRONG_FILE
:MSGTXT. public class '%i' name does not match file name
:MSGJTXT.  ����د� �׽'%i'�̖��O��̧�ٖ��ƈ�v���܂���
:WARNING. 3
A public class name should match the source file name
to aid in searching for classes.

:MSGSYM. ERR_CF_OVERWRITE_CLASS
:MSGTXT. overwriting class file for '%T'
:MSGJTXT. '%T'�̂��߂̸׽ ̧�ق��㏑�����Ă��܂�
Operating system restriction do not recognize case sensitive names
in files so two classes will write to the same class file

:MSGSYM. INF_CF_OVERWRITE_BY
:MSGTXT. overwritten by class '%T'
:MSGJTXT. �׽'%T'�ɂ���ď㏑������܂���
:INFO.
This informational message indicates the actual
type that caused the classfile to be overwritten.

:MSGSYM. ERR_CF_LOAD_BAD_ACCESS
:MSGTXT. invalid access flags in classfile
:MSGJTXT.  �׽̧�ق̱��� �׸ނ��s���ł�
When reading a class file incorrect access flags where found
produced by an incorrect or outdated compiler.

:MSGSYM. WARN_CF_LOAD_PRIVATE_PROTECTED_ACCESS
:MSGTXT. private protected access made public
:MSGJTXT.  ��ײ�ް� ���øï�ޱ�����public�ɂȂ�܂�
:WARNING. 1
When reading a class file a private protected access flag was found
produced by an outdated compile.

:MSGSYM. ERR_CF_WRITE_ERROR_DURING_OUTPUT
:MSGTXT. write error detected when writing class file
:MSGJTXT.  �׽̧�ق̏������ݒ��ɏ������ݴװ���������܂���
The compiler could not write out the entire
class file due to a system write error.

:MSGSYM. ERR_CF_FORMAT_ERROR
:MSGTXT. class file format limits exceeded
:MSGJTXT.
The compiler could not write out a
class file because the Java class exceeded a
format restriction in the class file format.

:eMSGGRP. CF
:cmt -------------------------------------------------------------------
:MSGGRP. ST
:MSGGRPTXT. Messages related to statements
:MSGJGRPTXT. ���Ɋւ���ү����
:cmt -------------------------------------------------------------------

:MSGSYM. INF_SWITCH_LOCATION
:MSGTXT. 'switch' statement is located %L
:MSGJTXT. 'switch'���� %L �ɂ���܂�
:INFO.
This informational message indicates the location of the current
.kw switch
statement referenced in a diagnostic message.

:MSGSYM. INF_CASE_LOCATION
:MSGTXT. 'case' label is located %L
:MSGJTXT. 'case'���ق� %L�ɂ���܂�
:INFO.
This informational message indicates the location of a
.kw case
label referenced in a diagnostic message.

:MSGSYM. INF_DEFAULT_LOCATION
:MSGTXT. 'default' label is located %L
:MSGJTXT. 'default'���ق� %L �ɂ���܂�
:INFO.
This informational message indicates the location of a
.kw default
label referenced in a diagnostic message.

:MSGSYM. ERR_ST_SWITCH_EXPR_TYPE
:MSGTXT. type of switch expression must be char, byte, short, or int
:MSGJTXT.  �������̎���char, byte, short, int�̂����ꂩ�łȂ���΂Ȃ�܂���
The
.kw switch
expression had a type other than one indicated in the message.
An informational message will display the type of the expression.
:errbad.
class bad {
    void fun( boolean arg ) {
       switch( arg ) {
         case true :
           return 1;
         case false :
           return 0;
        }
    }
}
:eerrbad.
The example is erroneous since it has a
.kw switch
expression and constant expressions for the
.kw case
labels which are
.kw boolean.

:MSGSYM. ERR_ST_CASE_EXPR_TYPE
:MSGTXT. type of expression with case label must be char, byte, short, or int
:MSGJTXT.  case���ق̎��� char, byte, short, int�̂����ꂩ�łȂ��Ă͂Ȃ�܂���
The expression with the
.kw case
label had a type other than one indicated in the message.
An informational message will display the type of the expression.
:errbad.
class bad {
    void fun( boolean arg ) {
       switch( arg ) {
         case true :
           return 1;
         case false :
           return 0;
        }
    }
}
:eerrbad.
The example is erroneous since it has a
.kw switch
expression and constant expressions for the
.kw case
labels which are
.kw boolean.

:MSGSYM. ERR_ST_CASE_NONCONSTANT
:MSGTXT. case expression is not constant
:MSGJTXT.  case�����萔�ł͂���܂���
The indicated expression was not a constant expression.

:MSGSYM. ERR_ST_CASE_DUPLICATE
:MSGTXT. case expression is duplicate
:MSGJTXT.  case�����d�����Ă��܂�
The indicated expression was already used in a
.kw case
label for the current
.kw switch
statement.

:MSGSYM. ERR_ST_DEFAULT_DUPLICATE
:MSGTXT. default label is duplicated for switch
:MSGJTXT. ��̫�� ���قͽ����̂��߂ɕ�������Ă��܂�
A
.kw default
label has already occurred for the current
.kw switch
statement.

:MSGSYM. ERR_ST_RETURN_REQD
:MSGTXT. method requires 'return' statement
:MSGJTXT. ҿ��ނ�'return'����K�v�Ƃ��܂�
The current method did not complete with a
.kw return
statement, since it is declared to not have a
.kw void
return.
:errbad.
class example {
    int foo() {
       poo();
    }
    int poo() {
       return 98;
    }
}
:eerrbad.
The function
.id foo
is required to complete with a
.kw return
statement.

:MSGSYM. ERR_ST_NOT_THROWABLE
:MSGTXT. type of expression is not assignable to 'Throwable'
:MSGJTXT.  ���̌^�� 'Throwable' �Ɏg�p�ł��܂���
The indicated type of the throw expression is not assignable to
the type
.id Throwable,
as is required.
Practically, this means that the type should extend that type.
:errbad.
class example {
    static void throw_int() {
        throw 19;
    }
}
:eerrbad.
In the example, the
.kw throw
of the
.kw int
expression is diagnosed, since an
.kw int
is not derived from the class
.id Throwable.

:MSGSYM. ERR_ST_CASE_NOT_ASSIGNABLE
:MSGTXT. case expression is not assignable to 'switch' expression
:MSGJTXT.  case����'switch'���ɓK�����Ă��܂���
The indicated constant expression could not be assigned to the expression used
in the associated
.kw switch
statement.
:errbad.
class example {
    static void fun( byte b ) {
        switch( b ) {
          case 1 :      // ok
          case 10000 :  // not assignable
        }
    }
}
:eerrbad.
The second
.kw case
label expression is not assignable to a
.kw byte
type and so is erroneous.

:MSGSYM. ERR_ST_CTOR_CALLS_SELF
:MSGTXT. constructor invokes self
:MSGJTXT. �ݽ�׸��͎��g���Ăяo���Ă��܂�
The explicit constructor invocation invokes the constructor which contains it.
:errbad.
class bad {
    int var;
    bad( int i ) {
        this( i + 1 );
        var = i;
    }
}
:eerrbad.
The example is erroneous because the explicit constructor invocation invokes
the constructor which contains it.

:MSGSYM. ERR_ST_CTOR_CALLS_INDIRECT
:MSGTXT. constructor invokes self indirectly
:MSGJTXT. �ݽ�׸��͎��g���ިڸ�؂��Ăяo���Ă��܂�
The explicit constructor invocation invokes the constructor which contains it,
by invoking a chain of constructors which ultimately invoke the original
constructor.
The chain of constructors is indicated by a list of informational messages
which follow this diagnostic.
:errbad.
class bad {
    int var1;
    int var2;
    bad( int i ) {
        this( i, 1 );
        var1 = i;
    }
    bad( int i, int j ) {
        this( i );
        var2 = j;
    }
}
:eerrbad.
The example is erroneous because the explicit constructor invocation within
.id bad( int )
invokes the constructor
.id bad( int, int )
which invokes the original constructor.

:MSGSYM. INF_CTOR
:MSGTXT. constructor: %S
:MSGJTXT. �ݽ�׸�: %S
:INFO.
This information message indicates a constructor which is erroneous.

:MSGSYM. INF_CTOR_INVOCATION
:MSGTXT. invokes constructor: %S
:MSGJTXT. �ݽ�׸����Ăяo��: %S
:INFO.
This information message indicates a constructor which is invoked by the
constructor in the preceding informational message.

:MSGSYM. ERR_ST_RETURN_STAT_INIT
:MSGTXT. static initializer cannot contain return statement
:MSGJTXT. ��è�� �Ƽ�ײ�ނ�return �����܂߂܂���
A
.kw return
statement is not allowed within a
.kw static
initializer.
:errbad.
class Sample {
    static int i;
    static int j;
    static { i = 23; j =19; return; }
}
:eerrbad.
The example is erroneous because a
.kw return
statement is found within the
.kw static
initializer.

:MSGSYM. ERR_ST_EXC_IN_INIT
:MSGTXT. initializer expression cannot cause checked exception
:MSGJTXT.  �Ƽ�ײ�ނ̎��͗�O�������ł��܂���
An initializer expression cannot allow the possibility that a checked
exception can b thrown.
:errbad.
class Exc extends Exception {
}
class test {
    int foo() throws Exc
    {
        throw new Exc();
    }
    int i = foo();
}
:eerrbad.
The example is erroneous because the initializer invokes a function
.id foo
which could allow a checked exception
.id Exc
to be thrown.

:MSGSYM. ERR_ST_EXC_IN_STAT_INIT
:MSGTXT. static initializer must catch checked exception
:MSGJTXT. ��è�� �Ƽ�ײ�ނ͗�O���������Ȃ���΂Ȃ�܂���
A static initializer cannot allow the possibility that a checked
exception can be thrown and not handled within the initializer.
:errbad.
class Exc extends Exception {
}
class test {
    static int i;
    int foo() throws Exc
    {
        throw new Exc();
    }
    static
    {
        i = foo();
    }
}
:eerrbad.
The example is erroneous because the static initializer invokes a function
.id foo
which could allow a checked exception
.id Exc
to be thrown.

:MSGSYM. ERR_ST_EXPL_CTOR_THIS
:MSGTXT. cannot use 'this' in explicit constructor call
:MSGJTXT. �����I�ݽ�׸� ��ق�'this'�͎g�p�ł��܂���
The keyword
.kw this
cannot be used in an explicit constructor call statement.
:errbad.
class Sample {
    float f;
    Sample( float init ) {
        f = init;
    }
    Sample()
        this( this.f );
    }
}
:eerrbad.
The explicit constructor call within the second constructor is erroneous
because it uses
.kw this
in the expression for the first parameter.

:MSGSYM. ERR_ST_EXPL_CTOR_SUPER
:MSGTXT. cannot use 'super' in explicit constructor call
:MSGJTXT. �����I�ݽ�׸� ��ق�'super'�͎g�p�ł��܂���
The keyword
.kw super
cannot be used in an explicit constructor call statement.
:errbad.
class Sample {
    float f;
    Sample( float init ) {
        f = init;
    }
}
class Another {
    float fv;
    Another() {
        super( super.f );
    }
}
:eerrbad.
The explicit constructor call within the constructor for
.id Another
is erroneous because it uses
.kw super
in the expression for the first parameter.

:MSGSYM. ERR_ST_EXPL_CTOR_INSTANCE
:MSGTXT. cannot use instance variable in explicit constructor call
:MSGJTXT. �����I�ݽ�׸� ��قɲݽ�ݽ�ϐ��͎g�p�ł��܂���
An instance variable for the object being initialized (including those in
super class(es)) cannot be used in an explicit constructor call statement.
:errbad.
class Sample {
    float f;
    Sample( float init ) {
        f = init;
    }
    Sample()
        this( f );
    }
}
:eerrbad.
The explicit constructor call within the second constructor is erroneous
because it uses the instance variable
.id f
in the expression for the first parameter.

:MSGSYM. ERR_ST_EXPL_CTOR_METHOD
:MSGTXT. cannot use instance method in explicit constructor call
:MSGJTXT. �����I�ݽ�׸� ��قɲݽ�ݽ ҿ��ނ��g�p�ł��܂���
An instance method for the object being initialized (including those in
super class(es)) cannot be invoked in an explicit constructor call statement.
:errbad.
class Sample {
    float getDefault() {
        return f;
    }
    float f;
    Sample( float init ) {
        f = init;
    }
    Sample()
        this( getDefault() );
    }
}
:eerrbad.
The explicit constructor call within the second constructor is erroneous
because it uses the instance method
.id getDefault
in the expression for the first parameter.

:MSGSYM. ERR_ST_RETURN_INST_INIT
:MSGTXT. instance initializer cannot contain return statement
:MSGJTXT. �ݽ�ݽ �Ƽ�ײ�ނ�return���͎g�p�ł��܂���
A
.kw return
statement is not allowed within an
.kw instance
initializer.
:errbad.
class Sample {
    int i;
    int j;
    { i = 23; j =19; return; }
}
:eerrbad.
The example is erroneous because a
.kw return
statement is found within the
.kw instance
initializer.

:MSGSYM. ERR_ST_INSTANCE_EXC
:MSGTXT. throws clause omits exception type used in instance initializer
:MSGJTXT.  �ݽ�ݽ �Ƽ�ײ�ޓ���throw���ȗ�������O���g�p���Ă��܂�
A checked exception was thrown or could be thrown within an
.kw instance
initializer
at the indicated location.
The
.kw throws
clause for the constructor contained neither this type nor the type of
a super object for this type.
:errbad.
class Exc extends java.lang.Throwable {
};
class Sample {
    int i;
    int j;
    { i = 23; throw new Exc(); }
    Sample() {
        super();
        j = 19;
    }
}
:eerrbad.
The example is erroneous because a checked exception of type
.id Exc
was thrown from within the
.kw instance
initializer and there was no
.kw throws
clause with the constructor.

:MSGSYM. INF_ST_PREVIOUS_THROW
:MSGTXT. exception can be thrown %L
:MSGJTXT. ��O�� %L �ɑ��o����܂�
:INFO.
This informational message indicates the source location
where an exception of the indicated type could be thrown.

:MSGSYM. WARN_ST_UNREACHABLE_FOR_UPDATE
:MSGTXT. update portion of 'for' statement will never be executed
:MSGJTXT. 'for'���̈ꕔ�����s����܂���
:WARNING. 1
The update portion of the
.kw for
statement will never be executed.  Although this is allowed
by the Java language, it is often not what the programmer intended.
:errbad.
class Sample {
    void fn( int m ) {
        for( int i = 0; i < m; ++i ) {
            return;
        }
    }
}
:eerrbad.

:eMSGGRP. ST
:cmt -------------------------------------------------------------------
:MSGGRP. CG
:MSGGRPTXT. Messages related to code generation
:MSGJGRPTXT. ���ސ����Ɋւ���ү����
:cmt -------------------------------------------------------------------

:MSGSYM. ERR_CG_INITIALIZATION
:MSGTXT. cannot initialize code generator
:MSGJTXT. ���޼ު�ڰ����������ł��܂���
The code generator component did not initialize properly.
This may indicate that the java compiler was not properly installed.

:MSGSYM. INF_CG_CODESIZE
:MSGTXT. code size: %u
:MSGJTXT. ���޻���:%u
:INFO.
This informational message indicates the size of the compiled code.

:MSGSYM. ERR_CG_ERROR
:MSGTXT. error during code generation
:MSGJTXT. ���ސ������̴װ
This message indicates that a non-fatal (recoverable) error was detected
during code generation.
An informational message provides text describing the error.
This may indicate that the java compiler was not properly installed.

:MSGSYM. ERR_CG_INTERNAL_ERROR
:MSGTXT. internal error %u during code generation
:MSGJTXT. ���ސ������̓����װ%u
This message indicates that a non-fatal (recoverable) error was detected
during code generation.
This may indicate that the java compiler was not properly installed or may
indicate that a bug has been detected.

:MSGSYM. ERR_CG_FATAL
:MSGTXT. fatal error during code generation
:MSGJTXT. ���ސ������̒v���I�ȴװ
This message indicates that a fatal (non-recoverable) error was detected
during code generation.
An informational message provides text describing the error.
This may indicate that the java compiler was not properly installed.

:MSGSYM. INF_CG_MESSAGE
:MSGTXT. message is '%s'
:MSGJTXT. ү���ނ�'%s'
:INFO.
This informational message indicates the type of error detected during code
generation.

:MSGSYM. ERR_CG_IMPOSSIBLE
:MSGTXT. internal compiler error during code generation
:MSGJTXT. ���ސ������̓����I���߲� �װ
This message should never appear.

:MSGSYM. WARN_CG_OPTIMIZATION
:MSGTXT. insufficient memory for complete optimization
:MSGJTXT. �œK���ɕK�v����؂��m�ۂł��܂���
:WARNING. 3
This message indicates that there was insufficient memory available to permit
complete optimization of the object code.

:eMSGGRP. CG
:cmt -------------------------------------------------------------------
:MSGGRP. AC
:MSGGRPTXT. Messages related to access
:MSGJGRPTXT. �����Ɋւ���ү����
:cmt -------------------------------------------------------------------

:MSGSYM. ERR_AC_PRIVATE
:MSGTXT. cannot access private member %P
:MSGJTXT. ��ײ�ް� ���� %P �ɱ����ł��܂���
Access is only permitted from within the type of where the member is declared.
:errbad.
class t1{
    private int a;
}
class t2 extends t1{
    void f(){
        a = 1;
    }
}
:eerrbad.

:MSGSYM. ERR_AC_DEFAULT
:MSGTXT. cannot access member %P (default access)
:MSGJTXT. ���� %P �ɱ����ł��܂���(��̫�� ����)
Access is only permitted from within the package of where the member is declared.
:errbad.
package p1;
public class t1{
   int a;
}
-----------------
package p2;
class t2 extends p1.t1{
    void f(){
        a = 1;
    }
}
:eerrbad.

:MSGSYM. ERR_AC_PROTECTED
:MSGTXT. cannot access protected member %P
:MSGJTXT. ���ø� ���� %P �ɱ����ł��܂���
Access is only permitted from within the package of where the member is declared or
from an implementation of that object.
:errbad.
package p1;
public class t1{
   protected int a;
}
-----------------
package p2;
class t2 extends p1.t1{
    void f(t2 p ){
        p.a = 1;   //okay f implements type t2
    }
    void f( p1.t1 p ){
        p.a = 1;   //error f does not implement type t1
    }
}
:eerrbad.

:MSGSYM. ERR_AC_INACCESSIBLE_CLASS
:MSGTXT. class %P is inaccessible
:MSGJTXT. �׽ %P �ɂͱ����ł��܂���
Access is only permitted from within the package of where the class is declared.
:errbad.
package p1;
class t1{
   int a;
}
-----------------
package p2;
class t2 extends p1.t1{
   int b;
}
:eerrbad.

:eMSGGRP. AC
:cmt -------------------------------------------------------------------
:MSGGRP. CP
:MSGGRPTXT. Messages related to compilation progress
:MSGJGRPTXT. ���߲� ��۸�ڽ�Ɋւ���ү����
:cmt -------------------------------------------------------------------

:MSGSYM. WARN_CP_PARSING_FILE
:MSGTXT. parsing file '%s'
:MSGJTXT. ̧�� '%s'����͒�
:WARNING. 1
This message indicates that the specified file
is undergoing the initial parse to create the
internal representation of the class described
in the source file.

:MSGSYM. WARN_CP_LOADING_CLASS
:MSGTXT. loading definition for '%S'
:MSGJTXT. '%S'�̒�`��ǂݍ��ݒ�
:WARNING. 1
This message indicates that the definition
for the class is required and it is being
read from the repository (i.e., file system
or database).

:MSGSYM. WARN_CP_COMPILING_CLASS_METHODS
:MSGTXT. compiling methods for '%S'
:MSGJTXT. '%S'��ҿ��ނ���ْ߲�
:WARNING. 1
This message indicates that the method bodies
of the class are being compiled (i.e., analyzed).

:MSGSYM. WARN_CP_WRITING_CLASS
:MSGTXT. writing '%s'
:MSGJTXT. '%s'���������ݒ�
:WARNING. 1
This message indicates that a classfile is being written
out to the file system.

:MSGSYM. INF_CP_LOADING_CLASS_FILE
:MSGTXT. class found in '%s'
:MSGJTXT. �׽��'%s'�Ō�����܂���
:INFO.
This informational message indicates the file name
of the Java classfile that contains the class definition.

:MSGSYM. INF_CP_LOADING_CLASS_ARCHIVE
:MSGTXT. class found in archive '%s'
:MSGJTXT. �׽��������'%s'�Ō�����܂���
:INFO.
This informational message indicates the file name
of the Java archive that contains the class definition.

:MSGSYM. WARN_CP_CLASSFILE_WRITTEN
:MSGTXT. classfile name: '%s'
:MSGJTXT. �׽ ̧�ٖ�:'%s'
:WARNING. 1
This message indicates that a classfile is being written
out to the file system with the specified name.

:MSGSYM. INF_CP_CLASSFILE_SIZE
:MSGTXT. classfile size: %u
:MSGJTXT. �׽ ̧�� ����:%u
:INFO.
This message indicates that the size of the classfile being written.

:MSGSYM. WARN_CP_METHOD_CODE
:MSGTXT. method name: '%S'
:MSGJTXT. ҿ��ޖ�:'%S'
:WARNING. 1
This message indicates the name of a method for which
Java byte codes have been generated.

:MSGSYM. INF_CP_CODE_ATTRIBUTE_SIZE
:MSGTXT. code attribute size: %u
:MSGJTXT. ���ޑ�������:%u
:INFO.
This message indicates that the size of the Java byte code
generated for the method.

:MSGSYM. WARN_CP_COMPILATION_TIME
:MSGTXT. compilation finished in %C
:MSGJTXT. %C�ł̺��߲ق��I�����܂���
:WARNING. 1
This message displays the total time required to
compile all the Java source files.

:MSGSYM. WARN_CP_PARSE_TIME
:MSGTXT. parse finished in %C
:MSGJTXT. %C�ŉ�͂��I�����܂���
:WARNING. 1
This message displays the total time required to
parse the Java source file.

:MSGSYM. INF_CP_PARSE_TIME_FILE
:MSGTXT. file parsed was '%s'
:MSGJTXT. ��͂��ꂽ̧�ق�'%s'�ł�
:INFO.
This message indicates the file that just finished parsing.

:MSGSYM. WARN_CP_LOAD_TIME
:MSGTXT. class load finished in %C
:MSGJTXT. '%C'�Ÿ׽�̓ǂݍ��݂��I�����܂���
:WARNING. 1
This message displays the total time required to
load the definition of a class from the repository.

:MSGSYM. INF_CP_LOAD_TIME_TYPE
:MSGTXT. type loaded was '%S'
:MSGJTXT. �ǂݍ��܂ꂽ�^��'%S'�ł�
:INFO.
This message indicates the type that just finished loading.

:MSGSYM. WARN_CP_ANALYSIS_TIME
:MSGTXT. class method analysis finished in %C
:MSGJTXT.  %C�Ÿ׽ ҿ��މ�͂��I�����܂���
:WARNING. 1
This message displays the total time required to
check and process the methods of a class.

:MSGSYM. INF_CP_ANALYSIS_TIME_TYPE
:MSGTXT. type compiled was '%S'
:MSGJTXT. ���߲ق��ꂽ�^��'%S'�ł�
:INFO.
This message indicates the type that just finished being analysed.

:MSGSYM. WARN_CP_WRITE_TIME
:MSGTXT. class method output finished in %C
:MSGJTXT. %C�Ÿ׽ ҿ��ޏo�͂��I�����܂���
:WARNING. 1
This message displays the total time required to
output the class definition to the classfile.

:MSGSYM. INF_CP_WRITE_TIME_TYPE
:MSGTXT. type written out was '%S'
:MSGJTXT. �����ꂽ�^��'%S'�ł�
:INFO.
This message indicates the type that was just written out to the repository.
:eMSGGRP. CP

:cmt -------------------------------------------------------------------
:MSGGRP. IN
:MSGGRPTXT. Messages related to inner classes
:MSGJGRPTXT. �����׽�Ɋւ���ү����
:cmt -------------------------------------------------------------------

:MSGSYM. ERR_IN_CANT_HAVE_SAME_NAME
:MSGTXT. nested class has same name as '%S'
:MSGJTXT. Ƚè��� �׽��'%S'�Ɠ������O�ł�
A nested class cannot have the same name as an
enclosing class.  This restriction allows code
to unambiguously refer to any enclosing class
instance with the
.kw class-name . this
construct.
:errbad.
class example {
    class example {
    };
};
:eerrbad.

:MSGSYM. ERR_IN_CANT_BE_STATIC
:MSGTXT. inner class member cannot be 'static'
:MSGJTXT. �����׽ ���ް��'static'���w�肷�邱�Ƃ͂ł��܂���
Inner classes cannot have
.kw static
class variables or class methods.
The static members can be moved into the
innermost enclosing top-level class
(i.e., a package-level class or a nested class
with the
.kw static
modifier).
:errbad.
class example {
    class nested {
        static int a;
        static int f() {
            return 1;
        }
    };
};
:eerrbad.

:MSGSYM. ERR_IN_NO_STATIC_INIT
:MSGTXT. inner class cannot have static initializer
:MSGJTXT. �����׽�ͽ�è�� �Ƽ�ײ�ނ����Ă܂���
Inner classes cannot have
.kw static
initializer blocks since
they cannot have
.kw static
members.
:errbad.
class example {
    class nested {
        static {
        }
    };
};
:eerrbad.

:MSGSYM. ERR_IN_INNER_HAS_STATIC_CLASS
:MSGTXT. inner class cannot contain top-level class
:MSGJTXT. �����׽��į������ �׽�������Ƃ͂ł��܂���
Inner classes cannot have
.kw static
(or top-level)
classes defined within them.
:errbad.
class example {
    class inner {
        // default 'static'
        interface I {
        };
        static class S {
        };
    };
};
:eerrbad.

:MSGSYM. ERR_IN_INVALID_THIS_CLASS
:MSGTXT. invalid 'this' type qualifier '%T'
:MSGJTXT.  �C���q '%T' �ɑ΂���s���� 'this'
Qualifying the
.kw this
reference with a class type can only
be done with types that are enclosing
inner classes.
The immediate class and any class
that contains an inner class form
the set of classes that can qualify
the
.kw this
reference.
:errbad.
class C {
    static class S {
        class I {
            void foo() {
                C.this.c = 1; // error
                S.this.s = 1;
                I.this.i = 1;
            }
            int i;
        };
        int s;
    };
    int c;
};
:eerrbad.

:MSGSYM. ERR_IN_NO_ENCLOSING_INSTANCE
:MSGTXT. only an inner class can have enclosing instance specified
:MSGJTXT. �������̪���Ͳ�Ű �׽�ɂ̂ݎw�肷�邱�Ƃ��ł��܂�
Specifying the enclosing instance in the explicit
constructor invocation can only be done if the class
being constructed is an inner class.
All other classes do not need an enclosing instance specified.
:errbad.
class C {
    static class S {
        S( C p ) {
            p.super();
        }
    };
};
:eerrbad.

:MSGSYM. ERR_IN_NEW_QUAL_INVALID
:MSGTXT. 'new' can be qualified only when allocating an inner class
:MSGJTXT.  'new'�Ͳ�Ű �׽���쐬���鎞�ɂ̂ݎg�p�ł��܂�
A
.kw new
operator can be qualified only when the object being allocated is
an inner class.
The qualifier must be a class object whose type encloses the type
of the object being allocated.
:errbad.
class X {
}
class Y {
    void foo() {
        X var = X.new X();
    }
}
:eerrbad.

:MSGSYM. ERR_IN_NEW_QUAL_IMPOSSIBLE
:MSGTXT. no enclosing instance when allocating an inner class
:MSGJTXT. �����׽�̊��蓖�Ē��ɁA�����ݽ�ݽ��������܂���
An unqualified
.kw new
of an inner class was specified.
There is no
.kw this
instance that can be associated with the allocated object.
:errbad.
class Z {
    class Inner {
    };
    static void foo() {
        Inner var = new Inner();
    }
}
:eerrbad.
The
.kw new
is erroneous since a static routine has no
.kw this
associated with it.

:MSGSYM. INF_IN_NEW_QUAL_TYPE
:MSGTXT. type of 'new' qualifier is '%T'
:MSGJTXT.  'new'�̌^�̏C���q��'%T'�ł�
:INFO.
This message indicates the type of the
.kw new
qualifier.

:MSGSYM. ERR_IN_SUPER_QUAL_IMPOSSIBLE
:MSGTXT. no enclosing instance for a super inner class
:MSGJTXT. ���߰�����׽�̂��߂̕����ݽ�ݽ������܂���
There was no enclosing instance for the
.kw super
constructor which is being invoked.
:errbad.
class j01 {
    class J01_inner {
    };
}
class test {
    class test_inner extends j01.J01_inner {
    }
}
:eerrbad.
The class
.id test.test_inner
requires a default constructor to be generated.
This results in an error because there is not enclosing
instance for the class
.id j01.J01_inner.

:MSGSYM. ERR_IN_CANT_ACCESS_NON_FINAL_LOCAL
:MSGTXT. cannot access '%S' since it is not 'final'
:MSGJTXT. '%S'��'final'�ł͂Ȃ��̂ű����ł��܂���
A method in a local class cannot access a local
variable from its host method unless the local
variable is
.kw final.
This effectively prevents one from being misled
into thinking that modification of the local
variable is meaningful.
:errbad.
class example {
    void fn( int a ) {
        class L {
            void m1() {
                a = 1; // error
                ++a;   // error
            }
        };
    }
}
:eerrbad.

:MSGSYM. ERR_IN_NEW_FROM_STATIC_METHOD
:MSGTXT. cannot allocate an inner class from a static method
:MSGJTXT. ��è�� ҿ��ނ����Ű �׽���쐬���邱�Ƃ͂ł��܂���
An unqualified
.kw new
of an inner class was specified.
There is no
.kw this
instance that can be associated with the allocated object.
:errbad.
class Z {
    class Inner {
    };
    static void foo() {
        Inner var = new Inner();
    }
}
:eerrbad.
The
.kw new
is erroneous since a static routine has no
.kw this
associated with it.

:MSGSYM. ERR_IN_NEW_FROM_STATIC_INIT
:MSGTXT. cannot allocate an inner class during static initialization
:MSGJTXT. ��è�����������Ͳ�Ű �׽���쐬�ł��܂���
An unqualified
.kw new
of an inner class was specified.
There is no
.kw this
instance that can be associated with the allocated object.
:errbad.
class Z {
    class Inner {
    };
    static {
        Inner var = new Inner();
    }
}
:eerrbad.
The
.kw new
is erroneous since a static initialization has no
.kw this
associated with it.

:MSGSYM. WARN_IN_COULD_HAVE_BEEN_STATIC
:MSGTXT. inner class '%T' can be made 'static'
:MSGJTXT. ��Ű �׽ '%T' �� 'static' �ō쐬����܂�
:WARNING. 1
The compiler has detected that you are using an inner class that
could instead be a nested top-level class.
If you change it to a top-level class, by prefixing the class definition
with the
.kw static
keyword, then instantiation of the class will be more efficient.
You should not change the definition if you are still developing the class,
in case future changes prevent it from being a top-level class,
or if you want to maintain binary compatibility with classfiles
that depend on the current class definition.
:errbad.
class Z {
    class Inner {
        int m;
        int getM() {
            return m;
        }
    };
}
:eerrbad.

:MSGSYM. ERR_IN_AMBIGUOUS_LEXID
:MSGTXT. ambiguity: '%i' found in both super class and enclosing class
:MSGJTXT. �����܂��ł�: '%i'�����߰�׽�ƕ����׽�����ɑ��݂��܂�
The compiler is required to detect potentially problematic
uses of single identifiers that occur in the context of
nested or inner classes.
If a name is found through inheritance that could also be found
in a lexically enclosing non-package scope, the compiler is
required to report an ambiguity.
The ambiguity can be resolved through the use of a qualified
or unqualified
.kw this
expression that specifies which class should be used
in the member reference.
:errbad.
class example {
    static class I1 {
        void foo( I1 a ) {
        }
    }
    static class I2 extends I1 {
        void foo( int a ) {
        }
    }
    static class I3 {
        void foo( I1 a ) {
        }
        void foo( int a ) {
        }
        class II4 extends I2 {
            void bar( I1 p ) {
                foo( p );       // error I1.foo or I3.foo?
                I3.this.foo( p ); // ok
                this.foo( p ); // ok
            }
        }
    }
}
:eerrbad.

:eMSGGRP. IN
