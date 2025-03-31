#include "wrapping_integers.hh"
#include "debug.hh"

using namespace std;

Wrap32 Wrap32::wrap( uint64_t n, Wrap32 zero_point ) //这里的zerp_point 就是 ISN 初始序列号
{
  //将绝对序列号转换成序列号
  return  static_cast<Wrap32>(n) + zero_point.raw_value_;
}

uint64_t Wrap32::unwrap( Wrap32 zero_point, uint64_t checkpoint ) const
{
  
  return {};
}
