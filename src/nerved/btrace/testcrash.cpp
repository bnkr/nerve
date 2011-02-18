#include "btrace/crash_detector.cpp"
#include "btrace/backtrace.cpp"
#include "btrace/demangle.cpp"

int main() {
  btrace::crash_detector cd;
  abort();
  //

  btrace::pretty_backtrace bt;
  std::for_each(bt.begin(), bt.end(), print_call);

  return 0;
}
