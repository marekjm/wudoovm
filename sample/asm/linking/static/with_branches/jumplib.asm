.def: jumprint 0
    arg 0 1
    print 1

    istore 2 42
    ilt 2 1
    branch 2 :lesser :greater_or_equal

    .mark: greater_or_equal
    ;strstore 3 "Supplied number is greater than (or equal to) 42!"
    jump :finish

    .mark: lesser
    ;strstore 3 "Supplied number is lesser than 42!"
    ;jump :finish

    .mark: finish
    ;print 3
    end
.end
