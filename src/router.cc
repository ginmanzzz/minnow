#include "router.hh"

#include <iostream>
#include <limits>

using namespace std;

Router::Router() : interfaces_(), rule_table_() { }

// route_prefix: The "up-to-32-bit" IPv4 address prefix to match the datagram's destination address against
// prefix_length: For this route to be applicable, how many high-order (most-significant) bits of
//    the route_prefix will need to match the corresponding bits of the datagram's destination address?
// next_hop: The IP address of the next hop. Will be empty if the network is directly attached to the router (in
//    which case, the next hop address should be the datagram's final destination).
// interface_num: The index of the interface to send the datagram out on.
void Router::add_route( const uint32_t route_prefix,
                        const uint8_t prefix_length,
                        const optional<Address> next_hop,
                        const size_t interface_num )
{
  cerr << "DEBUG: adding route " << Address::from_ipv4_numeric( route_prefix ).ip() << "/"
       << static_cast<int>( prefix_length ) << " => " << ( next_hop.has_value() ? next_hop->ip() : "(direct)" )
       << " on interface " << interface_num << "\n";
  rule_table_.emplace_back(route_prefix, prefix_length, next_hop, interface_num);
  (void)route_prefix;
  (void)prefix_length;
  (void)next_hop;
  (void)interface_num;
}

void Router::route() {
	for (auto& interface_ : interfaces_) {
		optional<InternetDatagram> dgram;
		while ( (dgram = interface_.maybe_receive()).has_value() ) {
			int longest_prefix = 0;
			optional<Rule> send_rule;
			for (auto rule : rule_table_) {
				if (longest_prefix == 0 && rule.prefix_length_ == 0) {
					// default route
					send_rule = rule;
					continue;
				}
				if (int tmp = countPrefixLength(dgram.value().header.dst, rule); tmp > longest_prefix) {
					longest_prefix = tmp;
					send_rule = rule;
				}
			}
			if (!send_rule.has_value() || dgram.value().header.ttl <= 1)
				continue;
			dgram->header.ttl--;
			dgram->header.compute_checksum();
			if (!send_rule.value().next_hop_.has_value()) {
				// if attach directly, rule's Address will be empty, use dst_ip to route
				interface(send_rule.value().interface_num_).send_datagram(dgram.value(), Address::from_ipv4_numeric(dgram->header.dst));
			} else {
				interface(send_rule.value().interface_num_).send_datagram(dgram.value(), send_rule.value().next_hop_.value());
			}
		}
	}
}

int Router::countPrefixLength(uint32_t ip, Rule& rule) {
	uint8_t cnt = 0;
	for (int i = 31; i >= 0; i--) {
		if (((ip >> i) & 1) == ((rule.route_prefix_ >> i) & 1))
			cnt++;
		else
			break;
	}
	return cnt >= rule.prefix_length_ ? rule.prefix_length_ : 0;
}
