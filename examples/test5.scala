// SUPPOSED TO FAIL
def allocate_large() = {
  val x: Array[Int] = new Array[Int](200000);
  x(0) = 65;
  x(1) = 66;
  putchar(x(0));
  putchar(x(1))
};

def checker_large(): Array[Array[Int]] = {
  val a: Array[Array[Int]] = new Array[Array[Int]](100);
  var i = 0;
  while (i < 200) {
    val y: Array[Int] = new Array[Int](1000);
    y(0) = 65;
    y(1) = 66;
    putchar(y(0));
    putchar(y(1));
    if (i % 2 == 0) a(i/2) = y;
    i = i + 1
  };
  a
};

val z: Array[Array[Int]] = checker_large();
allocate_large();
0
