#include "wrapping_integers.hh"
#include <limits>
#include <functional>

using namespace std;

Wrap32 Wrap32::wrap( uint64_t n, Wrap32 zero_point )
{
	return Wrap32(static_cast<uint32_t>(n) & numeric_limits<uint32_t>::max()) + zero_point.raw_value_;
  // Your code here.
  (void)n;
  (void)zero_point;
  // return Wrap32 { 0 };
}

uint64_t Wrap32::unwrap( Wrap32 zero_point, uint64_t checkpoint ) const
{
	function<uint64_t(uint64_t, uint64_t)> calDis = [](uint64_t a, uint64_t b) -> uint64_t {
		if (a >= b)
			return a - b;
		return b - a;
	};
	uint64_t abs_seq = static_cast<uint64_t>(this->raw_value_ - zero_point.raw_value_);
	uint64_t times_mod = checkpoint >> 32;
	uint64_t remain = checkpoint << 32 >> 32;
	uint64_t bound;
	if (remain > (1UL << 31))
		bound = times_mod + 1;
	else
		bound = times_mod;
	uint64_t seq_left = ((bound == 0? 0 : bound - 1) << 32) + abs_seq;
	uint64_t seq_right = (bound << 32) + abs_seq;
	return calDis(seq_left, checkpoint) < calDis(seq_right, checkpoint)? seq_left : seq_right;
  // Your code here.
  (void)zero_point;
  (void)checkpoint;
  return {};
}
