#include "tcp_sender.hh"
#include "debug.hh"
#include "tcp_config.hh"

using namespace std;

// This function is for testing only; don't add extra state to support it.
uint64_t TCPSender::sequence_numbers_in_flight() const
{
  return numbers_in_flight_;
}

// This function is for testing only; don't add extra state to support it.
uint64_t TCPSender::consecutive_retransmissions() const
{
  return consecutive_retransmissions_;
}

void TCPSender::push( const TransmitFunction& transmit )
{

  if (send_FIN) return;

  uint16_t window_size = receive_window_size_ > 0 ? receive_window_size_ : 1; //初始假设为1

  TCPSenderMessage msg;

  //传输开始
  if( !send_SYN ) {

    msg.SYN = true;
    msg.seqno = isn_;
    msg.RST = reader().has_error();

    uint64_t window_left = receive_seqno_ + window_size > next_seqno_ ? receive_seqno_ + window_size - next_seqno_ : 0;
    if(!window_left) return;
    window_left = min( TCPConfig::MAX_PAYLOAD_SIZE , static_cast<size_t>( window_left ));

    if( reader().bytes_buffered() && receive_seqno_ + window_size > next_seqno_ ) {

      read( reader() , window_left , msg.payload );

    }

    if( reader().is_finished() && receive_seqno_ + window_size > next_seqno_ ) {

      msg.FIN = true;
      send_FIN = true;

    }

    transmit(msg);
    next_seqno_ += msg.sequence_length();
    send_SYN = true;
    numbers_in_flight_ += msg.sequence_length();
    wait_ack_msg.push(msg);

    if (!is_tick_running) {
      is_tick_running = true;
      passed_time_ = 0;
    }
    //5 所有未确认的数据都已经被确认时，关闭重传定时器
    if(wait_ack_msg.empty()) {
      is_tick_running = false;
    }
    return;
  }
  //传输结束
  if(reader().is_finished() && receive_seqno_ + window_size > next_seqno_) {

    msg.FIN = true;
    msg.RST = reader().has_error();
    msg.seqno = Wrap32::wrap( next_seqno_ , isn_);

    transmit(msg);
    next_seqno_ += msg.sequence_length();
    send_FIN = true;
    wait_ack_msg.push(msg);
    numbers_in_flight_ += msg.sequence_length();

    if (!is_tick_running) {
      is_tick_running = true;
      passed_time_ = 0;
    }
    //5 所有未确认的数据都已经被确认时，关闭重传定时器
    if(wait_ack_msg.empty()) {
      is_tick_running = false;
    }
    return;
  }
  //数据传输
  while( reader().bytes_buffered() && receive_seqno_ + window_size > next_seqno_ ) {

    uint64_t window_left = receive_seqno_ + window_size > next_seqno_ ? receive_seqno_ + window_size - next_seqno_ : 0;
    if(!window_left) return;
    window_left = min( TCPConfig::MAX_PAYLOAD_SIZE , static_cast<size_t>( window_left ));

    msg.RST = reader().has_error();
    msg.seqno = Wrap32::wrap( next_seqno_ , isn_ );    
    read( reader() , static_cast<size_t>( window_left ) , msg.payload );

    if (reader().is_finished() && ( msg.sequence_length() + next_seqno_ - receive_seqno_ ) < window_size ) {
      msg.FIN = true;
      send_FIN = true;
    }

    transmit(msg);

    next_seqno_ += msg.sequence_length();
    numbers_in_flight_ += msg.sequence_length();
    wait_ack_msg.push(msg);

    if (!is_tick_running) {
      is_tick_running = true;
      passed_time_ = 0;
    }
    //5 所有未确认的数据都已经被确认时，关闭重传定时器
    if(wait_ack_msg.empty()) {
      is_tick_running = false;
    }
  }
}

TCPSenderMessage TCPSender::make_empty_message() const
{
  TCPSenderMessage msg;
  msg.seqno = Wrap32::wrap( next_seqno_ , isn_ );
  msg.RST = reader().has_error();
  return msg;
}

void TCPSender::receive( const TCPReceiverMessage& msg )
{
  //传输错误
  if(msg.RST) {
    reader().set_error();
    return;
  }
  //未收到确认号
  if(!msg.ackno.has_value()) return;

  //绝对序列确认号
  uint64_t abs_seqno = msg.ackno.value().unwrap(isn_ , next_seqno_);

  if(abs_seqno > next_seqno_) { //这是一种不可能的情况
    //TCP是鲁棒的，不能因为确认号大于期望序列号就报错，而是应该忽略掉这个ACK
    return;
  }

  if(abs_seqno >= receive_seqno_) {
    //更新ack 和 window
    receive_seqno_ = abs_seqno;
    receive_window_size_ = msg.window_size;
  }

  while (!wait_ack_msg.empty()) {

    TCPSenderMessage send_msg = wait_ack_msg.front();
    // 未确认
    if (receive_seqno_ < send_msg.seqno.unwrap(isn_, next_seqno_) + send_msg.sequence_length()) {
      return;
    }else { // 已确认
      wait_ack_msg.pop();
      numbers_in_flight_ -= send_msg.sequence_length();
      //7 重置重传超时时间、重传次数与重传计时器
      RTO_ms_ = initial_RTO_ms_;
      consecutive_retransmissions_ = 0;
      passed_time_ = 0;
    }
  }
  // 重传计时器是否启动取决于发送方是否有未完成的数据
  is_tick_running = !wait_ack_msg.empty();
}

void TCPSender::tick( uint64_t ms_since_last_tick, const TransmitFunction& transmit )
{
  if(!is_tick_running) return;

  passed_time_ += ms_since_last_tick;

  //3 6
  if(passed_time_ >= RTO_ms_ && !wait_ack_msg.empty()) {
    //发送最早未确认的数据
    transmit(wait_ack_msg.front());
    //修改参数
    if(receive_window_size_ > 0) {
      RTO_ms_ *= 2;
      consecutive_retransmissions_ ++;
    }
    //重置
    passed_time_ = 0;
  }
}
