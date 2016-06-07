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
    _client_hw_address = HWAddress<6>(id);
    std::cout << "MAC: " << _client_hw_address << "\n";
    _state = New;
}

void PXEClient::dhcp_discover(Tins::PacketSender &sender) {
    if (_state == New) {
        srand(time(0));
        _dhcp_xid = (uint32_t) rand();
        EthernetII eth(HWAddress<6>::broadcast, _client_hw_address);
        auto *ip = new IP(IPv4Address::broadcast, IPv4Address());
        auto *udp = new UDP(67, 68);
        auto *dhcp = new DHCP();

        dhcp->xid(_dhcp_xid);
        dhcp->chaddr(HWAddress<6>(_client_hw_address));
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

        _state = DHCPDiscovering;
        sender.send(eth);
    }
}

void PXEClient::dhcp_request(Tins::PacketSender &sender, const Tins::IPv4Address &dhcp_server_address,
                             const Tins::IPv4Address &dhcp_client_address) {
    if (_state == DHCPDiscovering) {
        _dhcp_server_address = dhcp_server_address;
        _dhcp_client_address = dhcp_client_address;

        EthernetII eth(HWAddress<6>::broadcast, _client_hw_address);
        auto *ip = new IP(IPv4Address::broadcast, IPv4Address());
        auto *udp = new UDP(67, 68);
        auto *dhcp = new DHCP();

        dhcp->xid(_dhcp_xid);
        dhcp->chaddr(_client_hw_address);
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
                                 array_from_uint32(_dhcp_server_address).data()
                         });
        dhcp->add_option({
                                 DHCP::OptionTypes::DHCP_REQUESTED_ADDRESS,
                                 4,
                                 array_from_uint32(_dhcp_client_address).data()
                         });
        dhcp->end();

        udp->inner_pdu(dhcp);
        ip->inner_pdu(udp);
        eth.inner_pdu(ip);
        sender.send(eth);
        _state = DHCPRequested;
    }
}

const bool PXEClient::dhcp_acknowledged(const DHCP &pdu) {
    if (_state >= DHCPAcknowledged) return true;
    if (pdu.yiaddr() == _dhcp_client_address && pdu.chaddr() == _client_hw_address && _state == DHCPRequested) {
        _dhcp_server_address = (char *) pdu.search_option((DHCP::OptionTypes) 66)->data_ptr();
        std::cout << "TFTP Server Saved: " << _dhcp_server_address << "\n";
        _download_handler.add_download((char *) pdu.search_option((DHCP::OptionTypes) 67)->data_ptr());
        std::cout << "TFTP File to Download: " << (char *) pdu.search_option((DHCP::OptionTypes) 67)->data_ptr() <<
        "\n";
        _state = DHCPAcknowledged;
        return true;
    }
    return false;
}

void PXEClient::arp_request_dhcp_server(Tins::PacketSender &sender) {
    if (_state == DHCPAcknowledged) {
        EthernetII eth(HWAddress<6>::broadcast, _client_hw_address);
        auto *arp = new ARP(_dhcp_server_address, _dhcp_client_address, HWAddress<6>(), _client_hw_address);
        arp->opcode(ARP::REQUEST);
        eth.inner_pdu(arp);
        sender.send(eth);
        _state = ARPRequested;
    }
}

void PXEClient::arp_reply_dhcp_server(Tins::PacketSender &sender, const Tins::HWAddress<6> &dhcp_server_mac_address) {
    _dhcp_hw_address = dhcp_server_mac_address;
    EthernetII eth(dhcp_server_mac_address, _client_hw_address);
    auto *arp = new ARP(_dhcp_server_address, _dhcp_client_address, dhcp_server_mac_address, _client_hw_address);
    arp->opcode(ARP::REPLY);
    eth.inner_pdu(arp);
    sender.send(eth);
}


void PXEClient::ping(Tins::PacketSender &sender, const Tins::IPv4Address &ip_address,
                     const Tins::HWAddress<6> &mac_address) const {
    if (_state >= DHCPAcknowledged) {
        EthernetII eth(mac_address, _client_hw_address);
        auto *ip = new IP(ip_address, _dhcp_client_address);
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
    if (_state >= DHCPAcknowledged) {
        EthernetII eth(mac_address, _client_hw_address);
        auto *ip = new IP(ip_address, _dhcp_client_address);
        ip->ttl(64);
        auto *new_icmp = icmp.clone();
        new_icmp->type(ICMP::ECHO_REPLY);
        ip->inner_pdu(new_icmp);
        eth.inner_pdu(ip);
        sender.send(eth);
    }
}

void PXEClient::tftp_read(Tins::PacketSender &sender) {
    if (_state == ARPRecieved || _state == TFTPWaitingFilesRequest || _state == TFTPWaitingConfigRequest) {
        EthernetII eth(_dhcp_hw_address, _client_hw_address);
        auto *ip = new IP(_dhcp_server_address, _dhcp_client_address);
        auto *udp = new UDP(69, 1024);
        auto *tftp = new TFTP();
        tftp->opcode(TFTP::READ_REQUEST);
        tftp->mode("octet");
        tftp->filename(_download_handler.start_new_download());
        tftp->add_option({"blksize", "1432"});
        tftp->add_option({"tsize", "0"});
        udp->inner_pdu(tftp);
        ip->inner_pdu(udp);
        eth.inner_pdu(ip);
        sender.send(eth);

        if (_state == ARPRecieved) _state = TFTPBootRequest;
        else if (_state == TFTPWaitingConfigRequest) _state = TFTPConfigRequest;
        else _state = TFTPFilesRequest;

    }
}

void PXEClient::tftp_ack_options(Tins::PacketSender &sender, uint16_t dest_port, uint16_t block_size,
                                 uint32_t total_size) {
    if (_state == TFTPBootRequest || _state == TFTPConfigRequest || _state == TFTPFilesRequest) {
        EthernetII eth(_dhcp_hw_address, _client_hw_address);
        auto *ip = new IP(_dhcp_server_address, _dhcp_client_address);
        auto *udp = new UDP(dest_port, 1024);
        auto *tftp = new TFTP();
        _download_handler.set_current_download_sizes(block_size, total_size);
        tftp->opcode(TFTP::ACKNOWLEDGEMENT);
        tftp->block(0);
        udp->inner_pdu(tftp);
        ip->inner_pdu(udp);
        eth.inner_pdu(ip);
        sender.send(eth);

        if (_state == TFTPBootRequest) _state = TFTPBootDownloading;
        else if (_state == TFTPConfigRequest) _state = TFTPConfigDownloading;
        else _state = TFTPFilesDownloading;
    }
}

void PXEClient::tftp_ack_data(
        Tins::PacketSender &sender,
        uint16_t dest_port,
        const Data &data,
        uint16_t block_number
) {
    if (_state == TFTPFilesDownloading || _state == TFTPBootDownloading || _state == TFTPConfigDownloading) {
        EthernetII eth(_dhcp_hw_address, _client_hw_address);
        auto *ip = new IP(_dhcp_server_address, _dhcp_client_address);
        auto *udp = new UDP(dest_port, 1024);
        auto *tftp = new TFTP();
        _download_handler.append_data(data, block_number);
        tftp->opcode(TFTP::ACKNOWLEDGEMENT);
        tftp->block(block_number);
        udp->inner_pdu(tftp);
        ip->inner_pdu(udp);
        eth.inner_pdu(ip);
        sender.send(eth);

        if (_download_handler.current_download_complete()) {
            std::cout << "Download Finished.\n";
            _download_handler.finalize_current_download();
            if (_state == TFTPFilesDownloading && !_download_handler.complete()) _state = TFTPWaitingFilesRequest;
            else if (_state == TFTPBootDownloading) {
                _state = TFTPWaitingConfigRequest;
                const std::string pxelinuxcfg = "pxelinux.cfg/";
                std::string hw_str = _client_hw_address.to_string();
                for (char &c: hw_str) if (c == ':') c = '-';
                _download_handler.add_download(pxelinuxcfg + "01-" + hw_str); // pxelinux.cfg/01-xx-xx-xx-xx-xx-xx
                std::stringstream ip_ss;
                ip_ss << std::hex << std::uppercase << std::noshowbase
                << std::setw(8) << std::setfill('0') << Endian::do_change_endian(_dhcp_client_address);
                for (uint8_t specification = 8; specification > 0; specification--)
                    _download_handler.add_download(pxelinuxcfg + ip_ss.str().substr(0, specification));
                // pxelinux.cfg/xxxxxxxx to pxelinux.cfg/x
                _download_handler.add_download(pxelinuxcfg + "default");
                // pxelinux.cfg/default
            } else if (_state == TFTPConfigDownloading) _state = TFTPWaitingFilesRequest;
            else _state = Completed;

        }
    }
}

void PXEClient::tftp_not_found_error() {
    _download_handler.delete_current_download();
    _state = TFTPWaitingConfigRequest;
}

const IPv4Address &PXEClient::dhcp_client_address() const {
    return _dhcp_client_address;
}

ClientState PXEClient::state() const {
    return _state;
}

void PXEClient::arp_reply_recieved(const Tins::HWAddress<6> &tftp_hw_address) {
    _dhcp_hw_address = tftp_hw_address;
    _state = ARPRecieved;
}
