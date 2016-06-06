//
// Created by warnelso on 5/31/16.
//

#ifndef PXESIM_PXECLIENT_H
#define PXESIM_PXECLIENT_H

#include <tins/tins.h>
#include <queue>
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

    void dhcp_discover(Tins::PacketSender &sender);

    void dhcp_request(
            Tins::PacketSender &sender,
            const Tins::IPv4Address &dhcp_server_address,
            const Tins::IPv4Address &dhcp_client_address
    );

    const bool dhcp_acknowledged(
            const Tins::DHCP &pdu
    );

    void arp_request_dhcp_server(Tins::PacketSender &sender);

    void arp_reply_dhcp_server(
            Tins::PacketSender &sender,
            const Tins::HWAddress<6> &dhcp_server_mac_address
    );

    void ping(
            Tins::PacketSender &sender,
            const Tins::IPv4Address &ip_address,
            const Tins::HWAddress<6> &mac_address
    ) const;

    void pong(
            Tins::PacketSender &sender,
            const Tins::IPv4Address &ip_address,
            const Tins::HWAddress<6> &mac_address,
            const Tins::ICMP &icmp
    ) const;


    void tftp_read(Tins::PacketSender &sender);

    const Tins::IPv4Address &dhcp_client_address() const;

    const ClientState state() const;

    void tftp_hw_address(const Tins::HWAddress<6> &mac_address);

private:
    Tins::HWAddress<6> _client_hw_address, _dhcp_hw_address, _tftp_hw_address;

    Tins::IPv4Address _dhcp_client_address, _dhcp_server_address, _tftp_server_address;
    uint32_t _dhcp_xid;

    uint16_t _tftp_source_port, _tftp_dest_port;

    std::queue<std::string> _files_to_download;
    ClientState _state;

};

uint8_t *array_from_uint32(uint32_t number);


#endif //PXESIM_PXECLIENT_H
