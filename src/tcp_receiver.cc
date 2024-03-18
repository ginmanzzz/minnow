#include "tcp_receiver.hh"

using namespace std;

void TCPReceiver::receive( TCPSenderMessage message, Reassembler& reassembler, Writer& inbound_stream )
{
	if (message.SYN) {
		ISN_ = Wrap32(message.seqno);
		is_isn_set = true;
		message.seqno = message.seqno + 1; // redirect the sequence number
	}
	if (message.FIN) {
		is_fin_set = true;
		FIN_ = message.seqno + message.payload.size();
	} else {
		is_fin_set = false;
	}
	if (is_isn_set) {
		reassembler.insert(message.seqno.unwrap(ISN_, inbound_stream.bytes_pushed()) - 1, message.payload.release(),
				is_fin_set, inbound_stream);
	}
  // Your code here.
  (void)message;
  (void)reassembler;
  (void)inbound_stream;
}

TCPReceiverMessage TCPReceiver::send( const Writer& inbound_stream ) const
{
	TCPReceiverMessage rec_ack;
	Wrap32 ack_seq = Wrap32::wrap(inbound_stream.bytes_pushed() + 1, ISN_);
	if (is_isn_set)
		rec_ack.ackno = ack_seq == FIN_? ack_seq + 1 : ack_seq;
	rec_ack.window_size = inbound_stream.available_capacity() > UINT16_MAX? UINT16_MAX : inbound_stream.available_capacity();
	return rec_ack;
  // Your code here.
  (void)inbound_stream;
  return {};
}
