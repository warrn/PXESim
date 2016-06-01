//
// Created by warnelso on 5/31/16.
//

#include "PXEClient.h"

using namespace Tins;

uint8_t *array_from_uint32(uint32_t number) {
    uint8_t *array = new uint8_t[4];
    array[3] = (uint8_t) (number >> 24);
    array[2] = (uint8_t) (number >> 16);
    array[1] = (uint8_t) (number >> 8);
    array[0] = (uint8_t) (number);
    return array; //TODO: Memory leak
}


PXEClient::PXEClient() {
    srand(time(0));
    uint8_t id[] = {0x00, 0x0B, 0xCD, (uint8_t) rand(), (uint8_t) rand(), (uint8_t) rand()};
    // Simulating HP Servers
    this->_mac_address = HWAddress<6>(id);
    std::cout << "MAC: " << this->_mac_address << "\n";
    this->_state = New;
}

const EthernetII PXEClient::create_dhcp_discover() {
    srand(time(0));
    this->_dhcp_xid = (uint32_t) rand();
    EthernetII eth("ff:ff:ff:ff:ff:ff", this->_mac_address);
    auto *ip = new IP("255.255.255.255", "0.0.0.0");
    auto *udp = new UDP(67, 68);
    auto *dhcp = new DHCP();

    dhcp->xid(this->_dhcp_xid);
    dhcp->chaddr(HWAddress<6>(this->_mac_address));
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
    return eth;
}

const EthernetII PXEClient::create_dhcp_request(const IPv4Address &dhcp_server_address,
                                                const IPv4Address &dhcp_client_address) {
    this->_dhcp_server_address = dhcp_server_address;
    this->_dhcp_client_address = dhcp_client_address;

    EthernetII eth("ff:ff:ff:ff:ff:ff", this->_mac_address);
    auto *ip = new IP("255.255.255.255", "0.0.0.0");
    auto *udp = new UDP(67, 68);
    auto *dhcp = new DHCP();

    dhcp->xid(this->_dhcp_xid);
    dhcp->chaddr(this->_mac_address);
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
                             array_from_uint32(this->_dhcp_server_address)
                     });
    dhcp->add_option({
                             DHCP::OptionTypes::DHCP_REQUESTED_ADDRESS,
                             4,
                             array_from_uint32(this->_dhcp_client_address)
                     });
    dhcp->end();

    udp->inner_pdu(dhcp);
    ip->inner_pdu(udp);
    eth.inner_pdu(ip);

    this->_state = DHCPRequested;
    return eth;
}

const bool PXEClient::dhcp_acknowledged(const DHCP &pdu) {
    if (this->_state >= DHCPAcknowledged) return true;
    if (pdu.yiaddr() == this->_dhcp_client_address && pdu.chaddr() == this->_mac_address) {
        this->_tftp_server_address = (char *) pdu.search_option((DHCP::OptionTypes) 66)->data_ptr();
        std::cout << "TFTP Server Saved: " << this->_tftp_server_address << "\n";
        this->_files_to_download.push_back((char *) pdu.search_option((DHCP::OptionTypes) 67)->data_ptr());
        std::cout << "TFTP File to Download: " << *(this->_files_to_download.begin()) << "\n";
        this->_state = DHCPAcknowledged;

        return true;
    }
    return false;
}

const EthernetII PXEClient::create_arp_request_to_dhcp_server() {
    EthernetII eth("ff:ff:ff:ff:ff:ff", this->_mac_address);
    auto *arp = new ARP(_dhcp_server_address, _dhcp_client_address, "00:00:00:00:00:00", _mac_address);
    arp->opcode(ARP::REQUEST);
    eth.inner_pdu(arp);

    this->_state = ARPRequested;
    return eth;
}

const EthernetII PXEClient::create_arp_reply_to_dhcp_server(const HWAddress<6> &dhcp_server_mac_address) {
    EthernetII eth(dhcp_server_mac_address, this->_mac_address);
    auto *arp = new ARP(_dhcp_server_address, _dhcp_client_address, dhcp_server_mac_address, _mac_address);
    arp->opcode(ARP::REPLY);
    eth.inner_pdu(arp);

    this->_state = ARPRecieved;
    return eth;
}


const Tins::EthernetII PXEClient::create_ping(const Tins::IPv4Address &ip_address,
                                              const Tins::HWAddress<6> &mac_address) const {
    if (this->_state > DHCPAcknowledged) {
        EthernetII eth(mac_address, this->_mac_address);
        auto *ip = new IP(ip_address, this->_dhcp_client_address);
        ip->ttl(64);
        auto *icmp = new ICMP();
        srand(time(0));
        icmp->id((uint16_t) rand());
        ip->inner_pdu(icmp);
        eth.inner_pdu(icmp);
        return eth;
    } else return EthernetII();
}

const Tins::EthernetII PXEClient::create_pong(const Tins::IPv4Address &ip_address,
                                              const Tins::HWAddress<6> &mac_address, const Tins::ICMP &icmp) const {
    if (this->_state > DHCPAcknowledged) {
        EthernetII eth(mac_address, this->_mac_address);
        auto *ip = new IP(ip_address, this->_dhcp_client_address);
        ip->ttl(64);
        auto *new_icmp = icmp.clone();
        new_icmp->type(ICMP::ECHO_REPLY);
        ip->inner_pdu(new_icmp);
        eth.inner_pdu(ip);
        return eth;
    } else return EthernetII();
}

const IPv4Address &PXEClient::get_dhcp_client_address() {
    return this->_dhcp_client_address;
}

const ClientState PXEClient::get_state() const {
    return _state;
}