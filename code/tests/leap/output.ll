; ModuleID = 'mini-c'
source_filename = "mini-c"

declare i32 @print_int(i32)

define i32 @is_leap(i32 %year) {
entry:
  %temp = alloca i32, align 4
  %isLeap = alloca i32, align 4
  %year1 = alloca i32, align 4
  store i32 %year, ptr %year1, align 4
  store i32 0, ptr %isLeap, align 4
  store i32 0, ptr %temp, align 4
  %year2 = load i32, ptr %year1, align 4
  %0 = srem i32 %year2, 4
  %eqtmp = icmp eq i32 %0, 0
  %ifcond = icmp ne i1 %eqtmp, false
  br i1 %ifcond, label %then, label %else

then:                                             ; preds = %entry
  %year3 = load i32, ptr %year1, align 4
  %1 = srem i32 %year3, 100
  %eqtmp4 = icmp eq i32 %1, 0
  %ifcond5 = icmp ne i1 %eqtmp4, false
  br i1 %ifcond5, label %then6, label %else7

else:                                             ; preds = %entry
  store i32 0, ptr %isLeap, align 4
  br label %ifcont

ifcont:                                           ; preds = %else, %ifcont8
  %isLeap15 = load i32, ptr %isLeap, align 4
  ret i32 %isLeap15

then6:                                            ; preds = %then
  %year9 = load i32, ptr %year1, align 4
  %2 = srem i32 %year9, 400
  %eqtmp10 = icmp eq i32 %2, 0
  %ifcond11 = icmp ne i1 %eqtmp10, false
  br i1 %ifcond11, label %then12, label %else13

else7:                                            ; preds = %then
  store i32 1, ptr %isLeap, align 4
  br label %ifcont8

ifcont8:                                          ; preds = %else7, %ifcont14
  br label %ifcont

then12:                                           ; preds = %then6
  store i32 1, ptr %isLeap, align 4
  br label %ifcont14

else13:                                           ; preds = %then6
  store i32 0, ptr %isLeap, align 4
  br label %ifcont14

ifcont14:                                         ; preds = %else13, %then12
  br label %ifcont8
}
