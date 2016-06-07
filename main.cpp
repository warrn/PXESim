#include "PXEClient.h"
#include "tftp.h"

using namespace Tins;

PacketSender sender("eth1");
PXEClient client;

bool sniff(Sniffer &sniffer) {
    // TODO: Refactor this mess

    PDU *pdu = sniffer.next_packet().pdu();
    const auto eth = pdu->rfind_pdu<EthernetII>();
    if (eth.dst_addr() == client.get_client_hw_address()) {
        if (pdu->find_pdu<ARP>()) {
            // ARP packet
            const auto arp = pdu->rfind_pdu<ARP>();
            if (arp.opcode() == arp.ARP::REQUEST) {
                std::cout << "ARP Request: " << arp.sender_ip_addr() << " ->? " << arp.target_ip_addr() << "\n";

                if (arp.target_ip_addr() == client.dhcp_client_address()) {
                    client.arp_reply_dhcp_server(sender, arp.sender_hw_addr());
                    std::cout << "ARP Reply sent on behalf of PXEClient ("
                    << client.dhcp_client_address() << ").\n";
                }
            }

            if (arp.opcode() == ARP::REPLY) {
                std::cout << "ARP Reply: " << arp.sender_ip_addr() << " @ " << arp.sender_hw_addr() << " -> " <<
                arp.target_ip_addr() << "\n";
                if (arp.target_ip_addr() == client.dhcp_client_address() && client.state() == ARPRequested) {
                    client.arp_reply_recieved(arp.sender_hw_addr());
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
                std::cout << "Pong sent from PXEClient: " << ip.dst_addr()
                << " to requestee " << ip.src_addr() << "\n";
            }

        } else if (pdu->find_pdu<UDP>() &&
                   (pdu->rfind_pdu<UDP>().sport() == 1024 || pdu->rfind_pdu<UDP>().dport() == 1024 ||
                    pdu->rfind_pdu<UDP>().sport() == 69 || pdu->rfind_pdu<UDP>().dport() == 69)) {
            //TFTP Packet
            const auto udp = pdu->rfind_pdu<UDP>();
            const auto tftp = pdu->rfind_pdu<RawPDU>().to<TFTP>();
            std::cout << "Found TFTP Packet Type: " << (uint16_t) tftp.opcode() << "\n";

            if (tftp.opcode() == TFTP::OPT_ACKNOWLEDGEMENT) {
                std::cout << "Total size: " << tftp.search_option("tsize").second << " bytes.\n";
                std::cout << "Block size: " << tftp.search_option("blksize").second << " bytes.\n";

                client.tftp_ack_options(sender, udp.sport(),
                                        (uint16_t) atoi(tftp.search_option("blksize").second.data()),
                                        (uint32_t) atoi(tftp.search_option("tsize").second.data()));

            } else if (tftp.opcode() == TFTP::DATA) {
                std::cout << "Block recieved: #" << tftp.block() << ".\n";
                client.tftp_ack_data(sender, udp.sport(), tftp.data(), tftp.block());

            } else if (tftp.opcode() == TFTP::ERROR) {
                std::cout << "TFTP Error #" << (uint16_t) tftp.error_code() << ": " << tftp.error() << "\n";
                if (tftp.error_code() == TFTP::FILE_NOT_FOUND || tftp.error_code() == TFTP::ILLEGAL_OPERATION)
                    client.tftp_not_found_error();
                else return false;
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
    while (client.state() != Terminated && client.state() != Completed) {
        sniff(sniffer);
        if (client.state() == ARPRecieved || client.state() == TFTPWaitingConfigRequest ||
            client.state() == TFTPWaitingFilesRequest) {
            client.tftp_read(sender);
            std::cout << "Sent TFTP READ Packet.\n";
        }
    }
}