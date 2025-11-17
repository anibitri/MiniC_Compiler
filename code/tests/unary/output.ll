; ModuleID = 'mini-c'
source_filename = "mini-c"

declare i32 @print_int(i32)

declare float @print_float(float)

define float @unary(i32 %n, float %m) {
entry:
  %sum = alloca float, align 4
  %result = alloca float, align 4
  %m2 = alloca float, align 4
  %n1 = alloca i32, align 4
  store i32 %n, ptr %n1, align 4
  store float %m, ptr %m2, align 4
  store float 0.000000e+00, ptr %result, align 4
  store float 0.000000e+00, ptr %sum, align 4
  store float 0.000000e+00, ptr %sum, align 4
  %n3 = load i32, ptr %n1, align 4
  %m4 = load float, ptr %m2, align 4
  %0 = sitofp i32 %n3 to float
  %addtmp = fadd float %0, %m4
  store float %addtmp, ptr %result, align 4
  %result5 = load float, ptr %result, align 4
  %1 = call float @print_float(float %result5)
  %sum6 = load float, ptr %sum, align 4
  %result7 = load float, ptr %result, align 4
  %addtmp8 = fadd float %sum6, %result7
  store float %addtmp8, ptr %sum, align 4
  %n9 = load i32, ptr %n1, align 4
  %m10 = load float, ptr %m2, align 4
  %subtmp = fsub float 0.000000e+00, %m10
  %2 = sitofp i32 %n9 to float
  %addtmp11 = fadd float %2, %subtmp
  store float %addtmp11, ptr %result, align 4
  %result12 = load float, ptr %result, align 4
  %3 = call float @print_float(float %result12)
  %sum13 = load float, ptr %sum, align 4
  %result14 = load float, ptr %result, align 4
  %addtmp15 = fadd float %sum13, %result14
  store float %addtmp15, ptr %sum, align 4
  %n16 = load i32, ptr %n1, align 4
  %m17 = load float, ptr %m2, align 4
  %subtmp18 = fsub float 0.000000e+00, %m17
  %subtmp19 = fsub float 0.000000e+00, %subtmp18
  %4 = sitofp i32 %n16 to float
  %addtmp20 = fadd float %4, %subtmp19
  store float %addtmp20, ptr %result, align 4
  %result21 = load float, ptr %result, align 4
  %5 = call float @print_float(float %result21)
  %sum22 = load float, ptr %sum, align 4
  %result23 = load float, ptr %result, align 4
  %addtmp24 = fadd float %sum22, %result23
  store float %addtmp24, ptr %sum, align 4
  %n25 = load i32, ptr %n1, align 4
  %subtmp26 = sub i32 0, %n25
  %m27 = load float, ptr %m2, align 4
  %subtmp28 = fsub float 0.000000e+00, %m27
  %6 = sitofp i32 %subtmp26 to float
  %addtmp29 = fadd float %6, %subtmp28
  store float %addtmp29, ptr %result, align 4
  %result30 = load float, ptr %result, align 4
  %7 = call float @print_float(float %result30)
  %sum31 = load float, ptr %sum, align 4
  %result32 = load float, ptr %result, align 4
  %addtmp33 = fadd float %sum31, %result32
  store float %addtmp33, ptr %sum, align 4
  %sum34 = load float, ptr %sum, align 4
  ret float %sum34
}
