$! set noon
$!
$! VMS build script for the Ingres PHP driver
$!
$ IF F$TRNLNM("II_SYSTEM") .EQS. "" THEN GOTO ERROR_II_SYSTEM
$ IF F$TRNLNM("PHP_INCLUDES") .EQS. "" THEN GOTO ERROR_INC
$ IF F$TRNLNM("PHP_ROOT") .EQS. "" THEN GOTO ERROR_PHPROOT
$! PHP_INCLUDES should be defined to include the following:
$! PHP_ROOT:[000000...], APACHE_SOURCE_ROOT:[BUILD.APL.ALPHA-V1_2.PORTLIB]
$!
$! The following logicals are needed to build the extension
$ DEFINE /NOLOG EXT PHP_ROOT:[ext]
$ DEFINE /NOLOG LIBBCMATH PHP_ROOT:[ext.bcmath.libbcmath]
$ DEFINE /NOLOG LIBCDB PHP_ROOT:[ext.dba.libcdb]
$ DEFINE /NOLOG LIBFLATFILE PHP_ROOT:[ext.dba.libflatfile]
$ DEFINE /NOLOG PCRELIB PHP_ROOT:[ext.pcre.pcrelib]
$ DEFINE /NOLOG MAIN PHP_ROOT:[main]
$ DEFINE /NOLOG SYS PHP_ROOT:[SUPPORT],APL_ROOT:[PortLIB]
$ DEFINE /NOLOG REGEX PHP_ROOT:[regex]
$ DEFINE /NOLOG TSRM PHP_ROOT:[tsrm]
$ DEFINE /NOLOG WIN32 PHP_ROOT:[win32]
$!
$! Compile convertUTF.c
$COMPILE:
$ IF F$SEARCH("CONVERTUTF.OBJ") .NES. "" THEN $ object_time = f$time("convertutf.obj")
$ source_time = f$time("convertutf.c")
$ IF source_time .lts. object_time then GOTO II_C 
$ WRITE SYS$OUTPUT "Compiling convertUTF.c"
$ CC convertUTF.c /NoDebug/NoWarn/Optimize/Machine_Code -
        /Show=EXPANSION/Prefix_Library_Entries=ALL -
        /INCLUDE=(II_SYSTEM:[ingres.files ], PHP_INCLUDES) -
        /DEFINE=(COMPILE_DL_INGRES=1,HAVE_INGRES=1,_VMS_V6_SOURCE,__VMS_VER_OVERRIDE)
$!
$! Compile ii.c
$II_C:
$ IF F$SEARCH("II.OBJ") .NES. "" THEN $ object_time = f$time("ii.obj")
$ source_time = f$time("ii.c")
$ IF source_time .lts. object_time then GOTO CREATE_LIBRARY 
$ WRITE SYS$OUTPUT "Compiling ii.c"
$ IF F$SEARCH("II.OBJ") .NES. "" THEN DEL II.OBJ;*
$ CC ii.c /NoDebug/NoWarn/Optimize/Machine_Code -
        /Show=EXPANSION/Prefix_Library_Entries=ALL -
        /INCLUDE=(II_SYSTEM:[ingres.files ], PHP_INCLUDES) -
        /DEFINE=(COMPILE_DL_INGRES=1,HAVE_INGRES=1,_VMS_V6_SOURCE,__VMS_VER_OVERRIDE)
$!
$! Build the library
$CREATE_LIBRARY
$ IF F$SEARCH("LIBINGRES.OLB") .NES. "" THEN DEL LIBINGRES.OLB;*
$ LIBRARY/CREATE/LOG LIBINGRES.OLB
$ LIBRARY/INSERT/LOG/GLOBALS LIBINGRES.OLB convertUTF.obj, ii.obj
$!
$! Link
$ WRITE SYS$OUTPUT "Linking shared image PHP_INGRES.EXE"
$ LINK /SHARE=PHP_INGRES /DSF=[.OBJ]PHP_INGRES  /MAP=[.LIS]PHP_INGRES /CROSS/FULL INGRES.OPT/OPT
$ GOTO EOS
$!
$! Some simple error reporting
$ERROR_II_SYSTEM:
$ WRITE SYS$OUTPUT "Ingres logical II_SYSTEM is not defined"
$ WRITE SYS$OUTPUT "Unable to continue"
$ GOTO EOS
$ERROR_PHPROOT:
$ WRITE SYS$OUTPUT "Logical PHP_ROOT is missing see the file README.VMS for more information"
$ WRITE SYS$OUTPUT "Unable to continue"
$ GOTO EOS
$ERROR_INC:
$ WRITE SYS$OUTPUT "Logical PHP_INCLUDES is missing see the file README.VMS for more information"
$ WRITE SYS$OUTPUT "Unable to continue"
$ GOTO EOS
$EOS:
