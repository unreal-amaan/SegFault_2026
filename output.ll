; ModuleID = 'benchmarks/polybench/build/ir/gemm_with_ids.ll'
source_filename = "./linear-algebra/blas/gemm/gemm.c"
target datalayout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-i128:128-f80:128-n8:16:32:64-S128"
target triple = "x86_64-unknown-linux-gnu"

@.str = private unnamed_addr constant [1 x i8] zeroinitializer, align 1
@stderr = external global ptr, align 8
@.str.1 = private unnamed_addr constant [23 x i8] c"==BEGIN DUMP_ARRAYS==\0A\00", align 1
@.str.2 = private unnamed_addr constant [15 x i8] c"begin dump: %s\00", align 1
@.str.3 = private unnamed_addr constant [2 x i8] c"C\00", align 1
@.str.4 = private unnamed_addr constant [2 x i8] c"\0A\00", align 1
@.str.5 = private unnamed_addr constant [8 x i8] c"%0.2lf \00", align 1
@.str.6 = private unnamed_addr constant [17 x i8] c"\0Aend   dump: %s\0A\00", align 1
@.str.7 = private unnamed_addr constant [23 x i8] c"==END   DUMP_ARRAYS==\0A\00", align 1

; Function Attrs: noinline nounwind uwtable
define dso_local i32 @main(i32 noundef %argc, ptr noundef %argv) #0 {
entry:
  %retval = alloca i32, align 4
  %argc.addr = alloca i32, align 4
  %argv.addr = alloca ptr, align 8
  %ni = alloca i32, align 4
  %nj = alloca i32, align 4
  %nk = alloca i32, align 4
  %alpha = alloca double, align 8
  %beta = alloca double, align 8
  %C = alloca ptr, align 8
  %A = alloca ptr, align 8
  %B = alloca ptr, align 8
  store i32 0, ptr %retval, align 4
  store i32 %argc, ptr %argc.addr, align 4
  store ptr %argv, ptr %argv.addr, align 8
  store i32 1000, ptr %ni, align 4
  store i32 1100, ptr %nj, align 4
  store i32 1200, ptr %nk, align 4
  %call = call ptr @polybench_alloc_data(i64 noundef 1100000, i32 noundef 8)
  store ptr %call, ptr %C, align 8
  %call1 = call ptr @polybench_alloc_data(i64 noundef 1200000, i32 noundef 8)
  store ptr %call1, ptr %A, align 8
  %call2 = call ptr @polybench_alloc_data(i64 noundef 1320000, i32 noundef 8)
  store ptr %call2, ptr %B, align 8
  %0 = load i32, ptr %ni, align 4
  %1 = load i32, ptr %nj, align 4
  %2 = load i32, ptr %nk, align 4
  %3 = load ptr, ptr %C, align 8
  %arraydecay = getelementptr inbounds [1000 x [1100 x double]], ptr %3, i64 0, i64 0
  %4 = load ptr, ptr %A, align 8
  %arraydecay3 = getelementptr inbounds [1000 x [1200 x double]], ptr %4, i64 0, i64 0
  %5 = load ptr, ptr %B, align 8
  %arraydecay4 = getelementptr inbounds [1200 x [1100 x double]], ptr %5, i64 0, i64 0
  call void @init_array(i32 noundef %0, i32 noundef %1, i32 noundef %2, ptr noundef %alpha, ptr noundef %beta, ptr noundef %arraydecay, ptr noundef %arraydecay3, ptr noundef %arraydecay4)
  %6 = load i32, ptr %ni, align 4
  %7 = load i32, ptr %nj, align 4
  %8 = load i32, ptr %nk, align 4
  %9 = load double, ptr %alpha, align 8
  %10 = load double, ptr %beta, align 8
  %11 = load ptr, ptr %C, align 8
  %arraydecay5 = getelementptr inbounds [1000 x [1100 x double]], ptr %11, i64 0, i64 0
  %12 = load ptr, ptr %A, align 8
  %arraydecay6 = getelementptr inbounds [1000 x [1200 x double]], ptr %12, i64 0, i64 0
  %13 = load ptr, ptr %B, align 8
  %arraydecay7 = getelementptr inbounds [1200 x [1100 x double]], ptr %13, i64 0, i64 0
  call void @kernel_gemm(i32 noundef %6, i32 noundef %7, i32 noundef %8, double noundef %9, double noundef %10, ptr noundef %arraydecay5, ptr noundef %arraydecay6, ptr noundef %arraydecay7)
  %14 = load i32, ptr %argc.addr, align 4
  %cmp = icmp sgt i32 %14, 42
  br i1 %cmp, label %land.lhs.true, label %if.end

land.lhs.true:                                    ; preds = %entry
  %15 = load ptr, ptr %argv.addr, align 8
  %arrayidx = getelementptr inbounds ptr, ptr %15, i64 0
  %16 = load ptr, ptr %arrayidx, align 8
  %call8 = call i32 @strcmp(ptr noundef %16, ptr noundef @.str) #5
  %tobool = icmp ne i32 %call8, 0
  br i1 %tobool, label %if.end, label %if.then

if.then:                                          ; preds = %land.lhs.true
  %17 = load i32, ptr %ni, align 4
  %18 = load i32, ptr %nj, align 4
  %19 = load ptr, ptr %C, align 8
  %arraydecay9 = getelementptr inbounds [1000 x [1100 x double]], ptr %19, i64 0, i64 0
  call void @print_array(i32 noundef %17, i32 noundef %18, ptr noundef %arraydecay9)
  br label %if.end

if.end:                                           ; preds = %if.then, %land.lhs.true, %entry
  %20 = load ptr, ptr %C, align 8
  call void @free(ptr noundef %20) #6
  %21 = load ptr, ptr %A, align 8
  call void @free(ptr noundef %21) #6
  %22 = load ptr, ptr %B, align 8
  call void @free(ptr noundef %22) #6
  ret i32 0
}

declare ptr @polybench_alloc_data(i64 noundef, i32 noundef) #1

; Function Attrs: noinline nounwind uwtable
define internal void @init_array(i32 noundef %ni, i32 noundef %nj, i32 noundef %nk, ptr noundef %alpha, ptr noundef %beta, ptr noundef %C, ptr noundef %A, ptr noundef %B) #0 {
entry:
  %ni.addr = alloca i32, align 4
  %nj.addr = alloca i32, align 4
  %nk.addr = alloca i32, align 4
  %alpha.addr = alloca ptr, align 8
  %beta.addr = alloca ptr, align 8
  %C.addr = alloca ptr, align 8
  %A.addr = alloca ptr, align 8
  %B.addr = alloca ptr, align 8
  %i = alloca i32, align 4
  %j = alloca i32, align 4
  store i32 %ni, ptr %ni.addr, align 4
  store i32 %nj, ptr %nj.addr, align 4
  store i32 %nk, ptr %nk.addr, align 4
  store ptr %alpha, ptr %alpha.addr, align 8
  store ptr %beta, ptr %beta.addr, align 8
  store ptr %C, ptr %C.addr, align 8
  store ptr %A, ptr %A.addr, align 8
  store ptr %B, ptr %B.addr, align 8
  %0 = load ptr, ptr %alpha.addr, align 8
  store double 1.500000e+00, ptr %0, align 8
  %1 = load ptr, ptr %beta.addr, align 8
  store double 1.200000e+00, ptr %1, align 8
  store i32 0, ptr %i, align 4
  br label %for.cond

for.cond:                                         ; preds = %for.inc7, %entry
  %2 = load i32, ptr %i, align 4
  %3 = load i32, ptr %ni.addr, align 4
  %cmp = icmp slt i32 %2, %3
  br i1 %cmp, label %for.body, label %for.end9

for.body:                                         ; preds = %for.cond
  store i32 0, ptr %j, align 4
  br label %for.cond1

for.cond1:                                        ; preds = %for.inc, %for.body
  %4 = load i32, ptr %j, align 4
  %5 = load i32, ptr %nj.addr, align 4
  %cmp2 = icmp slt i32 %4, %5
  br i1 %cmp2, label %for.body3, label %for.end

for.body3:                                        ; preds = %for.cond1
  %6 = load i32, ptr %i, align 4
  %7 = load i32, ptr %j, align 4
  %mul = mul nsw i32 %6, %7
  %add = add nsw i32 %mul, 1
  %8 = load i32, ptr %ni.addr, align 4
  %rem = srem i32 %add, %8
  %conv = sitofp i32 %rem to double
  %9 = load i32, ptr %ni.addr, align 4
  %conv4 = sitofp i32 %9 to double
  %div = fdiv double %conv, %conv4
  %10 = load ptr, ptr %C.addr, align 8
  %11 = load i32, ptr %i, align 4
  %idxprom = sext i32 %11 to i64
  %arrayidx = getelementptr inbounds [1100 x double], ptr %10, i64 %idxprom
  %12 = load i32, ptr %j, align 4
  %idxprom5 = sext i32 %12 to i64
  %arrayidx6 = getelementptr inbounds [1100 x double], ptr %arrayidx, i64 0, i64 %idxprom5
  store double %div, ptr %arrayidx6, align 8
  br label %for.inc

for.inc:                                          ; preds = %for.body3
  %13 = load i32, ptr %j, align 4
  %inc = add nsw i32 %13, 1
  store i32 %inc, ptr %j, align 4
  br label %for.cond1, !llvm.loop !6

for.end:                                          ; preds = %for.cond1
  br label %for.inc7

for.inc7:                                         ; preds = %for.end
  %14 = load i32, ptr %i, align 4
  %inc8 = add nsw i32 %14, 1
  store i32 %inc8, ptr %i, align 4
  br label %for.cond, !llvm.loop !9

for.end9:                                         ; preds = %for.cond
  store i32 0, ptr %i, align 4
  br label %for.cond10

for.cond10:                                       ; preds = %for.inc31, %for.end9
  %15 = load i32, ptr %i, align 4
  %16 = load i32, ptr %ni.addr, align 4
  %cmp11 = icmp slt i32 %15, %16
  br i1 %cmp11, label %for.body13, label %for.end33

for.body13:                                       ; preds = %for.cond10
  store i32 0, ptr %j, align 4
  br label %for.cond14

for.cond14:                                       ; preds = %for.inc28, %for.body13
  %17 = load i32, ptr %j, align 4
  %18 = load i32, ptr %nk.addr, align 4
  %cmp15 = icmp slt i32 %17, %18
  br i1 %cmp15, label %for.body17, label %for.end30

for.body17:                                       ; preds = %for.cond14
  %19 = load i32, ptr %i, align 4
  %20 = load i32, ptr %j, align 4
  %add18 = add nsw i32 %20, 1
  %mul19 = mul nsw i32 %19, %add18
  %21 = load i32, ptr %nk.addr, align 4
  %rem20 = srem i32 %mul19, %21
  %conv21 = sitofp i32 %rem20 to double
  %22 = load i32, ptr %nk.addr, align 4
  %conv22 = sitofp i32 %22 to double
  %div23 = fdiv double %conv21, %conv22
  %23 = load ptr, ptr %A.addr, align 8
  %24 = load i32, ptr %i, align 4
  %idxprom24 = sext i32 %24 to i64
  %arrayidx25 = getelementptr inbounds [1200 x double], ptr %23, i64 %idxprom24
  %25 = load i32, ptr %j, align 4
  %idxprom26 = sext i32 %25 to i64
  %arrayidx27 = getelementptr inbounds [1200 x double], ptr %arrayidx25, i64 0, i64 %idxprom26
  store double %div23, ptr %arrayidx27, align 8
  br label %for.inc28

for.inc28:                                        ; preds = %for.body17
  %26 = load i32, ptr %j, align 4
  %inc29 = add nsw i32 %26, 1
  store i32 %inc29, ptr %j, align 4
  br label %for.cond14, !llvm.loop !11

for.end30:                                        ; preds = %for.cond14
  br label %for.inc31

for.inc31:                                        ; preds = %for.end30
  %27 = load i32, ptr %i, align 4
  %inc32 = add nsw i32 %27, 1
  store i32 %inc32, ptr %i, align 4
  br label %for.cond10, !llvm.loop !14

for.end33:                                        ; preds = %for.cond10
  store i32 0, ptr %i, align 4
  br label %for.cond34

for.cond34:                                       ; preds = %for.inc55, %for.end33
  %28 = load i32, ptr %i, align 4
  %29 = load i32, ptr %nk.addr, align 4
  %cmp35 = icmp slt i32 %28, %29
  br i1 %cmp35, label %for.body37, label %for.end57

for.body37:                                       ; preds = %for.cond34
  store i32 0, ptr %j, align 4
  br label %for.cond38

for.cond38:                                       ; preds = %for.inc52, %for.body37
  %30 = load i32, ptr %j, align 4
  %31 = load i32, ptr %nj.addr, align 4
  %cmp39 = icmp slt i32 %30, %31
  br i1 %cmp39, label %for.body41, label %for.end54

for.body41:                                       ; preds = %for.cond38
  %32 = load i32, ptr %i, align 4
  %33 = load i32, ptr %j, align 4
  %add42 = add nsw i32 %33, 2
  %mul43 = mul nsw i32 %32, %add42
  %34 = load i32, ptr %nj.addr, align 4
  %rem44 = srem i32 %mul43, %34
  %conv45 = sitofp i32 %rem44 to double
  %35 = load i32, ptr %nj.addr, align 4
  %conv46 = sitofp i32 %35 to double
  %div47 = fdiv double %conv45, %conv46
  %36 = load ptr, ptr %B.addr, align 8
  %37 = load i32, ptr %i, align 4
  %idxprom48 = sext i32 %37 to i64
  %arrayidx49 = getelementptr inbounds [1100 x double], ptr %36, i64 %idxprom48
  %38 = load i32, ptr %j, align 4
  %idxprom50 = sext i32 %38 to i64
  %arrayidx51 = getelementptr inbounds [1100 x double], ptr %arrayidx49, i64 0, i64 %idxprom50
  store double %div47, ptr %arrayidx51, align 8
  br label %for.inc52

for.inc52:                                        ; preds = %for.body41
  %39 = load i32, ptr %j, align 4
  %inc53 = add nsw i32 %39, 1
  store i32 %inc53, ptr %j, align 4
  br label %for.cond38, !llvm.loop !16

for.end54:                                        ; preds = %for.cond38
  br label %for.inc55

for.inc55:                                        ; preds = %for.end54
  %40 = load i32, ptr %i, align 4
  %inc56 = add nsw i32 %40, 1
  store i32 %inc56, ptr %i, align 4
  br label %for.cond34, !llvm.loop !18

for.end57:                                        ; preds = %for.cond34
  ret void
}

; Function Attrs: noinline nounwind uwtable
define internal void @kernel_gemm(i32 noundef %ni, i32 noundef %nj, i32 noundef %nk, double noundef %alpha, double noundef %beta, ptr noundef %C, ptr noundef %A, ptr noundef %B) #0 {
entry:
  %ni.addr = alloca i32, align 4
  %nj.addr = alloca i32, align 4
  %nk.addr = alloca i32, align 4
  %alpha.addr = alloca double, align 8
  %beta.addr = alloca double, align 8
  %C.addr = alloca ptr, align 8
  %A.addr = alloca ptr, align 8
  %B.addr = alloca ptr, align 8
  %i = alloca i32, align 4
  %j = alloca i32, align 4
  %k = alloca i32, align 4
  store i32 %ni, ptr %ni.addr, align 4
  store i32 %nj, ptr %nj.addr, align 4
  store i32 %nk, ptr %nk.addr, align 4
  store double %alpha, ptr %alpha.addr, align 8
  store double %beta, ptr %beta.addr, align 8
  store ptr %C, ptr %C.addr, align 8
  store ptr %A, ptr %A.addr, align 8
  store ptr %B, ptr %B.addr, align 8
  store i32 0, ptr %i, align 4
  br label %for.cond

for.cond:                                         ; preds = %for.inc32, %entry
  %0 = load i32, ptr %i, align 4
  %1 = load i32, ptr %ni.addr, align 4
  %cmp = icmp slt i32 %0, %1
  br i1 %cmp, label %for.body, label %for.end34

for.body:                                         ; preds = %for.cond
  store i32 0, ptr %j, align 4
  br label %for.cond1

for.cond1:                                        ; preds = %for.inc, %for.body
  %2 = load i32, ptr %j, align 4
  %3 = load i32, ptr %nj.addr, align 4
  %cmp2 = icmp slt i32 %2, %3
  br i1 %cmp2, label %for.body3, label %for.end

for.body3:                                        ; preds = %for.cond1
  %4 = load double, ptr %beta.addr, align 8
  %5 = load ptr, ptr %C.addr, align 8
  %6 = load i32, ptr %i, align 4
  %idxprom = sext i32 %6 to i64
  %arrayidx = getelementptr inbounds [1100 x double], ptr %5, i64 %idxprom
  %7 = load i32, ptr %j, align 4
  %idxprom4 = sext i32 %7 to i64
  %arrayidx5 = getelementptr inbounds [1100 x double], ptr %arrayidx, i64 0, i64 %idxprom4
  %8 = load double, ptr %arrayidx5, align 8
  %mul = fmul double %8, %4
  store double %mul, ptr %arrayidx5, align 8
  br label %for.inc

for.inc:                                          ; preds = %for.body3
  %9 = load i32, ptr %j, align 4
  %inc = add nsw i32 %9, 1
  store i32 %inc, ptr %j, align 4
  br label %for.cond1, !llvm.loop !20

for.end:                                          ; preds = %for.cond1
  store i32 0, ptr %k, align 4
  br label %for.cond6

for.cond6:                                        ; preds = %for.inc29, %for.end
  %10 = load i32, ptr %k, align 4
  %11 = load i32, ptr %nk.addr, align 4
  %cmp7 = icmp slt i32 %10, %11
  br i1 %cmp7, label %for.body8, label %for.end31

for.body8:                                        ; preds = %for.cond6
  store i32 0, ptr %j, align 4
  br label %for.cond9

for.cond9:                                        ; preds = %for.inc26, %for.body8
  %12 = load i32, ptr %j, align 4
  %13 = load i32, ptr %nj.addr, align 4
  %cmp10 = icmp slt i32 %12, %13
  br i1 %cmp10, label %for.body11, label %for.end28

for.body11:                                       ; preds = %for.cond9
  %14 = load double, ptr %alpha.addr, align 8
  %15 = load ptr, ptr %A.addr, align 8
  %16 = load i32, ptr %i, align 4
  %idxprom12 = sext i32 %16 to i64
  %arrayidx13 = getelementptr inbounds [1200 x double], ptr %15, i64 %idxprom12
  %17 = load i32, ptr %k, align 4
  %idxprom14 = sext i32 %17 to i64
  %arrayidx15 = getelementptr inbounds [1200 x double], ptr %arrayidx13, i64 0, i64 %idxprom14
  %18 = load double, ptr %arrayidx15, align 8
  %mul16 = fmul double %14, %18
  %19 = load ptr, ptr %B.addr, align 8
  %20 = load i32, ptr %k, align 4
  %idxprom17 = sext i32 %20 to i64
  %arrayidx18 = getelementptr inbounds [1100 x double], ptr %19, i64 %idxprom17
  %21 = load i32, ptr %j, align 4
  %idxprom19 = sext i32 %21 to i64
  %arrayidx20 = getelementptr inbounds [1100 x double], ptr %arrayidx18, i64 0, i64 %idxprom19
  %22 = load double, ptr %arrayidx20, align 8
  %23 = load ptr, ptr %C.addr, align 8
  %24 = load i32, ptr %i, align 4
  %idxprom22 = sext i32 %24 to i64
  %arrayidx23 = getelementptr inbounds [1100 x double], ptr %23, i64 %idxprom22
  %25 = load i32, ptr %j, align 4
  %idxprom24 = sext i32 %25 to i64
  %arrayidx25 = getelementptr inbounds [1100 x double], ptr %arrayidx23, i64 0, i64 %idxprom24
  %26 = load double, ptr %arrayidx25, align 8
  %27 = call double @llvm.fmuladd.f64(double %mul16, double %22, double %26)
  store double %27, ptr %arrayidx25, align 8
  br label %for.inc26

for.inc26:                                        ; preds = %for.body11
  %28 = load i32, ptr %j, align 4
  %inc27 = add nsw i32 %28, 1
  store i32 %inc27, ptr %j, align 4
  br label %for.cond9, !llvm.loop !22

for.end28:                                        ; preds = %for.cond9
  br label %for.inc29

for.inc29:                                        ; preds = %for.end28
  %29 = load i32, ptr %k, align 4
  %inc30 = add nsw i32 %29, 1
  store i32 %inc30, ptr %k, align 4
  br label %for.cond6, !llvm.loop !24

for.end31:                                        ; preds = %for.cond6
  br label %for.inc32

for.inc32:                                        ; preds = %for.end31
  %30 = load i32, ptr %i, align 4
  %inc33 = add nsw i32 %30, 1
  store i32 %inc33, ptr %i, align 4
  br label %for.cond, !llvm.loop !26

for.end34:                                        ; preds = %for.cond
  ret void
}

; Function Attrs: nounwind willreturn memory(read)
declare i32 @strcmp(ptr noundef, ptr noundef) #2

; Function Attrs: noinline nounwind uwtable
define internal void @print_array(i32 noundef %ni, i32 noundef %nj, ptr noundef %C) #0 {
entry:
  %ni.addr = alloca i32, align 4
  %nj.addr = alloca i32, align 4
  %C.addr = alloca ptr, align 8
  %i = alloca i32, align 4
  %j = alloca i32, align 4
  store i32 %ni, ptr %ni.addr, align 4
  store i32 %nj, ptr %nj.addr, align 4
  store ptr %C, ptr %C.addr, align 8
  %0 = load ptr, ptr @stderr, align 8
  %call = call i32 (ptr, ptr, ...) @fprintf(ptr noundef %0, ptr noundef @.str.1) #6
  %1 = load ptr, ptr @stderr, align 8
  %call1 = call i32 (ptr, ptr, ...) @fprintf(ptr noundef %1, ptr noundef @.str.2, ptr noundef @.str.3) #6
  store i32 0, ptr %i, align 4
  br label %for.cond

for.cond:                                         ; preds = %for.inc10, %entry
  %2 = load i32, ptr %i, align 4
  %3 = load i32, ptr %ni.addr, align 4
  %cmp = icmp slt i32 %2, %3
  br i1 %cmp, label %for.body, label %for.end12

for.body:                                         ; preds = %for.cond
  store i32 0, ptr %j, align 4
  br label %for.cond2

for.cond2:                                        ; preds = %for.inc, %for.body
  %4 = load i32, ptr %j, align 4
  %5 = load i32, ptr %nj.addr, align 4
  %cmp3 = icmp slt i32 %4, %5
  br i1 %cmp3, label %for.body4, label %for.end

for.body4:                                        ; preds = %for.cond2
  %6 = load i32, ptr %i, align 4
  %7 = load i32, ptr %ni.addr, align 4
  %mul = mul nsw i32 %6, %7
  %8 = load i32, ptr %j, align 4
  %add = add nsw i32 %mul, %8
  %rem = srem i32 %add, 20
  %cmp5 = icmp eq i32 %rem, 0
  br i1 %cmp5, label %if.then, label %if.end

if.then:                                          ; preds = %for.body4
  %9 = load ptr, ptr @stderr, align 8
  %call6 = call i32 (ptr, ptr, ...) @fprintf(ptr noundef %9, ptr noundef @.str.4) #6
  br label %if.end

if.end:                                           ; preds = %if.then, %for.body4
  %10 = load ptr, ptr @stderr, align 8
  %11 = load ptr, ptr %C.addr, align 8
  %12 = load i32, ptr %i, align 4
  %idxprom = sext i32 %12 to i64
  %arrayidx = getelementptr inbounds [1100 x double], ptr %11, i64 %idxprom
  %13 = load i32, ptr %j, align 4
  %idxprom7 = sext i32 %13 to i64
  %arrayidx8 = getelementptr inbounds [1100 x double], ptr %arrayidx, i64 0, i64 %idxprom7
  %14 = load double, ptr %arrayidx8, align 8
  %call9 = call i32 (ptr, ptr, ...) @fprintf(ptr noundef %10, ptr noundef @.str.5, double noundef %14) #6
  br label %for.inc

for.inc:                                          ; preds = %if.end
  %15 = load i32, ptr %j, align 4
  %inc = add nsw i32 %15, 1
  store i32 %inc, ptr %j, align 4
  br label %for.cond2, !llvm.loop !28

for.end:                                          ; preds = %for.cond2
  br label %for.inc10

for.inc10:                                        ; preds = %for.end
  %16 = load i32, ptr %i, align 4
  %inc11 = add nsw i32 %16, 1
  store i32 %inc11, ptr %i, align 4
  br label %for.cond, !llvm.loop !30

for.end12:                                        ; preds = %for.cond
  %17 = load ptr, ptr @stderr, align 8
  %call13 = call i32 (ptr, ptr, ...) @fprintf(ptr noundef %17, ptr noundef @.str.6, ptr noundef @.str.3) #6
  %18 = load ptr, ptr @stderr, align 8
  %call14 = call i32 (ptr, ptr, ...) @fprintf(ptr noundef %18, ptr noundef @.str.7) #6
  ret void
}

; Function Attrs: nounwind
declare void @free(ptr noundef) #3

; Function Attrs: nocallback nocreateundeforpoison nofree nosync nounwind speculatable willreturn memory(none)
declare double @llvm.fmuladd.f64(double, double, double) #4

; Function Attrs: nounwind
declare i32 @fprintf(ptr noundef, ptr noundef, ...) #3

attributes #0 = { noinline nounwind uwtable "frame-pointer"="all" "min-legal-vector-width"="0" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cmov,+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "tune-cpu"="generic" }
attributes #1 = { "frame-pointer"="all" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cmov,+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "tune-cpu"="generic" }
attributes #2 = { nounwind willreturn memory(read) "frame-pointer"="all" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cmov,+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "tune-cpu"="generic" }
attributes #3 = { nounwind "frame-pointer"="all" "no-trapping-math"="true" "stack-protector-buffer-size"="8" "target-cpu"="x86-64" "target-features"="+cmov,+cx8,+fxsr,+mmx,+sse,+sse2,+x87" "tune-cpu"="generic" }
attributes #4 = { nocallback nocreateundeforpoison nofree nosync nounwind speculatable willreturn memory(none) }
attributes #5 = { nounwind willreturn memory(read) }
attributes #6 = { nounwind }

!llvm.module.flags = !{!0, !1, !2, !3, !4}
!llvm.ident = !{!5}

!0 = !{i32 1, !"wchar_size", i32 4}
!1 = !{i32 8, !"PIC Level", i32 2}
!2 = !{i32 7, !"PIE Level", i32 2}
!3 = !{i32 7, !"uwtable", i32 2}
!4 = !{i32 7, !"frame-pointer", i32 2}
!5 = !{!"clang version 22.1.6 (https://github.com/llvm/llvm-project.git fc4aad7b5db3fff421df9a9637605b9ca5667881)"}
!6 = distinct !{!6, !7, !8}
!7 = !{!"llvm.loop.mustprogress"}
!8 = !{!"compiler_cost_model.loop_id", i32 5}
!9 = distinct !{!9, !7, !10}
!10 = !{!"compiler_cost_model.loop_id", i32 4}
!11 = distinct !{!11, !7, !12, !13}
!12 = !{!"compiler_cost_model.loop_id", i32 3}
!13 = !{!"llvm.loop.unroll.count", i32 4}
!14 = distinct !{!14, !7, !15}
!15 = !{!"compiler_cost_model.loop_id", i32 2}
!16 = distinct !{!16, !7, !17}
!17 = !{!"compiler_cost_model.loop_id", i32 1}
!18 = distinct !{!18, !7, !19}
!19 = !{!"compiler_cost_model.loop_id", i32 0}
!20 = distinct !{!20, !7, !21}
!21 = !{!"compiler_cost_model.loop_id", i32 7}
!22 = distinct !{!22, !7, !23}
!23 = !{!"compiler_cost_model.loop_id", i32 9}
!24 = distinct !{!24, !7, !25}
!25 = !{!"compiler_cost_model.loop_id", i32 8}
!26 = distinct !{!26, !7, !27}
!27 = !{!"compiler_cost_model.loop_id", i32 6}
!28 = distinct !{!28, !7, !29}
!29 = !{!"compiler_cost_model.loop_id", i32 11}
!30 = distinct !{!30, !7, !31}
!31 = !{!"compiler_cost_model.loop_id", i32 10}
