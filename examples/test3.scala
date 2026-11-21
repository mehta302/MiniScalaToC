def allocate_large() = {
  val x: Array[Int] = new Array[Int](200000);
  x(0) = 65;
  x(1) = 66;
  putchar(x(0));
  putchar(x(1))
};
allocate_large();
allocate_large();
0
