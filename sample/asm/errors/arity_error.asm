.function: foo/1
    print (arg 1 0)
    return
.end

.function: main/1
    frame 0
    call foo/1

    izero 0
    return
.end
