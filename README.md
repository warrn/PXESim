# PXESim
#### A (to be) multithreaded, multiclient simulator for PXE boot

Built with [libtins](http://libtins.github.io) which is based on [libpcap](https://sourceforge.net/projects/libpcap/).

---
**What is PXE?**

**PXE** or **P**reboot E**X**ecution **E**nvironment is a standardized preboot environment option currently available on
the majority of bare metal servers. This mode allows the hardware to contact the network and secure itself the required
data to boot without the necessity for a physical hard drive or human intervention.

PXE is based on the following processes:
 1. A DHCP Transaction with an available DHCP Server on the local network.
 2. TFTP Transaction(s) with an available TFTP Server that contains the required boot images and configuration.

---
**Why make PXESim**

In order to test PXEServers, it becomes necessary to test clients. However, to massively test to capability of a server
to handle many clients, the overhead to replicate the clients is incredibly costly. The objective of PXESim is to simulate
the transactions involved on a massive scale with as little overhead as possible.
