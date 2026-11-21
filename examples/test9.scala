def buildHistogram(data: Array[Int], dataLen: Int): Array[Int] = {
  val hist = new Array[Int](10);
  
  def init(i: Int): Unit = { 
    if(i < 10) { 
      hist(i) = 0; 
      init(i + 1) 
    } 
  };
  init(0);

  def count(i: Int): Unit = {
    if (i < dataLen) {
      val v = data(i);
      if (v >= 0 && v <= 9) {
        hist(v) = hist(v) + 1
      };
      count(i + 1)
    }
  };
  count(0);
  hist
};

def testHistogram(): Int = {
  val len = 12;
  val rawData = new Array[Int](len);
  // data: [1, 2, 2, 5, 1, 9, 2, 5, 0, 1, 2, 8]
  rawData(0)=1; rawData(1)=2; rawData(2)=2; rawData(3)=5;
  rawData(4)=1; rawData(5)=9; rawData(6)=2; rawData(7)=5;
  rawData(8)=0; rawData(9)=1; rawData(10)=2; rawData(11)=8;

  val result = buildHistogram(rawData, len);
  
  def printHist(i: Int): Unit = {
    if (i < 10) {
      putchar(i+48); putchar(' '.toInt); putchar(result(i)+48);
      putchar('\n'.toInt);
      printHist(i + 1)
    }
  };
  putchar('\n'.toInt);
  printHist(0);
  0
};

testHistogram()
