#include "network_interface.hh"

#include "arp_message.hh"
#include "ethernet_frame.hh"

using namespace std;

// ethernet_address: Ethernet (what ARP calls "hardware") address of the interface
// ip_address: IP (what ARP calls "protocol") address of the interface
NetworkInterface::NetworkInterface( const EthernetAddress& ethernet_address, const Address& ip_address )
  : ethernet_address_( ethernet_address ), ip_address_( ip_address ),
	can_sent_(), ARP_request_time_(), wait_for_addr_(), ip2Ether_(),
	ARP_TTL_(5000), MAP_TTL_(30000)
{
  cerr << "DEBUG: Network interface has Ethernet address " << to_string( ethernet_address_ ) << " and IP address "
       << ip_address.ip() << "\n";
}

// dgram: the IPv4 datagram to be sent
// next_hop: the IP address of the interface to send it to (typically a router or default gateway, but
// may also be another host if directly connected to the same network as the destination)

// Note: the Address type can be converted to a uint32_t (raw 32-bit IP address) by using the
// Address::ipv4_numeric() method.
void NetworkInterface::send_datagram( const InternetDatagram& dgram, const Address& next_hop )
{
	if (!ip2Ether_.count(next_hop.ipv4_numeric())) {
		// if not found ethernet_address mapping
		// store InternetDatagram, wait for replying next_hop's ethernet_address
		wait_for_addr_[next_hop.ipv4_numeric()].push_back(dgram);
		// create an ARP Message
		ARPMessage ARP_dgram;
		ARP_dgram.target_ip_address = next_hop.ipv4_numeric();
		ARP_dgram.sender_ip_address = ip_address_.ipv4_numeric();
		// ARP_dgram.target_ethernet_address = ETHERNET_BROADCAST;
		ARP_dgram.sender_ethernet_address = ethernet_address_;
		ARP_dgram.opcode = ARPMessage::OPCODE_REQUEST;
		// create an EthernetFrame
		EthernetFrame Ethernet_dgram;
		Ethernet_dgram.header.type = EthernetHeader::TYPE_ARP;
		Ethernet_dgram.payload = serialize(ARP_dgram);
		Ethernet_dgram.header.src = ethernet_address_;
		Ethernet_dgram.header.dst = ETHERNET_BROADCAST;
		can_sent_.push(Ethernet_dgram);
	} else {
		// if found ethernet_address mapping
		EthernetFrame Ethernet_dgram;
		Ethernet_dgram.header.type = EthernetHeader::TYPE_IPv4;
		Ethernet_dgram.payload = serialize(dgram);
		Ethernet_dgram.header.src = ethernet_address_;
		Ethernet_dgram.header.dst = ip2Ether_[next_hop.ipv4_numeric()].first;
		can_sent_.push(Ethernet_dgram);
	}
  (void)dgram;
  (void)next_hop;
}

// frame: the incoming Ethernet frame
optional<InternetDatagram> NetworkInterface::recv_frame( const EthernetFrame& frame )
{
	optional<InternetDatagram> ret;
	// only deal with appropriate EthernetFrame
	if (frame.header.dst != ETHERNET_BROADCAST && frame.header.dst != ethernet_address_)
		return nullopt;
	if (frame.header.type == EthernetHeader::TYPE_ARP) {
		ARPMessage recv_ARP_dgram;
		if (parse(recv_ARP_dgram, frame.payload)) {
			// update mapping
			ip2Ether_[recv_ARP_dgram.sender_ip_address] = {recv_ARP_dgram.sender_ethernet_address, MAP_TTL_};
			if (wait_for_addr_.count(recv_ARP_dgram.sender_ip_address)) {
				// found new ip address, start to send
				while (!wait_for_addr_[recv_ARP_dgram.sender_ip_address].empty()) {
					send_datagram(wait_for_addr_[recv_ARP_dgram.sender_ip_address].front(), Address::from_ipv4_numeric(recv_ARP_dgram.sender_ip_address));
					wait_for_addr_[recv_ARP_dgram.sender_ip_address].pop_front();
				}
				wait_for_addr_.erase(recv_ARP_dgram.sender_ip_address);
			}
			// if this ARPMessage is requesting out IP, reply it
			if (recv_ARP_dgram.opcode == ARPMessage::OPCODE_REQUEST && recv_ARP_dgram.target_ip_address == ip_address_.ipv4_numeric()) {
				ARPMessage send_ARP_dgram;
				send_ARP_dgram.sender_ip_address = ip_address_.ipv4_numeric();
				send_ARP_dgram.target_ip_address = recv_ARP_dgram.sender_ip_address;
				send_ARP_dgram.sender_ethernet_address = ethernet_address_;
				send_ARP_dgram.target_ethernet_address = recv_ARP_dgram.sender_ethernet_address;
				send_ARP_dgram.opcode = ARPMessage::OPCODE_REPLY;
				EthernetFrame Ethernet_dgram;
				Ethernet_dgram.header.type = EthernetHeader::TYPE_ARP;
				Ethernet_dgram.payload = serialize(send_ARP_dgram);
				Ethernet_dgram.header.src = ethernet_address_;
				Ethernet_dgram.header.dst = recv_ARP_dgram.sender_ethernet_address;
				can_sent_.push(Ethernet_dgram);
			}
		}
	} else if (frame.header.type == EthernetHeader::TYPE_IPv4) {
		InternetDatagram dgram;
		if (parse(dgram, frame.payload)) {
			// ret = optional<InternetDatagram>(dgram);
			ret = dgram;
		}
	}
	return ret;
  (void)frame;
  return {};
}

// ms_since_last_tick: the number of milliseconds since the last call to this method
void NetworkInterface::tick( const size_t ms_since_last_tick )
{
	for (auto it = ip2Ether_.begin(); it != ip2Ether_.end(); ) {
		// ip2Ether_ -> [ip, pair<EthernetAddress, lastTime>]
		it->second.second -= ms_since_last_tick;
		if (it->second.second <= 0) {
			// Note: C++11 can't do, C++17 can.
			// in C++11, unordered_map.erase() don't return the back it of erased it
			it = ip2Ether_.erase(it); 
		} else {
			it++;
		}
	}
	for (auto it = ARP_request_time_.begin(); it != ARP_request_time_.end(); ) {
		it->second -= ms_since_last_tick;
		if (it->second <= 0)  {
			it = ARP_request_time_.erase(it);
		} else {
			it++;
		}
	}
	/*
	for (auto& [ip, mapping_time] : ip2Ether_) {
		mapping_time.second -= ms_since_last_tick;
		if (mapping_time.second <= 0)
			ip2Ether_.erase(ip);
	}
	for (auto& [ip, ARP_time] : ARP_request_time_) {
		ARP_time -= ms_since_last_tick;
		if (ARP_time <= 0)
			ARP_request_time_.erase(ip);
	}
	*/
  (void)ms_since_last_tick;
}

optional<EthernetFrame> NetworkInterface::maybe_send()
{
	// default nullopt
	optional<EthernetFrame> ret;
	if (!can_sent_.empty()) {
		EthernetFrame front_elem = can_sent_.front();
		can_sent_.pop();
		if (front_elem.header.type == EthernetHeader::TYPE_IPv4) {
			// InternetDatagram
			// ret = optional<EthernetFrame>(front_elem);
			ret = front_elem;
		} else {
			// ARP Datagram
			ARPMessage send_ARP_dgram;
			if (parse(send_ARP_dgram, front_elem.payload)) {
				if (send_ARP_dgram.opcode == ARPMessage::OPCODE_REQUEST && ARP_request_time_.count(send_ARP_dgram.target_ip_address)) {
					return nullopt;
				}
				// ret = optional<EthernetFrame>(front_elem);
				ret = front_elem;
				ARP_request_time_[send_ARP_dgram.target_ip_address] = ARP_TTL_;
			}
		}
		return ret;
	}
	return nullopt;
  return {};
}
