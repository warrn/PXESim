//
// Created by warnelso on 5/31/16.
//
#include "tftp.h"
#include "PXEClient.h"

using namespace Tins;

const std::vector<uint8_t> array_from_uint32(const uint32_t number) {
    auto array = std::vector<uint8_t>(4, 0); // using a vector for memory management
    array[3] = (uint8_t) (number >> 24);
    array[2] = (uint8_t) (number >> 16);
    array[1] = (uint8_t) (number >> 8);
    array[0] = (uint8_t) (number);
    return array;
}


PXEClient::PXEClient() {
    srand(time(0));
    uint8_t id[] = {0x00, 0x0B, 0xCD, (uint8_t) rand(), (uint8_t) rand(), (uint8_t) rand()};
    // Simulating HP Servers
    this->_client_hw_address = HWAddress<6>(id);
    std::cout << "MAC: " << this->_client_hw_address << "\n";
    this->_state = New;
}

void PXEClient::dhcp_discover(Tins::PacketSender &sender) {
    srand(time(0));
    this->_dhcp_xid = (uint32_t) rand();
    EthernetII eth(HWAddress<6>::broadcast, this->_client_hw_address);
    auto *ip = new IP(IPv4Address::broadcast, IPv4Address());
    auto *udp = new UDP(67, 68);
    auto *dhcp = new DHCP();

    dhcp->xid(this->_dhcp_xid);
    dhcp->chaddr(HWAddress<6>(this->_client_hw_address));
    dhcp->type(DHCP::DISCOVER);
    dhcp->htype(0x01);
    dhcp->add_option({
                             DHCP::OptionTypes::DHCP_PARAMETER_REQUEST_LIST,
                             7,
                             (const unsigned char *) "\x01\x03\x06\x0c\x0f\x42\x43"
                             // Request Subnet Mask, Router, DNS, Host, Domain, TFTP, Bootfile
                     });
    dhcp->end();

    udp->inner_pdu(dhcp);
    ip->inner_pdu(udp);
    eth.inner_pdu(ip);

    this->_state = DHCPDiscovering;
    sender.send(eth);
}

void PXEClient::dhcp_request(Tins::PacketSender &sender, const Tins::IPv4Address &dhcp_server_address,
                             const Tins::IPv4Address &dhcp_client_address) {
    this->_dhcp_server_address = dhcp_server_address;
    this->_dhcp_client_address = dhcp_client_address;

    EthernetII eth(HWAddress<6>::broadcast, this->_client_hw_address);
    auto *ip = new IP(IPv4Address::broadcast, IPv4Address());
    auto *udp = new UDP(67, 68);
    auto *dhcp = new DHCP();

    dhcp->xid(this->_dhcp_xid);
    dhcp->chaddr(this->_client_hw_address);
    dhcp->type(DHCP::REQUEST);
    dhcp->htype(0x01);
    dhcp->add_option({
                             DHCP::OptionTypes::DHCP_PARAMETER_REQUEST_LIST,
                             7,
                             (const unsigned char *) "\x01\x03\x06\x0c\x0f\x42\x43"
                             // Request Subnet Mask, Router, DNS, Host, Domain, TFTP, Bootfile
                     });
    dhcp->add_option({
                             DHCP::OptionTypes::DHCP_SERVER_IDENTIFIER,
                             4,
                             array_from_uint32(this->_dhcp_server_address).data()
                     });
    dhcp->add_option({
                             DHCP::OptionTypes::DHCP_REQUESTED_ADDRESS,
                             4,
                             array_from_uint32(this->_dhcp_client_address).data()
                     });
    dhcp->end();

    udp->inner_pdu(dhcp);
    ip->inner_pdu(udp);
    eth.inner_pdu(ip);

    this->_state = DHCPRequested;
    sender.send(eth);
}

const bool PXEClient::dhcp_acknowledged(const DHCP &pdu) {
    if (this->_state >= DHCPAcknowledged) return true;
    if (pdu.yiaddr() == this->_dhcp_client_address && pdu.chaddr() == this->_client_hw_address) {
        this->_tftp_server_address = (char *) pdu.search_option((DHCP::OptionTypes) 66)->data_ptr();
        std::cout << "TFTP Server Saved: " << this->_tftp_server_address << "\n";
        this->_files_to_download.push((char *) pdu.search_option((DHCP::OptionTypes) 67)->data_ptr());
        std::cout << "TFTP File to Download: " << this->_files_to_download.front() << "\n";
        this->_state = DHCPAcknowledged;
        return true;
    }
    return false;
}

void PXEClient::arp_request_dhcp_server(Tins::PacketSender &sender) {
    EthernetII eth(HWAddress<6>::broadcast, this->_client_hw_address);
    auto *arp = new ARP(_dhcp_server_address, _dhcp_client_address, HWAddress<6>(), _client_hw_address);
    arp->opcode(ARP::REQUEST);
    eth.inner_pdu(arp);

    this->_state = ARPRequested;
    sender.send(eth);
}

void PXEClient::arp_reply_dhcp_server(Tins::PacketSender &sender, const Tins::HWAddress<6> &dhcp_server_mac_address) {
    this->_dhcp_hw_address = dhcp_server_mac_address;
    EthernetII eth(dhcp_server_mac_address, this->_client_hw_address);
    auto *arp = new ARP(_dhcp_server_address, _dhcp_client_address, dhcp_server_mac_address, _client_hw_address);
    arp->opcode(ARP::REPLY);
    eth.inner_pdu(arp);

    this->_state = ARPRecieved;
    sender.send(eth);
}


void PXEClient::ping(Tins::PacketSender &sender, const Tins::IPv4Address &ip_address,
                     const Tins::HWAddress<6> &mac_address) const {
    if (this->_state > DHCPAcknowledged) {
        EthernetII eth(mac_address, this->_client_hw_address);
        auto *ip = new IP(ip_address, this->_dhcp_client_address);
        ip->ttl(64);
        auto *icmp = new ICMP();
        srand(time(nullptr));
        icmp->id((uint16_t) rand());
        ip->inner_pdu(icmp);
        eth.inner_pdu(icmp);
        sender.send(eth);
    }
}

void PXEClient::pong(
        Tins::PacketSender &sender,
        const Tins::IPv4Address &ip_address,
        const Tins::HWAddress<6> &mac_address,
        const Tins::ICMP &icmp
) const {
    if (this->_state > DHCPAcknowledged) {
        EthernetII eth(mac_address, this->_client_hw_address);
        auto *ip = new IP(ip_address, this->_dhcp_client_address);
        ip->ttl(64);
        auto *new_icmp = icmp.clone();
        new_icmp->type(ICMP::ECHO_REPLY);
        ip->inner_pdu(new_icmp);
        eth.inner_pdu(ip);
        sender.send(eth);
    }
}

void PXEClient::tftp_read(Tins::PacketSender &sender) {
    EthernetII eth(this->_tftp_hw_address, this->_client_hw_address);
    auto *ip = new IP(this->_tftp_server_address, this->_dhcp_client_address);
    auto *udp = new UDP(69, 1024);
    auto *tftp = new TFTP();
    tftp->opcode(TFTP::READ_REQUEST);
    tftp->mode("octet");
    tftp->filename(_files_to_download.front());
    _files_to_download.pop();
    tftp->add_option({
                             "blksize",
                             "1432"
                     });
    tftp->add_option({
                             "tsize",
                             "0"
                     });
    udp->inner_pdu(tftp);
    ip->inner_pdu(udp);
    eth.inner_pdu(ip);
    sender.send(eth);
}

const IPv4Address &PXEClient::dhcp_client_address() const {
    return this->_dhcp_client_address;
}

const ClientState PXEClient::state() const {
    return _state;
}

void PXEClient::tftp_hw_address(const Tins::HWAddress<6> &mac_address) {
    _tftp_hw_address = mac_address;
}
