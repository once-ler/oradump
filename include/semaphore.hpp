#include <mutex>
#include <future>
// a semaphore class
//
// All threads can wait on this object. When a waiting thread
// is woken up, it does its work and then notifies another waiting thread.
// In this way only n threads will be be doing work at any time.
// 
class Semaphore {
private:
  std::mutex m;
  std::condition_variable cv;
  unsigned int count;

public:
  Semaphore(int n) : count(n) {}
  void notify() {
    std::unique_lock<std::mutex> l(m);
    ++count;
    cv.notify_one();
  }
  void wait() {
    std::unique_lock<std::mutex> l(m);
    cv.wait(l, [this]{ return count != 0; });
    --count;
  }
};

// an RAII class to handle waiting and notifying the next thread
// Work is done between when the object is created and destroyed
class Semaphore_waiter_notifier {
  Semaphore &s;
public:
  Semaphore_waiter_notifier(Semaphore &s) : s(s) { s.wait(); }
  ~Semaphore_waiter_notifier() { s.notify(); }
};

template<typename Iterator, typename Func>
void parallel_for_each(Iterator first, Iterator last, Func f)
{
  ptrdiff_t const range_length = last - first;
  if (!range_length)
    return;
  if (range_length == 1)
  {
    f(*first);
    return;
  }

  Iterator const mid = first + (range_length / 2);

  std::future<void> bgtask = std::async(launch::async, &parallel_for_each<Iterator, Func>,
    first, mid, f);
  try
  {
    parallel_for_each(mid, last, f);
  }
  catch (...)
  {
    bgtask.wait();
    throw;
  }
  bgtask.get();
}
