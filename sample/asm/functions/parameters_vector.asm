.bsignature: misc::argsvector

.function: foo/4
    link misc

    ; FIXME: try will not be needed when ENTER instruction is implemented
    try
    enter misc::argsvector

    print 1
    return
.end

.function: main/1
    frame ^[(param 0 (istore 1 0)) (param 1 (istore 2 1)) (param 2 (istore 3 2)) (param 3 (istore 4 3))]
    call ^(izero 0) foo/4
    return
.end
