.function: one/0
    print (istore 1 42)
    izero 0
    ; no return here
.end

.function: two/0
    print (istore 1 48)
    izero 0
    return
.end

.function: main/1
    call (frame 0 2) one/0
    izero 0
    return
.end
