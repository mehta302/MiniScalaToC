def sieve(n: Int): Array[Int] = {
  val isPrime = new Array[Int](n + 1);
  
  def init(i: Int): Unit = { 
    if (i <= n) { 
      isPrime(i) = 1; 
      init(i + 1) 
    } 
  };
  init(0);
  isPrime(0) = 0;
  isPrime(1) = 0;

  def outer(p: Int): Unit = {
    if (p * p <= n) {
      if (isPrime(p) == 1) {
        def inner(i: Int): Unit = {
          if (i <= n) { 
            isPrime(i) = 0; 
            inner(i + p) 
          }
        };
        inner(p * p)
      };
      outer(p + 1)
    }
  };
  outer(2);
  isPrime
};

def testSieve(): Int = {
  val max = 30;
  val primes = sieve(max);
  
  def printPrimes(i: Int): Unit = {
    if (i <= max) {
      if (primes(i) == 1) {
        putchar(i+48);
        putchar(' '.toInt)
      };
      printPrimes(i + 1)
    }
  };
  printPrimes(0);
  putchar('\n'.toInt);
  0
};

testSieve()
