/**int foo(int e, int a){
**int b = a + 1;
**int c = b * 2;
**int f = a + 0;
**int g = f + 3;
**b = e << 1;
**int d = b / 4;
**return c * d;
**}
define dso_local i32 @foo (i32 noundef %0, i32 noundef %1) #0 {
  %3 = add nsw i32 %1, 1
  %4 = mul nsw i32 %3, 2
  %5 = add nsw i32 %1, 0
  %6 = mul nsw i32 %5, 3
  %7 = shl i32 %0, 1
  %8 = sdiv i32 %5, 4
  %9 = mul nsw i32 %4, %6
  ret i32 %7
}
