#include "PXEClient.h"

using namespace Tins;

PacketSender sender("eth1");
PacketWriter writer("/root/test.pcap", PacketWriter::ETH2);
PXEClient client;

bool callback(const PDU &pdu) {
    if (pdu.find_pdu<ARP>()) {
        const auto arp = pdu.rfind_pdu<ARP>();
        if (arp.opcode() == arp.ARP::REQUEST) {
            std::cout << "ARP Request: " << arp.sender_ip_addr() << " ->? " << arp.target_ip_addr() << "\n";
            if (arp.target_ip_addr() == client.get_dhcp_client_address()) {
                auto reply = client.create_arp_reply_to_dhcp_server(arp.sender_hw_addr());
                sender.send(reply);
            }
        }
        if (arp.opcode() == ARP::REPLY) {
            std::cout << "ARP Reply: " << arp.sender_ip_addr() << " @ " << arp.sender_hw_addr() << " -> " <<
            arp.target_ip_addr() << "\n";
        }
    } else if (pdu.find_pdu<ICMP>()) {
        const auto eth = pdu.rfind_pdu<EthernetII>();
        const auto icmp = pdu.rfind_pdu<ICMP>();
        const auto ip = pdu.rfind_pdu<IP>();
        if (icmp.type() == ICMP::ECHO_REQUEST && ip.dst_addr() == client.get_dhcp_client_address()) {
            std::cout << "Ping received from: " << ip.src_addr() << " to client " << ip.dst_addr() << "\n";
            auto reply = client.create_pong(ip.src_addr(), eth.src_addr(), icmp);
            sender.send(reply);
            std::cout << "Pong Sent.\n";
        }
    } else {
        auto dhcp = pdu.rfind_pdu<RawPDU>().to<DHCP>();
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
    }
    return true;
}

int main() {
    SnifferConfiguration config;
    config.set_promisc_mode(true);
    config.set_timeout(1);
    config.set_filter("icmp or arp or udp port 68");
    Sniffer sniff("eth1", config);
    auto eth = client.create_dhcp_discover();
    sender.send(eth);
    sniff.sniff_loop(callback);
    writer.~PacketWriter();
}