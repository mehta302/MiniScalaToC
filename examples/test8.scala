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

def insertionSort(arr: Array[Int], len: Int): Unit = {
  def insert(i: Int): Unit = {
    if (i < len) {
      val key = arr(i);
      def shift(j: Int): Int = {
        if (j >= 0 && arr(j) > key) {
          arr(j + 1) = arr(j);
          shift(j - 1)
        } else j
      };
      val pos = shift(i - 1);
      arr(pos + 1) = key;
      insert(i + 1)
    }
  };
  insert(1)
};

def testSort(): Int = {
  val size = 6;
  val arr = new Array[Int](size);
  arr(0) = 42; arr(1) = 17; arr(2) = 8; 
  arr(3) = 99; arr(4) = 23; arr(5) = 4;
  
  printArray(arr, size);
  
  insertionSort(arr, size);
  
  printArray(arr, size);
  0
};

testSort()
