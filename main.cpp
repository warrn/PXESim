#include "PXEClient.h"
#include "tftp.h"

using namespace Tins;

PacketSender sender("eth1");
PXEClient client;

bool sniff(Sniffer &sniffer) {
    // TODO: Refactor this mess
    for (Packet &pkt: sniffer) {
        PDU *pdu = pkt.pdu();
        if (pdu->find_pdu<ARP>()) {
            // ARP packet
            const auto arp = pdu->rfind_pdu<ARP>();
            if (arp.opcode() == arp.ARP::REQUEST) {
                std::cout << "ARP Request: " << arp.sender_ip_addr() << " ->? " << arp.target_ip_addr() << "\n";
                if (arp.target_ip_addr() == client.dhcp_client_address()) {
                    client.arp_reply_dhcp_server(sender, arp.sender_hw_addr());
                    std::cout << "ARP Reply sent on behalf of PXEClient (" << client.dhcp_client_address() << ").\n";
                }
            }
            if (arp.opcode() == ARP::REPLY) {
                std::cout << "ARP Reply: " << arp.sender_ip_addr() << " @ " << arp.sender_hw_addr() << " -> " <<
                arp.target_ip_addr() << "\n";
                if (arp.target_ip_addr() == client.dhcp_client_address()) {
                    client.tftp_hw_address(arp.sender_hw_addr());
                    std::cout << "SENDING TFTP READ Packet.\n";
                    client.tftp_read(sender);
                    std::cout << "SENT TFTP READ Packet.\n";
                }
            }
        } else if (pdu->find_pdu<ICMP>()) {
            // Ping packet
            const auto eth = pdu->rfind_pdu<EthernetII>();
            const auto icmp = pdu->rfind_pdu<ICMP>();
            const auto ip = pdu->rfind_pdu<IP>();
            if (icmp.type() == ICMP::ECHO_REQUEST && ip.dst_addr() == client.dhcp_client_address()) {
                std::cout << "Ping received from: " << ip.src_addr() << " to client " << ip.dst_addr() << "\n";
                client.pong(sender, ip.src_addr(), eth.src_addr(), icmp);
                std::cout << "Pong sent from PXEClient: " << ip.dst_addr() << " to requestee " << ip.src_addr() << "\n";
            }
        } else if (pdu->find_pdu<UDP>() &&
                   (pdu->rfind_pdu<UDP>().dport() == 1024 || pdu->rfind_pdu<UDP>().dport() == 1024)) {
            //TFTP Packet
            const auto tftp = pdu->rfind_pdu<RawPDU>().to<TFTP>();
            std::cout << "Found TFTP Packet Type: " << (uint16_t) tftp.opcode() << "\n";
            if (tftp.opcode() == TFTP::OPT_ACKNOWLEDGEMENT) {
                std::cout << "Total size: " << tftp.search_option("tsize").second << " bytes.\n";
                std::cout << "Block size: " << tftp.search_option("blksize").second << " bytes.\n";
            }
        } else if (pdu->find_pdu<UDP>() &&
                   (pdu->rfind_pdu<UDP>().dport() == 67 || pdu->rfind_pdu<UDP>().dport() == 68)) {
            // DHCP Packet
            const auto dhcp = pdu->rfind_pdu<RawPDU>().to<DHCP>();

            if (dhcp.type() == DHCP::OFFER) {
                std::cout << "DHCP Server: " << dhcp.siaddr().to_string() << "\n";
                std::cout << "Offered IP: " << dhcp.yiaddr().to_string() << "\n";
                client.dhcp_request(sender, dhcp.siaddr(), dhcp.yiaddr());
                std::cout << "Requested IP: " << dhcp.yiaddr().to_string() << "\n";
            }

            if (dhcp.type() == DHCP::ACK) {
                std::cout << "Acknowledged IP: " << dhcp.yiaddr().to_string() << "\n";
                std::cout << "Client Acknowledged? " << client.dhcp_acknowledged(dhcp) << "\n";
                client.arp_request_dhcp_server(sender);
                std::cout << "ARP Request Sent\n";
            }

            if (dhcp.type() == DHCP::NAK) {
                std::cerr << "Request not acknowledged, error in request: NAK\n";
            }
        }
    }

    return true;
}

int main() {
    SnifferConfiguration config;
    config.set_promisc_mode(true);
    config.set_timeout(1);
    config.set_filter("icmp or arp or udp port 68 or udp port 1024");
    Sniffer sniffer("eth1", config);
    client.dhcp_discover(sender);
    while (sniff(sniffer)) {
        // pass
    }
}