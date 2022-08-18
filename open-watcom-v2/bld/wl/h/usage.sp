%C  {directive}
Commands valid for the QNX executable format:
directive  ::= "File"       obj_spec{","obj_spec}
            | "Library"     library_file{","library_file}
            | "Name"        exe_file
            | "OPtion"      option{","option}
            | "FORMat"      form
            | "SYStem"      system_name
            | "SYStem Begin" system_name directive {directive} "End"      
            | "Path"        path_name{";"path_name}
            | "Debug"       dblist
            | "MODTrace"    module_name{","module_name}
            | "SYMTrace"    symbol_name{","symbol_name}
            | "@"           directive_file
            | "#"           comment
            | "SEgment"     segdesc{","segdesc}
            | "NEWsegment"
obj_spec  ::= obj_file["("obj_member")"] | library_file["("lib_member")"]
dblist    ::= [dboption{","dboption}]
dboption  ::= "LInes" | "Types" | "LOcals" | "All"
option    ::= "Map"["="map_file] | "NODefaultlibs" | "STack="n | "Dosseg"
            | "Verbose" | "Caseexact" | "Undefsok" | "NAMELen="n | "Quiet"
            | "SYMFile"["="symbol_file]
            | "NORelocs" | "LOnglived" | "Heapsize="n | "PACKCode="n
            | "RESource"("="res_file_name | "'"string"'")
form      ::= "QNX"
segdesc   ::= ("'"seg_name"'" | "Class""'"class_name"'")segmodel {segmodel}
segmodel  ::= "EXECUTEOnly" | "EXECUTERead" | "READOnly" | "READWrite"

For other commands use "wlink ?" (you may have to escape the "?") which will
output a complete list (including commands pertaining to executable
formats other than QNX load modules).
