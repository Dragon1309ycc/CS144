#include "wrapping_integers.hh"
#include "debug.hh"
#include <cmath>

using namespace std;

Wrap32 Wrap32::wrap( uint64_t n, Wrap32 zero_point ) //这里的zerp_point 代表 ISN 初始序列号
{
  return zero_point + static_cast<uint32_t>(n);
}

uint64_t Wrap32::unwrap( Wrap32 zero_point, uint64_t checkpoint ) const //解包出最接近这个检查点的Wrap32格式序列号
{
  uint64_t num = checkpoint >> 32;
  uint32_t r2z_distance = this->raw_value_ - zero_point.raw_value_;
  uint64_t spare_one = (num << 32) + r2z_distance;
  uint64_t spare_two = ((num + 1) << 32) + r2z_distance;
  uint64_t spare_thr = (num > 0) ? ((num - 1) << 32) +r2z_distance : spare_one;
  uint64_t dis_one = spare_one > checkpoint ? spare_one - checkpoint : checkpoint - spare_one;
  uint64_t dis_two = spare_two > checkpoint ? spare_two - checkpoint : checkpoint - spare_two;
  uint64_t dis_thr = spare_thr > checkpoint ? spare_thr - checkpoint : checkpoint - spare_thr;
  if (dis_one <= dis_two && dis_one <= dis_thr) return spare_one;
  if (dis_two <= dis_one && dis_two <= dis_thr) return spare_two;
  return spare_thr;
}
