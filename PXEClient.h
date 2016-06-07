//
// Created by warnelso on 5/31/16.
//

#ifndef PXESIM_PXECLIENT_H
#define PXESIM_PXECLIENT_H

#include <tins/tins.h>
#include <queue>
#include <string>
#include "DownloadHandler.h"

enum ClientState {
    New,
    DHCPDiscovering,
    DHCPRequested,
    DHCPAcknowledged,
    ARPRequested,
    ARPRecieved,
    TFTPBootRequest,
    TFTPBootDownloading,
    TFTPWaitingConfigRequest,
    TFTPConfigRequest,
    TFTPConfigDownloading,
    TFTPWaitingFilesRequest,
    TFTPFilesRequest,
    TFTPFilesDownloading,
    Completed,
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

    void tftp_ack_options(
            Tins::PacketSender &sender,
            uint16_t dest_port,
            uint16_t block_size,
            uint32_t total_size
    );

    void tftp_ack_data(
            Tins::PacketSender &sender,
            uint16_t dest_port,
            const Data &data,
            uint16_t block_number
    );

    void tftp_not_found_error();

    const Tins::IPv4Address &dhcp_client_address() const;

    ClientState state() const;

    void arp_reply_recieved(const Tins::HWAddress<6> &tftp_hw_address);

    const Tins::HWAddress<6> &get_client_hw_address() const { return _client_hw_address; }

private:
    Tins::HWAddress<6> _client_hw_address, _dhcp_hw_address;

    Tins::IPv4Address _dhcp_client_address, _dhcp_server_address;
    uint32_t _dhcp_xid;

    DownloadHandler _download_handler;
    ClientState _state;

};

const std::vector<uint8_t> array_from_uint32(const uint32_t number);


#endif //PXESIM_PXECLIENT_H
