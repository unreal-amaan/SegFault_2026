; ModuleID = '/home/syed/Amaan/amaan_codes/compilers/compiler-cost-model-data/benchmarks/polybench/linear-algebra/blas/gemver/gemver.c'
source_filename = "/home/syed/Amaan/amaan_codes/compilers/compiler-cost-model-data/benchmarks/polybench/linear-algebra/blas/gemver/gemver.c"
target datalayout = "e-m:e-p270:32:32-p271:32:32-p272:64:64-i64:64-i128:128-f80:128-n8:16:32:64-S128"
target triple = "x86_64-unknown-linux-gnu"

@.str = private unnamed_addr constant [1 x i8] zeroinitializer, align 1
@stderr = external global ptr, align 8
@.str.1 = private unnamed_addr constant [23 x i8] c"==BEGIN DUMP_ARRAYS==\0A\00", align 1
@.str.2 = private unnamed_addr constant [15 x i8] c"begin dump: %s\00", align 1
@.str.3 = private unnamed_addr constant [2 x i8] c"w\00", align 1
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
  %n = alloca i32, align 4
  %alpha = alloca double, align 8
  %beta = alloca double, align 8
  %A = alloca ptr, align 8
  %u1 = alloca ptr, align 8
  %v1 = alloca ptr, align 8
  %u2 = alloca ptr, align 8
  %v2 = alloca ptr, align 8
  %w = alloca ptr, align 8
  %x = alloca ptr, align 8
  %y = alloca ptr, align 8
  %z = alloca ptr, align 8
  store i32 0, ptr %retval, align 4
  store i32 %argc, ptr %argc.addr, align 4
  store ptr %argv, ptr %argv.addr, align 8
  store i32 2000, ptr %n, align 4
  %call = call ptr @polybench_alloc_data(i64 noundef 4000000, i32 noundef 8)
  store ptr %call, ptr %A, align 8
  %call1 = call ptr @polybench_alloc_data(i64 noundef 2000, i32 noundef 8)
  store ptr %call1, ptr %u1, align 8
  %call2 = call ptr @polybench_alloc_data(i64 noundef 2000, i32 noundef 8)
  store ptr %call2, ptr %v1, align 8
  %call3 = call ptr @polybench_alloc_data(i64 noundef 2000, i32 noundef 8)
  store ptr %call3, ptr %u2, align 8
  %call4 = call ptr @polybench_alloc_data(i64 noundef 2000, i32 noundef 8)
  store ptr %call4, ptr %v2, align 8
  %call5 = call ptr @polybench_alloc_data(i64 noundef 2000, i32 noundef 8)
  store ptr %call5, ptr %w, align 8
  %call6 = call ptr @polybench_alloc_data(i64 noundef 2000, i32 noundef 8)
  store ptr %call6, ptr %x, align 8
  %call7 = call ptr @polybench_alloc_data(i64 noundef 2000, i32 noundef 8)
  store ptr %call7, ptr %y, align 8
  %call8 = call ptr @polybench_alloc_data(i64 noundef 2000, i32 noundef 8)
  store ptr %call8, ptr %z, align 8
  %0 = load i32, ptr %n, align 4
  %1 = load ptr, ptr %A, align 8
  %arraydecay = getelementptr inbounds [2000 x [2000 x double]], ptr %1, i64 0, i64 0
  %2 = load ptr, ptr %u1, align 8
  %arraydecay9 = getelementptr inbounds [2000 x double], ptr %2, i64 0, i64 0
  %3 = load ptr, ptr %v1, align 8
  %arraydecay10 = getelementptr inbounds [2000 x double], ptr %3, i64 0, i64 0
  %4 = load ptr, ptr %u2, align 8
  %arraydecay11 = getelementptr inbounds [2000 x double], ptr %4, i64 0, i64 0
  %5 = load ptr, ptr %v2, align 8
  %arraydecay12 = getelementptr inbounds [2000 x double], ptr %5, i64 0, i64 0
  %6 = load ptr, ptr %w, align 8
  %arraydecay13 = getelementptr inbounds [2000 x double], ptr %6, i64 0, i64 0
  %7 = load ptr, ptr %x, align 8
  %arraydecay14 = getelementptr inbounds [2000 x double], ptr %7, i64 0, i64 0
  %8 = load ptr, ptr %y, align 8
  %arraydecay15 = getelementptr inbounds [2000 x double], ptr %8, i64 0, i64 0
  %9 = load ptr, ptr %z, align 8
  %arraydecay16 = getelementptr inbounds [2000 x double], ptr %9, i64 0, i64 0
  call void @init_array(i32 noundef %0, ptr noundef %alpha, ptr noundef %beta, ptr noundef %arraydecay, ptr noundef %arraydecay9, ptr noundef %arraydecay10, ptr noundef %arraydecay11, ptr noundef %arraydecay12, ptr noundef %arraydecay13, ptr noundef %arraydecay14, ptr noundef %arraydecay15, ptr noundef %arraydecay16)
  %10 = load i32, ptr %n, align 4
  %11 = load double, ptr %alpha, align 8
  %12 = load double, ptr %beta, align 8
  %13 = load ptr, ptr %A, align 8
  %arraydecay17 = getelementptr inbounds [2000 x [2000 x double]], ptr %13, i64 0, i64 0
  %14 = load ptr, ptr %u1, align 8
  %arraydecay18 = getelementptr inbounds [2000 x double], ptr %14, i64 0, i64 0
  %15 = load ptr, ptr %v1, align 8
  %arraydecay19 = getelementptr inbounds [2000 x double], ptr %15, i64 0, i64 0
  %16 = load ptr, ptr %u2, align 8
  %arraydecay20 = getelementptr inbounds [2000 x double], ptr %16, i64 0, i64 0
  %17 = load ptr, ptr %v2, align 8
  %arraydecay21 = getelementptr inbounds [2000 x double], ptr %17, i64 0, i64 0
  %18 = load ptr, ptr %w, align 8
  %arraydecay22 = getelementptr inbounds [2000 x double], ptr %18, i64 0, i64 0
  %19 = load ptr, ptr %x, align 8
  %arraydecay23 = getelementptr inbounds [2000 x double], ptr %19, i64 0, i64 0
  %20 = load ptr, ptr %y, align 8
  %arraydecay24 = getelementptr inbounds [2000 x double], ptr %20, i64 0, i64 0
  %21 = load ptr, ptr %z, align 8
  %arraydecay25 = getelementptr inbounds [2000 x double], ptr %21, i64 0, i64 0
  call void @kernel_gemver(i32 noundef %10, double noundef %11, double noundef %12, ptr noundef %arraydecay17, ptr noundef %arraydecay18, ptr noundef %arraydecay19, ptr noundef %arraydecay20, ptr noundef %arraydecay21, ptr noundef %arraydecay22, ptr noundef %arraydecay23, ptr noundef %arraydecay24, ptr noundef %arraydecay25)
  %22 = load i32, ptr %argc.addr, align 4
  %cmp = icmp sgt i32 %22, 42
  br i1 %cmp, label %land.lhs.true, label %if.end

land.lhs.true:                                    ; preds = %entry
  %23 = load ptr, ptr %argv.addr, align 8
  %arrayidx = getelementptr inbounds ptr, ptr %23, i64 0
  %24 = load ptr, ptr %arrayidx, align 8
  %call26 = call i32 @strcmp(ptr noundef %24, ptr noundef @.str) #5
  %tobool = icmp ne i32 %call26, 0
  br i1 %tobool, label %if.end, label %if.then

if.then:                                          ; preds = %land.lhs.true
  %25 = load i32, ptr %n, align 4
  %26 = load ptr, ptr %w, align 8
  %arraydecay27 = getelementptr inbounds [2000 x double], ptr %26, i64 0, i64 0
  call void @print_array(i32 noundef %25, ptr noundef %arraydecay27)
  br label %if.end

if.end:                                           ; preds = %if.then, %land.lhs.true, %entry
  %27 = load ptr, ptr %A, align 8
  call void @free(ptr noundef %27) #6
  %28 = load ptr, ptr %u1, align 8
  call void @free(ptr noundef %28) #6
  %29 = load ptr, ptr %v1, align 8
  call void @free(ptr noundef %29) #6
  %30 = load ptr, ptr %u2, align 8
  call void @free(ptr noundef %30) #6
  %31 = load ptr, ptr %v2, align 8
  call void @free(ptr noundef %31) #6
  %32 = load ptr, ptr %w, align 8
  call void @free(ptr noundef %32) #6
  %33 = load ptr, ptr %x, align 8
  call void @free(ptr noundef %33) #6
  %34 = load ptr, ptr %y, align 8
  call void @free(ptr noundef %34) #6
  %35 = load ptr, ptr %z, align 8
  call void @free(ptr noundef %35) #6
  ret i32 0
}

declare ptr @polybench_alloc_data(i64 noundef, i32 noundef) #1

; Function Attrs: noinline nounwind uwtable
define internal void @init_array(i32 noundef %n, ptr noundef %alpha, ptr noundef %beta, ptr noundef %A, ptr noundef %u1, ptr noundef %v1, ptr noundef %u2, ptr noundef %v2, ptr noundef %w, ptr noundef %x, ptr noundef %y, ptr noundef %z) #0 {
entry:
  %n.addr = alloca i32, align 4
  %alpha.addr = alloca ptr, align 8
  %beta.addr = alloca ptr, align 8
  %A.addr = alloca ptr, align 8
  %u1.addr = alloca ptr, align 8
  %v1.addr = alloca ptr, align 8
  %u2.addr = alloca ptr, align 8
  %v2.addr = alloca ptr, align 8
  %w.addr = alloca ptr, align 8
  %x.addr = alloca ptr, align 8
  %y.addr = alloca ptr, align 8
  %z.addr = alloca ptr, align 8
  %i = alloca i32, align 4
  %j = alloca i32, align 4
  %fn = alloca double, align 8
  store i32 %n, ptr %n.addr, align 4
  store ptr %alpha, ptr %alpha.addr, align 8
  store ptr %beta, ptr %beta.addr, align 8
  store ptr %A, ptr %A.addr, align 8
  store ptr %u1, ptr %u1.addr, align 8
  store ptr %v1, ptr %v1.addr, align 8
  store ptr %u2, ptr %u2.addr, align 8
  store ptr %v2, ptr %v2.addr, align 8
  store ptr %w, ptr %w.addr, align 8
  store ptr %x, ptr %x.addr, align 8
  store ptr %y, ptr %y.addr, align 8
  store ptr %z, ptr %z.addr, align 8
  %0 = load ptr, ptr %alpha.addr, align 8
  store double 1.500000e+00, ptr %0, align 8
  %1 = load ptr, ptr %beta.addr, align 8
  store double 1.200000e+00, ptr %1, align 8
  %2 = load i32, ptr %n.addr, align 4
  %conv = sitofp i32 %2 to double
  store double %conv, ptr %fn, align 8
  store i32 0, ptr %i, align 4
  br label %for.cond

for.cond:                                         ; preds = %for.inc46, %entry
  %3 = load i32, ptr %i, align 4
  %4 = load i32, ptr %n.addr, align 4
  %cmp = icmp slt i32 %3, %4
  br i1 %cmp, label %for.body, label %for.end48

for.body:                                         ; preds = %for.cond
  %5 = load i32, ptr %i, align 4
  %conv2 = sitofp i32 %5 to double
  %6 = load ptr, ptr %u1.addr, align 8
  %7 = load i32, ptr %i, align 4
  %idxprom = sext i32 %7 to i64
  %arrayidx = getelementptr inbounds double, ptr %6, i64 %idxprom
  store double %conv2, ptr %arrayidx, align 8
  %8 = load i32, ptr %i, align 4
  %add = add nsw i32 %8, 1
  %conv3 = sitofp i32 %add to double
  %9 = load double, ptr %fn, align 8
  %div = fdiv double %conv3, %9
  %div4 = fdiv double %div, 2.000000e+00
  %10 = load ptr, ptr %u2.addr, align 8
  %11 = load i32, ptr %i, align 4
  %idxprom5 = sext i32 %11 to i64
  %arrayidx6 = getelementptr inbounds double, ptr %10, i64 %idxprom5
  store double %div4, ptr %arrayidx6, align 8
  %12 = load i32, ptr %i, align 4
  %add7 = add nsw i32 %12, 1
  %conv8 = sitofp i32 %add7 to double
  %13 = load double, ptr %fn, align 8
  %div9 = fdiv double %conv8, %13
  %div10 = fdiv double %div9, 4.000000e+00
  %14 = load ptr, ptr %v1.addr, align 8
  %15 = load i32, ptr %i, align 4
  %idxprom11 = sext i32 %15 to i64
  %arrayidx12 = getelementptr inbounds double, ptr %14, i64 %idxprom11
  store double %div10, ptr %arrayidx12, align 8
  %16 = load i32, ptr %i, align 4
  %add13 = add nsw i32 %16, 1
  %conv14 = sitofp i32 %add13 to double
  %17 = load double, ptr %fn, align 8
  %div15 = fdiv double %conv14, %17
  %div16 = fdiv double %div15, 6.000000e+00
  %18 = load ptr, ptr %v2.addr, align 8
  %19 = load i32, ptr %i, align 4
  %idxprom17 = sext i32 %19 to i64
  %arrayidx18 = getelementptr inbounds double, ptr %18, i64 %idxprom17
  store double %div16, ptr %arrayidx18, align 8
  %20 = load i32, ptr %i, align 4
  %add19 = add nsw i32 %20, 1
  %conv20 = sitofp i32 %add19 to double
  %21 = load double, ptr %fn, align 8
  %div21 = fdiv double %conv20, %21
  %div22 = fdiv double %div21, 8.000000e+00
  %22 = load ptr, ptr %y.addr, align 8
  %23 = load i32, ptr %i, align 4
  %idxprom23 = sext i32 %23 to i64
  %arrayidx24 = getelementptr inbounds double, ptr %22, i64 %idxprom23
  store double %div22, ptr %arrayidx24, align 8
  %24 = load i32, ptr %i, align 4
  %add25 = add nsw i32 %24, 1
  %conv26 = sitofp i32 %add25 to double
  %25 = load double, ptr %fn, align 8
  %div27 = fdiv double %conv26, %25
  %div28 = fdiv double %div27, 9.000000e+00
  %26 = load ptr, ptr %z.addr, align 8
  %27 = load i32, ptr %i, align 4
  %idxprom29 = sext i32 %27 to i64
  %arrayidx30 = getelementptr inbounds double, ptr %26, i64 %idxprom29
  store double %div28, ptr %arrayidx30, align 8
  %28 = load ptr, ptr %x.addr, align 8
  %29 = load i32, ptr %i, align 4
  %idxprom31 = sext i32 %29 to i64
  %arrayidx32 = getelementptr inbounds double, ptr %28, i64 %idxprom31
  store double 0.000000e+00, ptr %arrayidx32, align 8
  %30 = load ptr, ptr %w.addr, align 8
  %31 = load i32, ptr %i, align 4
  %idxprom33 = sext i32 %31 to i64
  %arrayidx34 = getelementptr inbounds double, ptr %30, i64 %idxprom33
  store double 0.000000e+00, ptr %arrayidx34, align 8
  store i32 0, ptr %j, align 4
  br label %for.cond35

for.cond35:                                       ; preds = %for.inc, %for.body
  %32 = load i32, ptr %j, align 4
  %33 = load i32, ptr %n.addr, align 4
  %cmp36 = icmp slt i32 %32, %33
  br i1 %cmp36, label %for.body38, label %for.end

for.body38:                                       ; preds = %for.cond35
  %34 = load i32, ptr %i, align 4
  %35 = load i32, ptr %j, align 4
  %mul = mul nsw i32 %34, %35
  %36 = load i32, ptr %n.addr, align 4
  %rem = srem i32 %mul, %36
  %conv39 = sitofp i32 %rem to double
  %37 = load i32, ptr %n.addr, align 4
  %conv40 = sitofp i32 %37 to double
  %div41 = fdiv double %conv39, %conv40
  %38 = load ptr, ptr %A.addr, align 8
  %39 = load i32, ptr %i, align 4
  %idxprom42 = sext i32 %39 to i64
  %arrayidx43 = getelementptr inbounds [2000 x double], ptr %38, i64 %idxprom42
  %40 = load i32, ptr %j, align 4
  %idxprom44 = sext i32 %40 to i64
  %arrayidx45 = getelementptr inbounds [2000 x double], ptr %arrayidx43, i64 0, i64 %idxprom44
  store double %div41, ptr %arrayidx45, align 8
  br label %for.inc

for.inc:                                          ; preds = %for.body38
  %41 = load i32, ptr %j, align 4
  %inc = add nsw i32 %41, 1
  store i32 %inc, ptr %j, align 4
  br label %for.cond35, !llvm.loop !6

for.end:                                          ; preds = %for.cond35
  br label %for.inc46

for.inc46:                                        ; preds = %for.end
  %42 = load i32, ptr %i, align 4
  %inc47 = add nsw i32 %42, 1
  store i32 %inc47, ptr %i, align 4
  br label %for.cond, !llvm.loop !8

for.end48:                                        ; preds = %for.cond
  ret void
}

; Function Attrs: noinline nounwind uwtable
define internal void @kernel_gemver(i32 noundef %n, double noundef %alpha, double noundef %beta, ptr noundef %A, ptr noundef %u1, ptr noundef %v1, ptr noundef %u2, ptr noundef %v2, ptr noundef %w, ptr noundef %x, ptr noundef %y, ptr noundef %z) #0 {
entry:
  %n.addr = alloca i32, align 4
  %alpha.addr = alloca double, align 8
  %beta.addr = alloca double, align 8
  %A.addr = alloca ptr, align 8
  %u1.addr = alloca ptr, align 8
  %v1.addr = alloca ptr, align 8
  %u2.addr = alloca ptr, align 8
  %v2.addr = alloca ptr, align 8
  %w.addr = alloca ptr, align 8
  %x.addr = alloca ptr, align 8
  %y.addr = alloca ptr, align 8
  %z.addr = alloca ptr, align 8
  %i = alloca i32, align 4
  %j = alloca i32, align 4
  store i32 %n, ptr %n.addr, align 4
  store double %alpha, ptr %alpha.addr, align 8
  store double %beta, ptr %beta.addr, align 8
  store ptr %A, ptr %A.addr, align 8
  store ptr %u1, ptr %u1.addr, align 8
  store ptr %v1, ptr %v1.addr, align 8
  store ptr %u2, ptr %u2.addr, align 8
  store ptr %v2, ptr %v2.addr, align 8
  store ptr %w, ptr %w.addr, align 8
  store ptr %x, ptr %x.addr, align 8
  store ptr %y, ptr %y.addr, align 8
  store ptr %z, ptr %z.addr, align 8
  call void @__costmodel_tiling(i32 noundef 32)
  store i32 0, ptr %i, align 4
  br label %for.cond

for.cond:                                         ; preds = %for.inc18, %entry
  %0 = load i32, ptr %i, align 4
  %1 = load i32, ptr %n.addr, align 4
  %cmp = icmp slt i32 %0, %1
  br i1 %cmp, label %for.body, label %for.end20

for.body:                                         ; preds = %for.cond
  store i32 0, ptr %j, align 4
  br label %for.cond1

for.cond1:                                        ; preds = %for.inc, %for.body
  %2 = load i32, ptr %j, align 4
  %3 = load i32, ptr %n.addr, align 4
  %cmp2 = icmp slt i32 %2, %3
  br i1 %cmp2, label %for.body3, label %for.end

for.body3:                                        ; preds = %for.cond1
  %4 = load ptr, ptr %A.addr, align 8
  %5 = load i32, ptr %i, align 4
  %idxprom = sext i32 %5 to i64
  %arrayidx = getelementptr inbounds [2000 x double], ptr %4, i64 %idxprom
  %6 = load i32, ptr %j, align 4
  %idxprom4 = sext i32 %6 to i64
  %arrayidx5 = getelementptr inbounds [2000 x double], ptr %arrayidx, i64 0, i64 %idxprom4
  %7 = load double, ptr %arrayidx5, align 8
  %8 = load ptr, ptr %u1.addr, align 8
  %9 = load i32, ptr %i, align 4
  %idxprom6 = sext i32 %9 to i64
  %arrayidx7 = getelementptr inbounds double, ptr %8, i64 %idxprom6
  %10 = load double, ptr %arrayidx7, align 8
  %11 = load ptr, ptr %v1.addr, align 8
  %12 = load i32, ptr %j, align 4
  %idxprom8 = sext i32 %12 to i64
  %arrayidx9 = getelementptr inbounds double, ptr %11, i64 %idxprom8
  %13 = load double, ptr %arrayidx9, align 8
  %14 = call double @llvm.fmuladd.f64(double %10, double %13, double %7)
  %15 = load ptr, ptr %u2.addr, align 8
  %16 = load i32, ptr %i, align 4
  %idxprom10 = sext i32 %16 to i64
  %arrayidx11 = getelementptr inbounds double, ptr %15, i64 %idxprom10
  %17 = load double, ptr %arrayidx11, align 8
  %18 = load ptr, ptr %v2.addr, align 8
  %19 = load i32, ptr %j, align 4
  %idxprom12 = sext i32 %19 to i64
  %arrayidx13 = getelementptr inbounds double, ptr %18, i64 %idxprom12
  %20 = load double, ptr %arrayidx13, align 8
  %21 = call double @llvm.fmuladd.f64(double %17, double %20, double %14)
  %22 = load ptr, ptr %A.addr, align 8
  %23 = load i32, ptr %i, align 4
  %idxprom14 = sext i32 %23 to i64
  %arrayidx15 = getelementptr inbounds [2000 x double], ptr %22, i64 %idxprom14
  %24 = load i32, ptr %j, align 4
  %idxprom16 = sext i32 %24 to i64
  %arrayidx17 = getelementptr inbounds [2000 x double], ptr %arrayidx15, i64 0, i64 %idxprom16
  store double %21, ptr %arrayidx17, align 8
  br label %for.inc

for.inc:                                          ; preds = %for.body3
  %25 = load i32, ptr %j, align 4
  %inc = add nsw i32 %25, 1
  store i32 %inc, ptr %j, align 4
  br label %for.cond1, !llvm.loop !9

for.end:                                          ; preds = %for.cond1
  br label %for.inc18

for.inc18:                                        ; preds = %for.end
  %26 = load i32, ptr %i, align 4
  %inc19 = add nsw i32 %26, 1
  store i32 %inc19, ptr %i, align 4
  br label %for.cond, !llvm.loop !10

for.end20:                                        ; preds = %for.cond
  call void @__costmodel_fusion()
  store i32 0, ptr %i, align 4
  br label %for.cond21

for.cond21:                                       ; preds = %for.inc41, %for.end20
  %27 = load i32, ptr %i, align 4
  %28 = load i32, ptr %n.addr, align 4
  %cmp22 = icmp slt i32 %27, %28
  br i1 %cmp22, label %for.body23, label %for.end43

for.body23:                                       ; preds = %for.cond21
  store i32 0, ptr %j, align 4
  br label %for.cond24

for.cond24:                                       ; preds = %for.inc38, %for.body23
  %29 = load i32, ptr %j, align 4
  %30 = load i32, ptr %n.addr, align 4
  %cmp25 = icmp slt i32 %29, %30
  br i1 %cmp25, label %for.body26, label %for.end40

for.body26:                                       ; preds = %for.cond24
  %31 = load ptr, ptr %x.addr, align 8
  %32 = load i32, ptr %i, align 4
  %idxprom27 = sext i32 %32 to i64
  %arrayidx28 = getelementptr inbounds double, ptr %31, i64 %idxprom27
  %33 = load double, ptr %arrayidx28, align 8
  %34 = load double, ptr %beta.addr, align 8
  %35 = load ptr, ptr %A.addr, align 8
  %36 = load i32, ptr %j, align 4
  %idxprom29 = sext i32 %36 to i64
  %arrayidx30 = getelementptr inbounds [2000 x double], ptr %35, i64 %idxprom29
  %37 = load i32, ptr %i, align 4
  %idxprom31 = sext i32 %37 to i64
  %arrayidx32 = getelementptr inbounds [2000 x double], ptr %arrayidx30, i64 0, i64 %idxprom31
  %38 = load double, ptr %arrayidx32, align 8
  %mul = fmul double %34, %38
  %39 = load ptr, ptr %y.addr, align 8
  %40 = load i32, ptr %j, align 4
  %idxprom33 = sext i32 %40 to i64
  %arrayidx34 = getelementptr inbounds double, ptr %39, i64 %idxprom33
  %41 = load double, ptr %arrayidx34, align 8
  %42 = call double @llvm.fmuladd.f64(double %mul, double %41, double %33)
  %43 = load ptr, ptr %x.addr, align 8
  %44 = load i32, ptr %i, align 4
  %idxprom36 = sext i32 %44 to i64
  %arrayidx37 = getelementptr inbounds double, ptr %43, i64 %idxprom36
  store double %42, ptr %arrayidx37, align 8
  br label %for.inc38

for.inc38:                                        ; preds = %for.body26
  %45 = load i32, ptr %j, align 4
  %inc39 = add nsw i32 %45, 1
  store i32 %inc39, ptr %j, align 4
  br label %for.cond24, !llvm.loop !11

for.end40:                                        ; preds = %for.cond24
  br label %for.inc41

for.inc41:                                        ; preds = %for.end40
  %46 = load i32, ptr %i, align 4
  %inc42 = add nsw i32 %46, 1
  store i32 %inc42, ptr %i, align 4
  br label %for.cond21, !llvm.loop !12

for.end43:                                        ; preds = %for.cond21
  call void @__costmodel_unroll(i32 noundef 4)
  store i32 0, ptr %i, align 4
  br label %for.cond44

for.cond44:                                       ; preds = %for.inc53, %for.end43
  %47 = load i32, ptr %i, align 4
  %48 = load i32, ptr %n.addr, align 4
  %cmp45 = icmp slt i32 %47, %48
  br i1 %cmp45, label %for.body46, label %for.end55

for.body46:                                       ; preds = %for.cond44
  %49 = load ptr, ptr %x.addr, align 8
  %50 = load i32, ptr %i, align 4
  %idxprom47 = sext i32 %50 to i64
  %arrayidx48 = getelementptr inbounds double, ptr %49, i64 %idxprom47
  %51 = load double, ptr %arrayidx48, align 8
  %52 = load ptr, ptr %z.addr, align 8
  %53 = load i32, ptr %i, align 4
  %idxprom49 = sext i32 %53 to i64
  %arrayidx50 = getelementptr inbounds double, ptr %52, i64 %idxprom49
  %54 = load double, ptr %arrayidx50, align 8
  %add = fadd double %51, %54
  %55 = load ptr, ptr %x.addr, align 8
  %56 = load i32, ptr %i, align 4
  %idxprom51 = sext i32 %56 to i64
  %arrayidx52 = getelementptr inbounds double, ptr %55, i64 %idxprom51
  store double %add, ptr %arrayidx52, align 8
  br label %for.inc53

for.inc53:                                        ; preds = %for.body46
  %57 = load i32, ptr %i, align 4
  %inc54 = add nsw i32 %57, 1
  store i32 %inc54, ptr %i, align 4
  br label %for.cond44, !llvm.loop !13

for.end55:                                        ; preds = %for.cond44
  call void @__costmodel_vectorize(i32 noundef 4, i32 noundef 2)
  store i32 0, ptr %i, align 4
  br label %for.cond56

for.cond56:                                       ; preds = %for.inc77, %for.end55
  %58 = load i32, ptr %i, align 4
  %59 = load i32, ptr %n.addr, align 4
  %cmp57 = icmp slt i32 %58, %59
  br i1 %cmp57, label %for.body58, label %for.end79

for.body58:                                       ; preds = %for.cond56
  store i32 0, ptr %j, align 4
  br label %for.cond59

for.cond59:                                       ; preds = %for.inc74, %for.body58
  %60 = load i32, ptr %j, align 4
  %61 = load i32, ptr %n.addr, align 4
  %cmp60 = icmp slt i32 %60, %61
  br i1 %cmp60, label %for.body61, label %for.end76

for.body61:                                       ; preds = %for.cond59
  %62 = load ptr, ptr %w.addr, align 8
  %63 = load i32, ptr %i, align 4
  %idxprom62 = sext i32 %63 to i64
  %arrayidx63 = getelementptr inbounds double, ptr %62, i64 %idxprom62
  %64 = load double, ptr %arrayidx63, align 8
  %65 = load double, ptr %alpha.addr, align 8
  %66 = load ptr, ptr %A.addr, align 8
  %67 = load i32, ptr %i, align 4
  %idxprom64 = sext i32 %67 to i64
  %arrayidx65 = getelementptr inbounds [2000 x double], ptr %66, i64 %idxprom64
  %68 = load i32, ptr %j, align 4
  %idxprom66 = sext i32 %68 to i64
  %arrayidx67 = getelementptr inbounds [2000 x double], ptr %arrayidx65, i64 0, i64 %idxprom66
  %69 = load double, ptr %arrayidx67, align 8
  %mul68 = fmul double %65, %69
  %70 = load ptr, ptr %x.addr, align 8
  %71 = load i32, ptr %j, align 4
  %idxprom69 = sext i32 %71 to i64
  %arrayidx70 = getelementptr inbounds double, ptr %70, i64 %idxprom69
  %72 = load double, ptr %arrayidx70, align 8
  %73 = call double @llvm.fmuladd.f64(double %mul68, double %72, double %64)
  %74 = load ptr, ptr %w.addr, align 8
  %75 = load i32, ptr %i, align 4
  %idxprom72 = sext i32 %75 to i64
  %arrayidx73 = getelementptr inbounds double, ptr %74, i64 %idxprom72
  store double %73, ptr %arrayidx73, align 8
  br label %for.inc74

for.inc74:                                        ; preds = %for.body61
  %76 = load i32, ptr %j, align 4
  %inc75 = add nsw i32 %76, 1
  store i32 %inc75, ptr %j, align 4
  br label %for.cond59, !llvm.loop !14

for.end76:                                        ; preds = %for.cond59
  br label %for.inc77

for.inc77:                                        ; preds = %for.end76
  %77 = load i32, ptr %i, align 4
  %inc78 = add nsw i32 %77, 1
  store i32 %inc78, ptr %i, align 4
  br label %for.cond56, !llvm.loop !15

for.end79:                                        ; preds = %for.cond56
  ret void
}

; Function Attrs: nounwind willreturn memory(read)
declare i32 @strcmp(ptr noundef, ptr noundef) #2

; Function Attrs: noinline nounwind uwtable
define internal void @print_array(i32 noundef %n, ptr noundef %w) #0 {
entry:
  %n.addr = alloca i32, align 4
  %w.addr = alloca ptr, align 8
  %i = alloca i32, align 4
  store i32 %n, ptr %n.addr, align 4
  store ptr %w, ptr %w.addr, align 8
  %0 = load ptr, ptr @stderr, align 8
  %call = call i32 (ptr, ptr, ...) @fprintf(ptr noundef %0, ptr noundef @.str.1) #6
  %1 = load ptr, ptr @stderr, align 8
  %call1 = call i32 (ptr, ptr, ...) @fprintf(ptr noundef %1, ptr noundef @.str.2, ptr noundef @.str.3) #6
  store i32 0, ptr %i, align 4
  br label %for.cond

for.cond:                                         ; preds = %for.inc, %entry
  %2 = load i32, ptr %i, align 4
  %3 = load i32, ptr %n.addr, align 4
  %cmp = icmp slt i32 %2, %3
  br i1 %cmp, label %for.body, label %for.end

for.body:                                         ; preds = %for.cond
  %4 = load i32, ptr %i, align 4
  %rem = srem i32 %4, 20
  %cmp2 = icmp eq i32 %rem, 0
  br i1 %cmp2, label %if.then, label %if.end

if.then:                                          ; preds = %for.body
  %5 = load ptr, ptr @stderr, align 8
  %call3 = call i32 (ptr, ptr, ...) @fprintf(ptr noundef %5, ptr noundef @.str.4) #6
  br label %if.end

if.end:                                           ; preds = %if.then, %for.body
  %6 = load ptr, ptr @stderr, align 8
  %7 = load ptr, ptr %w.addr, align 8
  %8 = load i32, ptr %i, align 4
  %idxprom = sext i32 %8 to i64
  %arrayidx = getelementptr inbounds double, ptr %7, i64 %idxprom
  %9 = load double, ptr %arrayidx, align 8
  %call4 = call i32 (ptr, ptr, ...) @fprintf(ptr noundef %6, ptr noundef @.str.5, double noundef %9) #6
  br label %for.inc

for.inc:                                          ; preds = %if.end
  %10 = load i32, ptr %i, align 4
  %inc = add nsw i32 %10, 1
  store i32 %inc, ptr %i, align 4
  br label %for.cond, !llvm.loop !16

for.end:                                          ; preds = %for.cond
  %11 = load ptr, ptr @stderr, align 8
  %call5 = call i32 (ptr, ptr, ...) @fprintf(ptr noundef %11, ptr noundef @.str.6, ptr noundef @.str.3) #6
  %12 = load ptr, ptr @stderr, align 8
  %call6 = call i32 (ptr, ptr, ...) @fprintf(ptr noundef %12, ptr noundef @.str.7) #6
  ret void
}

; Function Attrs: nounwind
declare void @free(ptr noundef) #3

declare void @__costmodel_tiling(i32 noundef) #1

; Function Attrs: nocallback nocreateundeforpoison nofree nosync nounwind speculatable willreturn memory(none)
declare double @llvm.fmuladd.f64(double, double, double) #4

declare void @__costmodel_fusion() #1

declare void @__costmodel_unroll(i32 noundef) #1

declare void @__costmodel_vectorize(i32 noundef, i32 noundef) #1

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
!6 = distinct !{!6, !7}
!7 = !{!"llvm.loop.mustprogress"}
!8 = distinct !{!8, !7}
!9 = distinct !{!9, !7}
!10 = distinct !{!10, !7}
!11 = distinct !{!11, !7}
!12 = distinct !{!12, !7}
!13 = distinct !{!13, !7}
!14 = distinct !{!14, !7}
!15 = distinct !{!15, !7}
!16 = distinct !{!16, !7}
