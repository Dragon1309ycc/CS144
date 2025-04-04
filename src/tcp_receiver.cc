#include "tcp_receiver.hh"
#include "debug.hh"

using namespace std;

void TCPReceiver::receive( TCPSenderMessage message )   //SYN FIN seqno RST payload
{   
    if (message.RST) {

        reassembler_.reader().set_error();

    }

    if(message.SYN) { 

        zero_point.emplace(message.seqno);
        reassembler_.insert(0, message.payload, message.FIN);

    } else if(zero_point != nullopt) {

        // 这里的逻辑需要参考unwrap函数的具体实现过程
        uint64_t first_index =
            message.seqno.unwrap(zero_point.value(), reassembler_.writer().bytes_pushed()) - 1;

        reassembler_.insert(first_index, message.payload, message.FIN);

    }
    
}

TCPReceiverMessage TCPReceiver::send() const //ackno, window_size, RST
{
    TCPReceiverMessage msg{};

    if(zero_point.has_value()) {
        msg.ackno = Wrap32::wrap(reassembler_.writer().bytes_pushed() + 1, zero_point.value())
                    +reassembler_.writer().is_closed();
    }

    //window_size是16位无符号整形
    msg.window_size = reassembler_.writer().available_capacity() > (uint64_t)65535 ? 65535
        : reassembler_.writer().available_capacity();

    //复位标志
    msg.RST = reassembler_.reader().has_error();

    return msg;
}
