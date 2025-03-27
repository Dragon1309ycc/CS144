#include "reassembler.hh"
#include "debug.hh"

using namespace std;

void Reassembler::insert( uint64_t first_index, string data, bool is_last_substring )
{
  if(is_last_substring)
  {
    last_index_ = first_index;                                //如果为最后一部分的子字符串，将其记录
  }
  uint64_t stream_index = output_.writer().bytes_pushed();    //连续的字节流已经被push进入buffer区了
  if (first_index <= stream_index)                            //丢弃重复数据，保留有用数据
  {
    uint64_t cut_size = stream_index - first_index;
    if(cut_size > data.size())
    {
      return;
    }
    else
    {
      data = data.substr(cut_size);                           //更新data
      output_.writer().push(data);                            //直接push
      stream_index += data.size();                            //更新stream_index
    }
  }
  else
  {
    //对于有用数据的存储以及状态量的更新
    Reassembler_buffer_[first_index] = data;                  //存入数据
    R_buffer_size_ += data.size();                            //更新缓存区的数据量
    //写入操作
    while(Reassembler_buffer_.find(stream_index+1) != Reassembler_buffer_.end())
    {
      output_.writer().push(Reassembler_buffer_[stream_index+1]);   //插入相邻的子字符串
      R_buffer_size_ -=  Reassembler_buffer_[stream_index+1].size();//更新重组器缓存区的数据量
      Reassembler_buffer_.erase(stream_index+1);                    //更新重组器缓存区
      stream_index += Reassembler_buffer_[stream_index+1].size();   //更新流索引
    }

  }
  if((stream_index >= last_index_) && (output_.reader().bytes_buffered()==0))
  {
    output_.writer().close();                                 //表示写入结束，已经全部放入到了buffer中
  }
}

// How many bytes are stored in the Reassembler itself?
// This function is for testing only; don't add extra state to support it.
uint64_t Reassembler::count_bytes_pending() const
{
  return R_buffer_size_;
}
