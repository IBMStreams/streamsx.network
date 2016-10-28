# DNSPacketDPDKSource operator


## introduction 

DNSPacketDPDKSource is an operator for the IBM Streams product that
captures live network packets from one of the network interfaces attached to the
machine where it executes. The operator function and structure are
similar to the PacketLiveSource and DNSMessageParser operators; 
see the documentation for those for details
of common functions and general background.  
The primary difference between
these related operators is that DNSPacketDPDKSource leverages 
the [http://www.dpdk.org|'data plane development kit (DPDK)]
for higher performance.

Output filters and attribute assignments are SPL expressions. They may use any
of the built-in SPL functions, and any of these functions, which are specific to
the PacketFileSource operator:

* [tk$com.ibm.streamsx.network/fc$com.ibm.streamsx.network.dns.html|network header parser result functions]

The DNSPacketDPDKSource operator steps quietly over 'jmirror' headers prepended to packets
by Juniper Networks 'mirror encapsulation'.

This operator is part of the network toolkit. To use it in an
application, include this statement in the SPL source file:

    use com.ibm.streamsx.network.dns::*;


## promiscuous mode 

Network interfaces normally ignore packets that are not addressed to them.
However, when 'promiscious' mode is enabled on a network interface, it can
capture all network packets on its ethernet segment, even those that are not
addressed to it.  This is sometimes referred to as "network sniffing".  Modern
ethernet switches normally send network interfaces only packets that are
addressed to them; 'promiscious' mode is useful only when a switch has been
specifically configured to send packets to a network interface that are not
addressed to it. This is sometimes referred to as "mirroring".

The DNSPacketDPDKSource operator will enable 'promiscuous' mode in a ethernet
interface when its 'promiscous' parameter is set to `true`.


## dependencies

DPDK supports a variety of [http://www.dpdk.org/doc/nics|ethernet adapters]. 
The DNSPacketDPDKSource operator has been tested with these adapters:

* Mellanox ConnectX-3
* Mellanox ConnectX-3 Pro
* Intel e1000
* Intel I350

The [http://dpdk.org|DPDK libraries] must be compiled from source code and installed on the machine where
the network adapter is installed. Streams processing elements (PEs) must be placed
on this machine.


## notes on installing and configuring DPDK

The [http://dpdk.org/doc/guides/linux_gsg/index.html|DPDK user guide] has detailed instructions for
installing DPDK and configuring it for particular ethernet adapters. These notes are a summary of those instructions.
They include many Linux commands that should be entered at a command prompt in a Terminal window.
In the instructions below, these commands are prefixed with `bash> ...`.
Many of the commands require 'root' privileges; they are prefixed with `bash> sudo ...`.


### determine your machine architecture

IBM Streams is supported on two types of machine architectures, 'x86 64-bit' and 'Power8 64-bit (little-endian)'.
To compile and run the DPDK libraries, you will need to know which architecture your machine uses.

To determine your machine's architecture, open a Terminal window on the machine and type this at the command prompt:

    bash> uname -m

The 'uname' command will display one of the following architecture types:

* `x86_64` _... for x86 64-bit ..._
* `ppc64le` _... for Power8 64-bit (little-endian) ..._ 

If the 'uname' command displays some other architecture type, the IBM Streams product is not supported.


### identify your ethernet adapter and its Linux ethernet interface

The DNSPacketDPDKSource operator is supported on several types of ethernet adapters.
To compile the DPDK libraries, you will need to know which ethernet adapters your machine has,
and which one you want to use with DPDK. 
To use that adapter with DPDK, you will need to know which Linux network interface is assigned to it.

First, display a list of ethernet adapters installed in your machine by typing this at a command prompt:

    bash> lspci | grep ethernet
    0002:01:00.0 Ethernet controller: Mellanox Technologies MT27520 Family [ConnectX-3 Pro]
    0003:05:00.0 Ethernet controller: Broadcom Corporation NetXtreme BCM5719 Gigabit Ethernet PCIe (rev 01)
    0003:05:00.1 Ethernet controller: Broadcom Corporation NetXtreme BCM5719 Gigabit Ethernet PCIe (rev 01)
    0003:05:00.2 Ethernet controller: Broadcom Corporation NetXtreme BCM5719 Gigabit Ethernet PCIe (rev 01)
    0003:05:00.3 Ethernet controller: Broadcom Corporation NetXtreme BCM5719 Gigabit Ethernet PCIe (rev 01)
    0005:01:00.0 Ethernet controller: Mellanox Technologies MT27520 Family [ConnectX-3 Pro]

If none of the supported ethernet adapters listed above is displayed, the DNSPacketDPDKSource operator is not supported.

If one of the supported ethernet adapters is listed, note the numbers preceding the word "Ethernet".
This is the PCI bus address of the adapter.

Then, display a list of Linux network interfaces, and the ethernet adapters they are assigned to, by typing this at a command prompt:

    bash> ls -la /sys/class/net
    lrwxrwxrwx.  1 root root 0 Sep 28 09:49 enP2p1s0 -> ../../devices/pci0002:00/0002:00:00.0/0002:01:00.0/net/enP2p1s0
    lrwxrwxrwx.  1 root root 0 Sep 28 09:49 enP2p1s0d1 -> ../../devices/pci0002:00/0002:00:00.0/0002:01:00.0/net/enP2p1s0d1
    lrwxrwxrwx.  1 root root 0 Sep 28 09:49 enP3p5s0f0 -> ../../devices/pci0003:00/0003:00:00.0/0003:01:00.0/0003:02:09.0/0003:05:00.0/net/enP3p5s0f0
    lrwxrwxrwx.  1 root root 0 Sep 28 09:49 enP3p5s0f1 -> ../../devices/pci0003:00/0003:00:00.0/0003:01:00.0/0003:02:09.0/0003:05:00.1/net/enP3p5s0f1
    lrwxrwxrwx.  1 root root 0 Sep 28 09:49 enP3p5s0f2 -> ../../devices/pci0003:00/0003:00:00.0/0003:01:00.0/0003:02:09.0/0003:05:00.2/net/enP3p5s0f2
    lrwxrwxrwx.  1 root root 0 Sep 28 09:49 enP3p5s0f3 -> ../../devices/pci0003:00/0003:00:00.0/0003:01:00.0/0003:02:09.0/0003:05:00.3/net/enP3p5s0f3
    lrwxrwxrwx.  1 root root 0 Sep 28 09:49 enP5p1s0 -> ../../devices/pci0005:00/0005:00:00.0/0005:01:00.0/net/enP5p1s0
    lrwxrwxrwx.  1 root root 0 Sep 28 09:49 enP5p1s0d1 -> ../../devices/pci0005:00/0005:00:00.0/0005:01:00.0/net/enP5p1s0d1
    lrwxrwxrwx.  1 root root 0 Sep 28 09:49 lo -> ../../devices/virtual/net/lo

Find the Linux network interface of the ethernet adapter by matching its PCI bus address to those listed, preding the string "/net/".


### create a user group for DPDK

DPDK will lock the memory pages containing its buffers to prevent the kernel from swapping them out to disk while it is using them.
Users who run the DNSPacketDPDKSource operator will need permission to do this.

To grant this permission, create a user group for DPDK and add the user account[s] that will run the DNSPacketDPDKSource operator to the group.

For example, to create a user group 'dpdk' and add your own user account to it, open a Terminal window and type these commands at the prompt:

    bash> sudo groupadd dpdk
    bash> sudo usermod -a -G dpdk $USER

To verify that your user account is in the 'dpdk' group, type this at a command prompt:

    bash> groups $USER
    streamsdpdk : streamsdpdk dpdk

To grant permission to lock memory pages, edit the '/etc/security/limits.conf' system file:

    bash> sudo vi /etc/security/limits.conf

 and insert this line into the file:

    @dpdk - memlock unlimited

After making these changes, log off your user account and log back in to put the changes into effect.


### configure Linux 'hugepages' 

DPDK uses [https://www.kernel.org/doc/Documentation/vm/hugetlbpage.txt|Linux kernel 'hugepages' support] 
to allocate large buffers with minimal 'translation lookaside buffer (TLB)' overhead.
The size and number of 'hugepages' available for DPDK must be configured in the Linux kernel. 
This requires 'root' access, and may require rebooting the machine.

Note that the memory allocated for hugepages will reduce the memory available for Streams and other applications.

To display the amount of memory installed in your machine, and the amount of memory available, type this at a a command prompt:

    bash> cat /proc/meminfo | grep -i mem
    MemTotal:       263636096 kB
    MemFree:        226797632 kB
    MemAvailable:   249287680 kB
    Shmem:            131776 kB

To display the size and number of hugepages currently allocated, type this at a command prompt:

    bash> cat /proc/meminfo | grep -i hugepages
    AnonHugePages:         0 kB
    HugePages_Total:      10
    HugePages_Free:        0
    HugePages_Rsvd:        0
    HugePages_Surp:        0
    Hugepagesize:      16384 kB

The supported sizes of hugepages depend upon the machine architecture. 
Typical sizes for the architectures supported by IBM Streams include: 

* 4KB, 8KB, 64KB, 256KB, 1MB, 4MB, 16MB, or 256MB _... for 'x86_64' (x86_64 machines) ..._
* 4KB, 16MB, or 16GB _... for 'ppc64le' (Power8 little-endian machines) ..._

The size of hugepages on your machine is set by Linux kernel parameters when the machine is booted.
If you want to change the size of hugepages on your machine, please refer to these documents for details:

* https://www.kernel.org/doc/Documentation/vm/hugetlbpage.txt
* https://www.kernel.org/doc/Documentation/kernel-parameters.txt

The number of hugepages is determined by the 'vm.nr_hugepages' system control parameter.
This parameter can be set when the system is booted, and it can be changed later while the system is running.

For example, to set the 'vm.nr_hugepages' parameter to 10 when the system boots, edit the '/etc/sysctl.conf' system file and set the parameter like this:

    vm.nr_hugepages=10

To increase the 'vm.nr_hugepages' parameter to 1000 while the system is running, type this at a command prompt:

    bash> sudo sysctl -w vm.nr_hugepages=1000

To verify that the parameter has changed, type this at a command prompt:

    bash> sysctl vm.nr_hugepages
    vm.nr_hugepages = 1000

To verify that the number of hugepages has actually increased, type this at a command prompt:

    bash> cat /proc/meminfo | grep -i hugepages
    AnonHugePages:         0 kB
    HugePages_Total:    1000
    HugePages_Free:        0
    HugePages_Rsvd:        0
    HugePages_Surp:        0
    Hugepagesize:      16384 kB

If the number of hugepages has not actually been increased to 1000, memory may be too fragemented.
In this case, you will need to reboot the machine and then increase the number of hugepages.

To decrease the 'vm.nr_hugepages' parameter to 100 while the system is running, type this at a command prompt:

    sudo sysctl -w vm.nr_hugepages=100

To verify that the number of hugepages has actually increased, type this at a command prompt:

    bash> cat /proc/meminfo | grep -i hugepages
    AnonHugePages:         0 kB
    HugePages_Total:     100
    HugePages_Free:        0
    HugePages_Rsvd:        0
    HugePages_Surp:        0
    Hugepagesize:      16384 kB

To make the hugepages accessible to the user accounts that will run the DNSPacketDPDKSource operator,
create and mount a directory for them by typing these commands at a prompt:

    bash> sudo mkdir /dev/hugepages
    bash> sudo mount -t hugetlbfs nodev /dev/hugepages -o gid=dpdk,mode=1770

To ensure the hugepages are mounted in the '/dev/hugepages' directory when the machine is rebooted, 
edit the '/etc/fstab' system file by typing this command at a prompt:

    bash> sudo vi /etc/fstab

and inserting this line into the file:

    nodev  /dev/hugepages  hugetlbfs  gid=dpdk,mode=1770  0  0


### prepare your Linux user account for DPDK

You will need some environment variables when you compile the DPDK libraries, 
and whenever you compile Streams applications that use the DNSPacketDPDKSource operator.

To set these variables permanently, edit the '$HOME/.bashrc' script by typing this at a command prompt:

    vi $HOME/.bashrc

and insert these lines into the file: 

    export RTE_SDK=$HOME/dpdk-2.2.0
    [[ $( uname -m ) == "x86_64" ]] && export RTE_TARGET=x86_64-native-linuxapp-gcc
    [[ $( uname -m ) == "ppc64le" ]] && export RTE_TARGET=ppc_64-power8-linuxapp-gcc

After making this change, log off your user account and log back in to put the change into effect.


### compile the DPDK libraries

DPDK must be compiled from source code using the Gnu C/C++ compiler. 
This requires libraries for GCC and the specific version of the Linux kernel your machine uses.
To install these libraries, type these commands at a prompt:

    bash> sudo yum install glibc-devel
    bash> sudo yum install libibverbs-devel
    bash> sudo yum install "kernel-devel-$(uname -r)"

The DPDK source code needs to be downloaded,
configured for your machine's architecture and ethernet adapter,
and compiled into libraries and utilities.

First, download the DPDK source code and configure it for your machine's architecture
by typing these commands at a prompt:

    bash> wget http://fast.dpdk.org/rel/dpdk-2.2.0.tar.xz
    bash> tar -xvf dpdk-2.2.0.tar.xz
    bash> cd $RTE_SDK
    bash> make config T=$RTE_TARGET

Then, edit the DPDK configuration file by typing this command:

    bash> vi ./build/.config

and change this parameter in the file:

    CONFIG_RTE_BUILD_COMBINE_LIBS=y

and, if your machine has a Mellanox ethernet adapter, change this parameter in the file, too:

    CONFIG_RTE_LIBRTE_MLX4_PMD=y 

Then, compile DPDK by typing this command at a prompt:

    bash> EXTRA_CFLAGS=-fPIC make                
    bash> EXTRA_CFLAGS=-fPIC make install T=$RTE_TARGET


### compile Streams DPDK glue library

If you don't already have the Streams network toolkit, 
get it by typing these commands at a prompt:

    bash> cd $HOME/git
    bash> git clone git@github.com:ejpring/streamsx.network.git
    bash> cd $HOME/git/streamsx.network
    bash> ant

Compile the Streams DPDK glue library by typing these commands at a prompt:

    bash> cd $HOME/git/streamsx.network/com.ibm.streamsx.network/impl/src/source/dpdk
    bash> make





