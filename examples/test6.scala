def printArray(arr: Array[Int], len: Int) = {
  def loop(i: Int): Unit = {
    if (i < len) {
      putchar(arr(i)+48);
      putchar(' '.toInt);
      loop(i + 1)
    }
  };
  putchar('['.toInt);
  loop(0);
  putchar(']'.toInt);
  putchar('\n'.toInt)
};

def reverseArray(arr: Array[Int], len: Int): Unit = {
  def swap(i: Int, j: Int): Unit = {
    if (i < j) {
      val temp = arr(i);
      arr(i) = arr(j);
      arr(j) = temp;
      swap(i + 1, j - 1)
    }
  };
  swap(0, len - 1)
};

def testReversal(): Int = {
  val size = 5;
  val arr = new Array[Int](size);
  
  def init(i: Int): Unit = {
    if (i < size) {
      arr(i) = (i + 1) * 10;
      init(i + 1)
    }
  };
  init(0);
  
  printArray(arr, size);
  
  reverseArray(arr, size);
  
  printArray(arr, size);
  0
};

testReversal()
