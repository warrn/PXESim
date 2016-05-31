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
    return array;
}


PXEClient::PXEClient() {
    srand(time(0));
    uint8_t id[] = {0x00, 0x0B, 0xCD, (uint8_t) rand(), (uint8_t) rand(), (uint8_t) rand()};
    // Simulating HP Servers
    this->_mac_address = HWAddress<6>(id);
    std::cout << "MAC: " << this->_mac_address << "\n";
    this->_state = New;
}

EthernetII PXEClient::create_dhcp_discover() {
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

EthernetII PXEClient::create_dhcp_request(IPv4Address dhcp_server_address, IPv4Address dhcp_client_address) {
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

bool PXEClient::dhcp_acknowledged(DHCP &pdu) {
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

EthernetII PXEClient::create_arp_request_to_dhcp_server() {
    EthernetII eth("ff:ff:ff:ff:ff:ff", this->_mac_address);
    auto *arp = new ARP(_dhcp_server_address, _dhcp_client_address, "00:00:00:00:00:00", _mac_address);
    arp->opcode(ARP::REQUEST);
    eth.inner_pdu(arp);

    this->_state = ARPRequested;
    return eth;
}

EthernetII PXEClient::create_arp_reply_to_dhcp_server(HWAddress<6> dhcp_server_mac_address) {
    EthernetII eth("ff:ff:ff:ff:ff:ff", this->_mac_address);
    auto *arp = new ARP(_dhcp_server_address, _dhcp_client_address, _mac_address);
    arp->opcode(ARP::REPLY);
    eth.inner_pdu(arp);

    this->_state = ARPRecieved;
    return eth;
}
