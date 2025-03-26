#include "byte_stream.hh"

using namespace std;

ByteStream::ByteStream( uint64_t capacity ) : capacity_( capacity ) {}

void Writer::push( string data )
{
  //可以写入的长度。
  auto len = min ( data.size() , available_capacity());
  //如果len为0，无法写入。如果len小于data长度，将data截短。
  if(len == 0) return;
  if(len < data.size()) data.resize(len);
  //将data写入缓冲区
  buffer_.push(move(data));
  //写入前为空的时候需要更新buffer_view_ （写入后大小为1）
  if(buffer_.size() == 1)
  {
    buffer_view_ = buffer_.front();
  }
  //更新已经写入的数据长度
  total_pushed_ += len;
}

void Writer::close()
{
  flag |= (1 << CLOSED);  // 将flag 设置为 0001
}

bool Writer::is_closed() const
{
  return flag & (1 << CLOSED); // 将flag 与 0001 相与计算
}

uint64_t Writer::available_capacity() const
{
  return {capacity_ - reader().bytes_buffered()}; // 可用的容量为 最大容量 - 已经用掉的容量
}

uint64_t Writer::bytes_pushed() const
{
  return total_pushed_; // 总共的交换数据量
}

//查看未被读取的字节流
string_view Reader::peek() const
{
  return buffer_view_;
}

//从缓冲区中弹出这些个字节
void Reader::pop( uint64_t len )
{
  //如果要弹出的字节数大于缓存区字节数的大小
  if( len > bytes_buffered())
  {
    return;
  }
  total_poped_ += len;
  while(len > 0)
  {
    if(len >= buffer_view_.size())
    {
      len -= buffer_view_.size();
      buffer_.pop();
      buffer_view_ = buffer_.front();
    }
    else{
      buffer_view_.remove_prefix(len);
      len = 0;
    }
  }
}

bool Reader::is_finished() const
{
  return writer().is_closed() && (bytes_buffered() == 0);  //检查输入结束状态还有缓冲区状态
}
uint64_t Reader::bytes_buffered() const
{
  return writer().bytes_pushed() - bytes_popped();        //缓存区中仍存在的字节数
}

uint64_t Reader::bytes_popped() const
{
  return total_poped_;                                    //总共弹出的字节数
}

