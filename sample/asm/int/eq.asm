; This script tests support for integer equality checking.
; Its expected output is "true".

.function: main/1
    istore 1 1
    istore 2 1
    ieq 1 1 2

    ; should be true
    print 1
    izero 0
    return
.end
