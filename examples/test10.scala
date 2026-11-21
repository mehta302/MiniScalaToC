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

def expandArray(oldArr: Array[Int], oldLen: Int, newElem: Int): Array[Int] = {
  val newLen = oldLen + 1;
  val newArr = new Array[Int](newLen);
  
  def copy(i: Int): Unit = {
    if (i < oldLen) {
      newArr(i) = oldArr(i);
      copy(i + 1)
    }
  };
  
  copy(0);
  newArr(oldLen) = newElem;
  newArr
};

def buildDynamic(targetSize: Int): Array[Int] = {
  def loop(i: Int, currentArr: Array[Int]): Array[Int] = {
    if (i >= targetSize) currentArr
    else {
      val nextArr = expandArray(currentArr, i, i * 10);
      loop(i + 1, nextArr)
    }
  };
  val empty = new Array[Int](0);
  loop(0, empty)
};

def testDynamic(): Int = {
  val finalSize = 20; 
  putchar(finalSize+48);
  putchar('\n'.toInt);
  
  val result = buildDynamic(finalSize);
  
  printArray(result, finalSize);
  0
};

testDynamic()
