//
// Created by allenvic on 7/26/17.
//

#ifndef CHROMIUM_TIMER_H
#define CHROMIUM_TIMER_H

#include <chrono>
#include <iostream>

class Timer {
 public:
  Timer();
  void reset() { beg_ = clock_::now(); }
  double elapsed() const {
    return std::chrono::duration_cast<second_>(clock_::now() - beg_).count();
  }


 private:
  typedef std::chrono::high_resolution_clock clock_;
  typedef std::chrono::duration<double, std::ratio<1>> second_;
  std::chrono::time_point<clock_> beg_;
};

inline Timer::Timer() {
  beg_ = clock_::now();
}
#endif  // CHROMIUM_TIMER_H
