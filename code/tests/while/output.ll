; ModuleID = 'mini-c'
source_filename = "mini-c"

@test = global i32 0
@f = global float 0.000000e+00
@b = global i1 false

declare i32 @print_int(i32)

define i32 @While(i32 %n) {
entry:
  %result = alloca i32, align 4
  %n1 = alloca i32, align 4
  store i32 %n, ptr %n1, align 4
  store i32 12, ptr @test, align 4
  store i32 0, ptr %result, align 4
  %test = load i32, ptr @test, align 4
  %0 = call i32 @print_int(i32 %test)
  br label %while.cond

while.cond:                                       ; preds = %while.body, %entry
  %result2 = load i32, ptr %result, align 4
  %lttmp = icmp slt i32 %result2, 10
  br i1 %lttmp, label %while.body, label %while.end

while.body:                                       ; preds = %while.cond
  %result3 = load i32, ptr %result, align 4
  %addtmp = add i32 %result3, 1
  store i32 %addtmp, ptr %result, align 4
  br label %while.cond

while.end:                                        ; preds = %while.cond
  %result4 = load i32, ptr %result, align 4
  ret i32 %result4
}
