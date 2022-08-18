:cmt GML Macros used:
:cmt
:cmt    :chain. <char> <usage>                  options that start with <char>
:cmt                                            can be chained together i.e.,
:cmt                                            -oa -ox -ot => -oaxt
:cmt    :option. <option> <synonym> ...         define an option
:cmt    :target. <arch1> <arch2> ...            valid for these architectures
:cmt    :ntarget. <arch1> <arch2> ...           not valid for these architectures
:cmt    :immediate. <fn>                        <fn> is called when option parsed
:cmt    :code. <source-code>                    <source-code> is executed when option parsed
:cmt    :enumerate. <field> [<value>]           option is one value in <name> enumeration
:cmt    :number. [<fn>] [<default>]             =<n> allowed; call <fn> to check
:cmt    :id. [<fn>]                             =<id> req'd; call <fn> to check
:cmt    :char.[<fn>]                            =<char> req'd; call <fn> to check
:cmt    :file.                                  =<file> req'd
:cmt    :dir.                                   =<dir> req'd
:cmt    :path.                                  =<path> req'd
:cmt    :special. <fn> [<arg_usage_text>]       call <fn> to parse option
:cmt    :optional.                              value is optional
:cmt    :negate.                                for a simple switch turns off option
:cmt    :noequal.                               args can't have option '='
:cmt    :argequal. <char>                       args use <char> instead of '='
:cmt    :internal.                              option is undocumented
:cmt    :prefix.                                prefix of a :special. option
:cmt    :usage. <text>                          English usage text
:cmt    :jusage. <text>                         Japanese usage text
:cmt    :title.                                 English usage text
:cmt    :jtitle.                                Japanese usage text
:cmt    :timestamp.                             kludge to record "when" an option
:cmt                                            is set so that dependencies
:cmt                                            between options can be simulated

:cmt    where:
:cmt        <arch>:     i86, 386, axp, any, dbg, qnx

:title. Usage: wjava [options] file [options]
:jtitle. �g�p�@: wjava [��߼��] ̧�� [��߼��]
:target. any
:title. Options:
:jtitle. �I�v�V����:
:target. any
:title. \t    ( /option is also accepted )
:jtitle. \t    ( /option���g�p�ł��܂� )
:target. any
:ntarget. qnx
:title. \t    ( '=' is always optional, i.e., -w4 )
:jtitle. \t    ( '='�͏�ɏȗ��\�ł��B�܂� -w4 )
:target. any
:title. \t    ( a trailing '[-]' disables option where accepted )
:jtitle. \t    ( �I�[�� '[-]' ����̫�ĵ�߼�݂��g�p���܂� )
:target. any

:chain. m display message output
:jusage. m ү���ޏo�͂�\������

:option. br
:target. any
:usage. generate browser information in classfile
:jusage.

:option. cp classpath
:target. any
:path.
:usage. set class path for compilation
:jusage. ���߲ق̂��߂ɸ׽ �߽��ݒ肵�Ă�������

:option. cp:i
:target. any
:usage. ignore CLASSPATH environment variable
:jusage. CLASSPATH���ϐ��𖳎�����

:option. cp:p
:target. any
:path.
:usage. prepend path to class path
:jusage. �׽�߽�̑O�Ɏw�肳�ꂽ�߽���g�p����

:option. cp:o
:target. any
:negate.
:usage. print classpath
:jusage. �׽�߽��\������

:option. d
:target. any
:dir.
:usage. set root directory for class file output
:jusage. �׽ ̧�ُo�͂̂��߂�ٰ� �ިڸ�؂�ݒ�

:option. deprecation
:target. any
:usage. warn about use of a deprecated API
:jusage.  deprecated API�g�p���Ɍx����\������

:option. ef
:target. any
:file.
:optional.
:usage. set error file name
:jusage. �װ ̧�ٖ���ݒ肷��

:option. eq
:target. any
:negate.
:immediate. HandleOptionEQ
:usage. do not display error messages (but still write to .err file)
:jusage. �װ ү���ނ�\�����Ȃ�(.err ̧�قɂ͏o��)

:option. ew
:target. any
:negate.
:immediate. HandleOptionEW
:usage. alternate error message formatting
:jusage. ���ݴװ ү���� ̫�ϯ�

:option. e
:target. any
:number. CheckErrorLimit
:usage. set limit on number of error messages
:jusage. �װ ү���ނ̐��ɐ�����t����

:option. g
:target. any
:negate.
:usage. generate full debugging info (lines + variables)
:jusage. ���ׂĂ����ޯ�ޏ��𐶐�����(�s�ԍ�+�ϐ�)

:option. g:l
:target. any
:negate.
:usage. generate line number debugging info
:jusage. �s�ԍ����ޯ�ޏ��𐶐�����

:option. g:t
:target. any
:negate.
:usage. generate debug tables i.e. local variables
:jusage. ���ޯ�ޕ\�̐��������� �� ۰�ٕϐ�

:option. ic
:target. any
:negate.
:usage. ignore case differences in the file system
:jusage.  ̧�ٖ��̑啶���A�������𖳎�����

:option. javadoc
:target. any
:usage. temporary -- enable JAVADOC PROCESSING
:jusage.
:internal.

:option. jck
:target. any
:usage. compiling JCK source code
:jusage. JCK ��� ���ނ���ْ߲�
:internal.

:option. k
:target. any
:negate.
:usage. continue processing files (ignore errors)
:jusage. ̧�ق̏����𑱍s(�װ�𖳎�)

:option. kw
:target. any
:negate.
:usage. write out classfiles for correct source files
:jusage.

:option. m
:target. any
:usage. default = -mpst
:jusage. ��̫�� = -mpst

:option. mi
:target. any
:usage. internal statistics
:jusage. ������͌���
:internal.

:option. mp
:target. any
:usage. progress messages
:jusage. ��۸�ڽ ү����

:option. ms
:target. any
:usage. sizes
:jusage. ����

:option. mt
:target. any
:usage. timings
:jusage. ���ݸ�

:option. nowarn
:target. any
:usage. turn off warnings
:jusage. �x��������
:enumerate. warn_level

:option. nowrite
:target. any
:usage. compile only - do not generate class files
:jusage. ���߲ق̂� - �׽ ̧�ق𐶐����Ȃ�

:option. o
:target. any
:negate.
:usage.  full optimization (except for -o:rp)
:jusage. ���ׂĂ̍œK��(-o:rp������)

:option. o:i
:target. any
:negate.
:usage.  optimize by inlining static, private, and final methods
:jusage. ��ײ� static�Aprivate�A�� final ҿ��ނł̍œK��

:option. o:ib
:target. any
:number. CheckInlineLimit 32
:usage. restrict inlining to maximum <num> bytes of instructions
:jusage. �ݽ�׸��݂̍ő��޲Đ��𐧌�����

:option. o:o
:target. any
:negate.
:usage.  optimize bytecode jumps
:jusage. �޲� ���� �ެ��߂̍œK��

:option. o:rp
:target. any
:negate.
:usage.  optimize classfile by removing unreferenced private members
:jusage. �Q�Ƃ���Ă��Ȃ���ײ�ް� ���ް���������Ă̸׽̧�ق̍œK��

:option. parse
:target. any
:file.
:usage. generate parser info for source files into <file>
:jusage.
:internal.

:option. pj
:target. any
:usage. enable preprocessed functionality
:jusage.
:internal.

:option. prof
:target. any
:usage. instrument methods with profiling code
:jusage. ���̧�� ���ނ𐶐�
:internal.

:option. stream
:target. any
:usage. use stream i/o instead of file i/o
:jusage. ̧�ٓ��o�͂̑����stream���o�͂��g�p����
:internal.

:option. tp
:target. dbg
:id.
:usage. set #pragma on( <id> )
:jusage. #pragma on( <id> ) ��ݒ�
:internal.

:option. tw
:target. any
:number.
:usage. set tab width to <num>
:jusage. <num>����ޕ���ݒ�

:option. verbose
:target. any
:usage. same as -mpst
:jusage. -mpst�Ɠ��l

:option. w
:target. any
:enumerate. warn_level
:number. CheckWarnLevel
:usage. set warning level number
:jusage. �x�����ٔԍ���ݒ�

:option. wcd
:target. any
:special. handleWCD =<group>-<num>
:usage. warning control: disable warning message <group>-<num>
:jusage. �x�����۰�: �x��ү����<group>-<num>���g���Ȃ�����

:option. wce
:target. any
:special. handleWCE =<group>-<num>
:usage. warning control: enable warning message <group>-<num>
:jusage. �x�����۰�: �x��ү����<group>-<num>��L���ɂ���

:option. we
:target. any
:negate.
:usage. treat all warnings as errors
:jusage. �x�������ׂĴװ�Ƃ���

:option. ws
:target. any
:negate.
:usage. warn about Java style violations
:jusage. Java style violations���x������

:option. wx
:target. any
:enumerate. warn_level
:usage. set warning level to maximum setting
:jusage. �x�����ق��ő�ݒ�ɂ���

:option. x
:target. any
:negate.
:usage. disable extensions - pedantic checking
:jusage.  �g�����Ȃ� - ���������Ȃ�

:option. xref
:target. any
:usage. return list of all class files referenced
:jusage.  ���ׂĂ̸׽ ̧�ق̎Q��ؽĂ��쐬����
:internal.

:option. xrefx
:target. any
:path.
:usage. during xref, exclude classes starting with given string
:jusage. xref, exclude�׽�̊Ԃ͗^����ꂽ������Ŏn�߂�
:internal.

:option. xrefxp
:target. any
:path.
:usage. during xref, exclude classes found in given path
:jusage.  xref�Ɠ����A�������^����ꂽ�߽�ɂ���׽�͏��O����
:internal.

:option. xreffind
:target. any
:path.
:usage. regardless of xrefx, find classes starting with given string
:jusage.
:internal.

:option. zq
:target. any
:negate.
:usage. operate quietly (display only error messages)
:jusage. �璷��ү���ނ�\�����Ȃ�(�װ ү���ނ̂ݕ\��)

:option. zcm
:target. any
:negate.
:usage. create object for classes compatible with Microsoft VM
:jusage.  Microsoft VM�݊��̸׽ ��޼ު�Ă��쐬����
:internal.

:option. zcs
:target. any
:negate.
:usage. create object for classes compatible with Sparc VM
:jusage.  Sparc VM�݊��̸׽ ��޼ު�Ă��쐬����
:internal.

:option. zjm
:target. any
:negate.
:usage. create pre-compiled object compatible with Microsoft JIT
:jusage. Microsoft JIT�݊������ ���߲ٵ�޼ު�Ă��쐬����
:internal.

:option. zjs
:target. any
:negate.
:usage. create pre-compiled object compatible with Sparc JIT
:jusage. Sparc JIT�݊������ ���߲ٵ�޼ު�Ă��쐬����
:internal.

:option. zsd
:target. any
:negate.
:usage. feedback source dependencies
:jusage.
:internal.
