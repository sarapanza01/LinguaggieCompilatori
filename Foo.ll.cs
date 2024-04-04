; int foo(int e, int a) {
;   int b = a + 1;
;   int c = b - 1;
;   b = e << 1;
;   int d = b / 4;
;   int e = c + 2;
;   return c + d;
; }

define dso_local i32 @foo(i32 noundef %0, i32 noundef %1) #0 {
  %3 = add nsw i32 %1, 1
  %4 = sub nsw i32 %3, 1
  %5 = shl i32 %0, 1
  %6 = sdiv i32 %5, 4
  %7 = add nsw i32 %4, %6
  %8 = add nsw i32 %4, 2
  ret i32 %7
}