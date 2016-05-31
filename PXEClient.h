//
// Created by warnelso on 5/31/16.
//

#ifndef PXESIM_PXECLIENT_H
#define PXESIM_PXECLIENT_H

#include <tins/tins.h>
#include <list>
#include <string>

enum ClientState {
    New,
    DHCPDiscovering,
    DHCPRequested,
    DHCPAcknowledged,
    ARPRequested,
    ARPRecieved,
    TFTPRequested,
    TFTPAcknowledged,
    Finished,
    Terminated = -1
};

class PXEClient {
public:

    PXEClient();

    Tins::EthernetII create_dhcp_discover();

    Tins::EthernetII create_dhcp_request(
            Tins::IPv4Address dhcp_server_address,
            Tins::IPv4Address dhcp_client_address
    );

    Tins::EthernetII create_arp_request_to_dhcp_server();

    Tins::EthernetII create_arp_reply_to_dhcp_server(Tins::HWAddress<6> dhcp_server_mac_address);

    bool dhcp_acknowledged(Tins::DHCP &pdu);

private:
    Tins::HWAddress<6> _mac_address;
    Tins::IPv4Address _dhcp_client_address, _dhcp_server_address;
    uint32_t _dhcp_xid;

    Tins::IPv4Address _tftp_server_address;
    uint16_t _tftp_source_port, _tftp_dest_port;

    std::list<std::string> _files_to_download;
    ClientState _state;
};

uint8_t *array_from_uint32(uint32_t number);


#endif //PXESIM_PXECLIENT_H
