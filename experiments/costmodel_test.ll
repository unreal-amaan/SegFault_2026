define void @test(i32 %n) {
entry:
  br label %loop

loop:
  %i = phi i32 [ 0, %entry ], [ %next, %loop ]
  %next = add i32 %i, 1
  %cond = icmp slt i32 %next, %n

  br i1 %cond, label %loop, label %exit, !llvm.loop !0

exit:
  ret void
}

!0 = distinct !{
  !0,
  !1
}

!1 = !{
  !"costmodel",
  !2
}

!2 = !{
  !"unroll",
  !"factor",
  i32 4
}