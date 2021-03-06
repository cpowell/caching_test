#include <algorithm>
#include <chrono>
#include <iostream>
#include <map>
#include <string>
#include <unordered_map>
#include <vector>

#include "tbb/concurrent_hash_map.h"
#include "tbb/concurrent_vector.h"
#include "tbb/tbb.h"

#define RUNSIZE 100'000'000
#undef VERBOSE
#undef CACHING
#undef PROGRESS

class Sequence {
private:
  unsigned long seed_;

#ifdef CACHING
  // static std::unordered_map<int, int> smart_cache_;
  static std::map<int, int> smart_cache_;
// static std::vector<int> smart_cache_;
#endif

public:
  Sequence(int seed) : seed_(seed) {
    // nop
  }

  unsigned long nextValueFor(const unsigned long present_value) {
    if (present_value % 2 == 0) {
      return present_value / 2;
    } else {
      return 3 * present_value + 1;
    }
  }

  int calculate() {
#ifdef VERBOSE
    std::cout << seed_ << " => ";
#endif
    int length = 1;
    unsigned long current = seed_;

    while (current != 1) {
#ifdef VERBOSE
      std::cout << ".";
#endif
      current = nextValueFor(current);

#ifdef CACHING
      auto search = Sequence::smart_cache_.find(current);
      if (search != Sequence::smart_cache_.end()) {
        Sequence::smart_cache_[seed_] = length + search->second;
        // std::cout << "!" << std::endl;
        return length + search->second;
      }
// if (Sequence::smart_cache_[current] != 0) {
//    Sequence::smart_cache_[seed_] = length + Sequence::smart_cache_[current];
//    //std::cout << "!" << std::endl;

//    return Sequence::smart_cache_[seed_];
//}
#endif

      ++length;
    }

#ifdef CACHING
    Sequence::smart_cache_[seed_] = length;
#endif

#ifdef VERBOSE
    std::cout << std::endl;
#endif
    return length;
  }
};

#ifdef CACHING
std::map<int, int> Sequence::smart_cache_;
// std::vector<int> Sequence::smart_cache_(1'000'000, 0);
#endif

class Timer {
public:
  Timer() : beg_(clock_::now()) {}
  void reset() { beg_ = clock_::now(); }
  double elapsed() const {
    return std::chrono::duration_cast<second_>(clock_::now() - beg_).count();
  }

private:
  typedef std::chrono::high_resolution_clock clock_;
  typedef std::chrono::duration<double, std::ratio<1>> second_;
  std::chrono::time_point<clock_> beg_;
};

int main() {
  tbb::concurrent_vector<int> seeds_to_lengths_(RUNSIZE + 1, 0);

  Timer tmr;

  tbb::parallel_for(1, RUNSIZE, 1, [&seeds_to_lengths_](int s) {
    Sequence seq{s};
    seeds_to_lengths_[s] = seq.calculate();
#ifdef PROGRESS
    if (s % 1 == 0) {
      std::cout << ".";
    }
    return;
#endif
  });

  double t = tmr.elapsed();
  std::cout << t << " seconds" << std::endl;

  auto pElem = std::max_element(std::begin(seeds_to_lengths_),
                                std::end(seeds_to_lengths_));
  std::cout << std::endl
            << "The longest length of " << *pElem
            << " terms is for starting seed "
            << std::distance(std::begin(seeds_to_lengths_), pElem) << '\n';

  return 0;
}
