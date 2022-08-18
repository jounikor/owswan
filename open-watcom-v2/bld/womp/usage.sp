::
:: New Usage file for WOMP.
::
:segment ENGLISH
Usage: womp [options]* file [options|file]*
Options must precede the file(s) you wish them to affect (* is default)
Options:  ( /option is also accepted )
-o=spec Output filename/directory specification
-fm    *Generate Microsoft 16- and 32-bit object files
-fm2    Generate Microsoft OS/2 2.0 32-bit object files
-fp     Generate PharLap Easy OMF-386 object files
-f-     Do not generate an object file (useful with -dx)
-dm     Generate Microsoft CodeView debugging information
-dp    *Generate PharLap/Metaware variant of CodeView debugging information
-dt     Generate Turbo debugging information
-dx     Generate human-readable text to console
-d-     Do not generate any debugging information
-pw    *Parse WATCOM debugging information
-p7     Parse WATCOM C 7.0 debugging information
-p-     Do not attempt to parse any debugging information
-q      Be quiet
:segment HIDDEN
-l      Move library COMENTs to beginning of file
:endsegment
-b      Leave temporary files and output batch file commands
These options apply to .wmp files
@file   Read file[.wmp] for options
#       Ignore from # to end of line (comment)
:elsesegment JAPANESE
�g�p���@: womp [options]* file [options|file]*
�I�v�V�����̓t�@�C���̑O�Ɏw�肵�ĉ������w�肵�ĉ�����
-o=spec �o�̓t�@�C�����^�f�B���N�g���̎w��
-fm    *Microsoft��16�r�b�g��32�r�b�g�E�I�u�W�F�N�g�E�t�@�C���̐���
-fm2    Microsoft OS/2 2.0��32�r�b�g�E�I�u�W�F�N�g�E�t�@�C���̐���
-fp     PharLap Easy OMF-386 �I�u�W�F�N�g�E�t�@�C���̐���
-f-     �I�u�W�F�N�g�E�t�@�C���̐����̋֎~(-dx�Ƌ��Ɏg�p����ƕ֗�
-dm     Microsoft CodeView�f�o�b�O���̐���
-dp    *PharLap/Metaware�f�o�b�O���̐���
-dt     Turbo�f�o�b�O���̐���
-dx     �ǃe�L�X�g�̃R���\�[���ւ̏o��
-d-     �f�o�b�O���̐����̋֎~
-pw    *WATCOM�f�o�b�O���̉��
-p7     WATCOM C7.0�f�o�b�O���̉��
-p-     �f�o�b�O���̉�͂̋֎~
-q      �����b�Z�[�W�E���[�h
:segment HIDDEN
-l      ���C�u����COMENT���t�@�C���̐擪�Ɉړ�
:endsegment
-b      �e���|�����E�t�@�C�����c���A�o�b�`�t�@�C���E�R�}���h���o��
�ȉ��̃I�v�V������ .wmp �t�@�C���ɓK�p�ł��܂�
@file   �I�v�V�����Ɋւ���[.wmp]�t�@�C����ǂ݂܂�
#       #����s���܂ł𖳎����܂�(����)
:endsegment
