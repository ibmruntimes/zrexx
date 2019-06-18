#!/bin/bash
os=$(/bin/uname)
if [[ ${os} !=  'OS/390' ]]; then
    echo this addon is for zos only.
    exit 1
fi
id=$(/bin/id -nu)
dsn="${id}.REXX.TESTPDS"
alloc_cmd="alloc da('${dsn}') dsorg(po) block(100) space(2,2) lrecl(80) blksize(800) dir(1) recfm(f,b) new"
/bin/tso "${alloc_cmd}"
if [[ $? -ne 0 ]]; then
    echo "Failed to allocate $dsn"
    echo 
    echo "consider manually deleting it by"
    echo 
    echo "    /bin/tso \"delete '${dsn}'\" "
    echo 
    exit 1
fi
/bin/cat > $$.tmp  << "END"
/*rexx*/
Parse source src
Say "-----------------------------------------------"
say "Me:" src
parse arg n rest
 say "Arguments received:" n rest
If datatype(n,'W') <> 1 then n = 1
Do i=1 TO n
  Say 'Hello World the' i'. time ...'
End
return n*10;
END
/bin/cp $$.tmp "//'${dsn}(HELLO)'"
unlink $$.tmp

