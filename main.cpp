#include "PXEClient.h"

using namespace Tins;

PacketSender sender("eth1");
PacketWriter writer("/root/test.pcap", PacketWriter::ETH2);
PXEClient client;

bool callback(const PDU &pdu) {
    auto dhcp = pdu.rfind_pdu<RawPDU>().to<DHCP>();
    auto arp = pdu.rfind_pdu<RawPDU>().to<ARP>();

    // Retrieve the queries and print the dhcp details:
    if (dhcp.type() == DHCP::OFFER) {
        std::cout << "DHCP Server: " << dhcp.siaddr().to_string() << "\n";
        std::cout << "Offered IP: " << dhcp.yiaddr().to_string() << "\n";
        auto request = client.create_dhcp_request(dhcp.siaddr(), dhcp.yiaddr());
        sender.send(request);
        std::cout << "Requested IP: " << dhcp.yiaddr().to_string() << "\n";
    }
    if (dhcp.type() == DHCP::ACK) {
        std::cout << "Acknowledged IP: " << dhcp.yiaddr().to_string() << "\n";
        std::cout << "Client Acknowledged? " << client.dhcp_acknowledged(dhcp) << "\n";
        auto request = client.create_arp_request_to_dhcp_server();
        sender.send(request);
        std::cout << "ARP Request Sent\n";
    }
    if (dhcp.type() == DHCP::NAK) {
        std::cerr << "Request not acknowledged, error in request: NAK\n";
    }
    if (arp.opcode() == arp.ARP::REQUEST) {
        std::cout << "ARP Request: " << arp.sender_ip_addr() << " ->? " << arp.target_hw_addr() << "\n";
    }
    if (arp.opcode() == ARP::REPLY) {
        std::cout << "ARP Reply: " << arp.sender_ip_addr() << " @ " << arp.sender_hw_addr() << " -> " <<
        arp.target_ip_addr() << "\n";
    }

    return true;
}

int main() {
    SnifferConfiguration config;
    config.set_promisc_mode(true);
    config.set_filter("arp or udp");
    Sniffer sniff("eth1", config);
    auto eth = client.create_dhcp_discover();
    sender.send(eth);
    sniff.sniff_loop(callback);
    writer.~PacketWriter();
}