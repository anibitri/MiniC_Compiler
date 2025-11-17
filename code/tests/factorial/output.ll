; ModuleID = 'mini-c'
source_filename = "mini-c"

define i32 @factorial(i32 %n) {
entry:
  %factorial = alloca i32, align 4
  %i = alloca i32, align 4
  %n1 = alloca i32, align 4
  store i32 %n, ptr %n1, align 4
  store i32 0, ptr %i, align 4
  store i32 0, ptr %factorial, align 4
  store i32 1, ptr %factorial, align 4
  store i32 1, ptr %i, align 4
  br label %while.cond

while.cond:                                       ; preds = %while.body, %entry
  %i2 = load i32, ptr %i, align 4
  %n3 = load i32, ptr %n1, align 4
  %letmp = icmp sle i32 %i2, %n3
  br i1 %letmp, label %while.body, label %while.end

while.body:                                       ; preds = %while.cond
  %factorial4 = load i32, ptr %factorial, align 4
  %i5 = load i32, ptr %i, align 4
  %multmp = mul i32 %factorial4, %i5
  store i32 %multmp, ptr %factorial, align 4
  %i6 = load i32, ptr %i, align 4
  %addtmp = add i32 %i6, 1
  store i32 %addtmp, ptr %i, align 4
  br label %while.cond

while.end:                                        ; preds = %while.cond
  %factorial7 = load i32, ptr %factorial, align 4
  ret i32 %factorial7
}
