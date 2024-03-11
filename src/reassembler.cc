#include "reassembler.hh"
#include <algorithm>
#include <limits>
#include <iostream>

using namespace std; 

Reassembler::Reassembler() : firstAcceptIndex_(0), firstRejectIndex_(0), endIndex_(numeric_limits<long long>::max()), inited_(false), reassemblerBuf_(), flagBuf_() { }
Reassembler::~Reassembler() { }

void Reassembler::insert( uint64_t first_index, string data, bool is_last_substring, Writer& output )
{
	if (!inited_) {
		reassemblerBuf_.resize(output.available_capacity(), 0);
		flagBuf_.resize(output.available_capacity(), false);
		inited_ = true;
	}
	if (is_last_substring) {
		endIndex_ = first_index + data.size(); // more than the final char
	}
	// Accept interval -> [firstAcceptIndex_, firstRejectIndex_)
	// data's interval -> [first_index, first_index + data.size() - 1]
	firstAcceptIndex_ = output.bytes_pushed();
	firstRejectIndex_ = firstAcceptIndex_ + output.available_capacity();
	if (data.empty() == false) {
		if (first_index + data.size() - 1< firstAcceptIndex_ || first_index >= firstRejectIndex_)
			data = "";
		else {
			uint64_t str_begin = max(firstAcceptIndex_, first_index);
			uint64_t str_end = min(firstRejectIndex_ - 1, first_index + data.size() - 1);
			for (uint64_t i = str_begin; i <= str_end; i++) {
				reassemblerBuf_[i - firstAcceptIndex_] = data[i - first_index];
				flagBuf_[i - firstAcceptIndex_] = true; 
			}
		}
	}
	string tmp;
	while (flagBuf_[0] == true) {
		tmp += reassemblerBuf_[0];
		reassemblerBuf_.pop_front();
		flagBuf_.pop_front();
		reassemblerBuf_.push_back(0);
		flagBuf_.push_back(false);
	}
	output.push(tmp);
	if (output.bytes_pushed() == endIndex_)
		output.close();
  // Your code here.
  (void)first_index;
  (void)data;
  (void)is_last_substring;
  (void)output;
}
uint64_t Reassembler::bytes_pending() const
{
	return static_cast<uint64_t> (count(flagBuf_.begin(), flagBuf_.end(), true));
  // Your code here.
  // return 0;
}

