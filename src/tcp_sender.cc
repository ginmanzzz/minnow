#include "tcp_sender.hh"
#include "tcp_config.hh"

#include <random>

using namespace std;

/* TCPSender constructor (uses a random ISN if none given) */
TCPSender::TCPSender( uint64_t initial_RTO_ms, optional<Wrap32> fixed_isn )
  : isn_( fixed_isn.value_or( Wrap32 { random_device()() } ) ), 
  initial_RTO_ms_( initial_RTO_ms ),
  current_RTO_ms_(initial_RTO_ms),
  outstanding_bytes_(0),
  lastMsg_(),
  lastMsgPrimitiveWindowSize(1),
  abs_seqno_(0),
  isSentSYN_(false),
  isSentFIN_(false),
  alarmActivated_(false),
  alarmRemainTime_(initial_RTO_ms_),
  consecutive_retransmissions_(0),
  outstanding_collections_(),
  prepared_collections_()
{
	lastMsg_.ackno = isn_;
	lastMsg_.window_size = 1;
}

uint64_t TCPSender::sequence_numbers_in_flight() const
{
	return outstanding_bytes_;
  // Your code here.
  // return {};
}

uint64_t TCPSender::consecutive_retransmissions() const
{
	return consecutive_retransmissions_;
  // Your code here.
  // return {};
}

optional<TCPSenderMessage> TCPSender::maybe_send()
{
	if (prepared_collections_.empty()) 
		return nullopt;
	optional<TCPSenderMessage> msg_(prepared_collections_.front());
	prepared_collections_.pop_front();
	alarmActivated_ = true;
	return msg_;
  // Your code here.
  // return {};
}

// Reader provide interface:
// peek(), bytes_buffered(), bytes_poped(), pop()
void TCPSender::push( Reader& outbound_stream )
{
	while (outstanding_bytes_ < lastMsg_.window_size) {
		TCPSenderMessage msg_;
		if (!isSentSYN_) {
			isSentSYN_ = true;
			msg_.SYN = true;
			msg_.seqno = isn_;
		} else {
			msg_.seqno = Wrap32::wrap(abs_seqno_, isn_);
		}
		size_t payloadSize = min(min(outbound_stream.bytes_buffered(), TCPConfig::MAX_PAYLOAD_SIZE), static_cast<size_t>(lastMsg_.window_size - outstanding_bytes_));
		read(outbound_stream, payloadSize, msg_.payload); // including pop
		if (!isSentFIN_ && outbound_stream.is_finished() && outstanding_bytes_ + msg_.sequence_length() < lastMsg_.window_size) {
			isSentFIN_ = true;
			msg_.FIN = true;
		}
		if (msg_.sequence_length() == 0)
			break;
		else {
			outstanding_collections_.push_back(msg_);
			prepared_collections_.push_back(msg_);
			abs_seqno_ += msg_.sequence_length();
			outstanding_bytes_ += msg_.sequence_length();
		}
	}
  // Your code here.
  // (void)outbound_stream;
}

TCPSenderMessage TCPSender::send_empty_message() const
{
	TCPSenderMessage msg_;
	msg_.seqno = Wrap32::wrap(abs_seqno_, isn_);
	return msg_;
  // Your code here.
  // return {};
}

void TCPSender::receive( const TCPReceiverMessage& msg )
{
	lastMsg_ = msg;
	if (lastMsg_.window_size == 0)
		lastMsg_.window_size = 1;
	lastMsgPrimitiveWindowSize = msg.window_size;
	if (msg.ackno.has_value()) {
		// The receiver's ack shouldn't be future seq_no
		if (msg.ackno.value().unwrap(isn_, abs_seqno_) > abs_seqno_)
			return;
		while (outstanding_bytes_ != 0 && 
				outstanding_collections_.front().seqno.unwrap(isn_, abs_seqno_) + outstanding_collections_.front().sequence_length() <= lastMsg_.ackno.value().unwrap(isn_, abs_seqno_)) {
			outstanding_bytes_ -= outstanding_collections_.front().sequence_length();
			outstanding_collections_.pop_front();
		// reset the alarm
		if (outstanding_bytes_ == 0)
			alarmActivated_ = false;
		else
			alarmActivated_ = true;
		// only when get new ack, update the alarm
		consecutive_retransmissions_ = 0;
		current_RTO_ms_ = initial_RTO_ms_;
		alarmRemainTime_ = current_RTO_ms_;
		}
	}
  // Your code here.
  // (void)msg;
}

void TCPSender::tick( const size_t ms_since_last_tick )
{
	if (alarmActivated_) {
		alarmRemainTime_ -= static_cast<int>(ms_since_last_tick);
		if (alarmRemainTime_ <= 0) {
			prepared_collections_.push_front(outstanding_collections_.front()); // retransmit the earlist segment
			if (lastMsgPrimitiveWindowSize > 0) {
				consecutive_retransmissions_++;
				current_RTO_ms_ *= 2;
			} else {
				current_RTO_ms_ = initial_RTO_ms_;
			}
			alarmRemainTime_ = current_RTO_ms_;
		}
	}
  // Your code here.
  // (void)ms_since_last_tick;
}
