; ModuleID = 'mini-c'
source_filename = "mini-c"

declare i32 @print_int(i32)

define i32 @vector_total(ptr %a, ptr %b, i32 %n) {
entry:
  %total = alloca i32, align 4
  %i = alloca i32, align 4
  %n3 = alloca i32, align 4
  %b2 = alloca ptr, align 8
  %a1 = alloca ptr, align 8
  store ptr %a, ptr %a1, align 8
  store ptr %b, ptr %b2, align 8
  store i32 %n, ptr %n3, align 4
  store i32 0, ptr %i, align 4
  br label %while.cond

while.cond:                                       ; preds = %while.body, %entry
  %i4 = load i32, ptr %i, align 4
  %n5 = load i32, ptr %n3, align 4
  %lttmp = icmp slt i32 %i4, %n5
  br i1 %lttmp, label %while.body, label %while.end

while.body:                                       ; preds = %while.cond
  %total6 = load i32, ptr %total, align 4
  %a_loadedptr = load ptr, ptr %a1, align 8
  %i7 = load i32, ptr %i, align 4
  %arrayidx = getelementptr inbounds [10 x i32], ptr %a_loadedptr, i32 0, i32 %i7
  %arrayload = load i32, ptr %arrayidx, align 4
  %addtmp = add i32 %total6, %arrayload
  %b_loadedptr = load ptr, ptr %b2, align 8
  %i8 = load i32, ptr %i, align 4
  %arrayidx9 = getelementptr inbounds [10 x i32], ptr %b_loadedptr, i32 0, i32 %i8
  %arrayload10 = load i32, ptr %arrayidx9, align 4
  %addtmp11 = add i32 %addtmp, %arrayload10
  store i32 %addtmp11, ptr %total, align 4
  %i12 = load i32, ptr %i, align 4
  %addtmp13 = add i32 %i12, 1
  store i32 %addtmp13, ptr %i, align 4
  br label %while.cond

while.end:                                        ; preds = %while.cond
  %total14 = load i32, ptr %total, align 4
  ret i32 %total14
}
