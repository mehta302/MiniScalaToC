// SUPPOSED TO FAIL
val x: Array[Int] = new Array[Int](1000000);
x(0) = 65;
x(1) = 66;
putchar(x(0));
putchar(x(1));
0
