.function: one
    istore 1 42
    print 1
    izero 0
    ; no end here
.end

.function: two
    istore 1 48
    print 1
    izero 0
    end
.end

.function: main
    frame 0 2
    call one
    izero 0
    end
.end
