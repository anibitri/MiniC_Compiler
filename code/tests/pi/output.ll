; ModuleID = 'mini-c'
source_filename = "mini-c"

define float @pi() {
entry:
  %i = alloca i32, align 4
  %PI = alloca float, align 4
  %flag = alloca i1, align 1
  store i1 true, ptr %flag, align 1
  store float 3.000000e+00, ptr %PI, align 4
  store i32 2, ptr %i, align 4
  br label %while.cond

while.cond:                                       ; preds = %ifcont, %entry
  %i1 = load i32, ptr %i, align 4
  %lttmp = icmp slt i32 %i1, 100
  br i1 %lttmp, label %while.body, label %while.end

while.body:                                       ; preds = %while.cond
  %flag2 = load i1, ptr %flag, align 1
  %ifcond = icmp ne i1 %flag2, false
  br i1 %ifcond, label %then, label %else

while.end:                                        ; preds = %while.cond
  %PI22 = load float, ptr %PI, align 4
  ret float %PI22

then:                                             ; preds = %while.body
  %PI3 = load float, ptr %PI, align 4
  %i4 = load i32, ptr %i, align 4
  %i5 = load i32, ptr %i, align 4
  %addtmp = add i32 %i5, 1
  %multmp = mul i32 %i4, %addtmp
  %i6 = load i32, ptr %i, align 4
  %addtmp7 = add i32 %i6, 2
  %multmp8 = mul i32 %multmp, %addtmp7
  %0 = sitofp i32 %multmp8 to float
  %divtmp = fdiv float 4.000000e+00, %0
  %addtmp9 = fadd float %PI3, %divtmp
  store float %addtmp9, ptr %PI, align 4
  br label %ifcont

else:                                             ; preds = %while.body
  %PI10 = load float, ptr %PI, align 4
  %i11 = load i32, ptr %i, align 4
  %i12 = load i32, ptr %i, align 4
  %addtmp13 = add i32 %i12, 1
  %multmp14 = mul i32 %i11, %addtmp13
  %i15 = load i32, ptr %i, align 4
  %addtmp16 = add i32 %i15, 2
  %multmp17 = mul i32 %multmp14, %addtmp16
  %1 = sitofp i32 %multmp17 to float
  %divtmp18 = fdiv float 4.000000e+00, %1
  %subtmp = fsub float %PI10, %divtmp18
  store float %subtmp, ptr %PI, align 4
  br label %ifcont

ifcont:                                           ; preds = %else, %then
  %flag19 = load i1, ptr %flag, align 1
  %eqtmp = icmp eq i1 %flag19, false
  store i1 %eqtmp, ptr %flag, align 1
  %i20 = load i32, ptr %i, align 4
  %addtmp21 = add i32 %i20, 2
  store i32 %addtmp21, ptr %i, align 4
  br label %while.cond
}
