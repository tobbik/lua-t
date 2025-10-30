What up
=======

to develop more platform independent, running lua-t on FreeBSD allows to tes
the the network and the asynchronous loop functionality.


What is needed from Linux
=========================

Basics
------

Steps to take on Archlinux:

 - Enable virtualization in the BIOS if not done yet.
 - pacman -S qemu virt-manager qemu-guest-agent libguestfs virt-viewer
 - systemctl enable libvirtd
 - s usermod -a -G libvirt <username>

Untested:
 - ip link add name free_br type bridge
 - ip link set free_br up
 - ip address add dev free_br 192.168.17.5/24
 - ip link set eno1 master free_br

If needed:
 - ip link set eno1 nomaster
 - ip link delete free_br type bridge


FreeBSD
-------

 - Download qcow2.xz image
 - export VMS_FOLDER=/mnt/ext001/vms
 - export IMG_FOLDER=/mnt/ext001/vm_images
 - export IMG_FOLDER=/home/tobias/Downloads
 - export BSD_VERSION=14.0
 - export IMG_NAME=FreeBSD-${BSD_VERSION}-RELEASE-amd64.qcow2
 - ``xzcat ${IMG_FOLDER}/FreeBSD-${BSD_VERSION}*.xz > "${VMS_FOLDER}/${IMG_NAME}"``
 - ``qemu-img resize "${VMS_FOLDER}/${IMG_NAME}" +6G``
 - ``virt-install --name freebsd${BSD_VERSION} --memory 4096 --vcpus 2 \
      --disk ${VMS_FOLDER}/${IMG_NAME},bus=sata \
      --import --os-variant freebsd13.1 --network default``

Running VM
 - virsh start freebsd13.2
 - virsh shutdown freebsd13.2
 - virsh undefine freebsd13.2
Looking in running VM
 - virt-viewer --connect qemu:///session --wait freebsd13.2


Inside FreeBSD
--------------

 - first login as `root`  -> no password
 - gpart show ada0
 - gpart resize -i 4 -s 6000M ada0 (if necessary)
 - growfs /dev/ada0p4
   - if that fails:   service growfs onestart
 - enable sshd -> ``echo 'sshd_enable="YES"' >> /etc/rc.conf``
 - enable sshd -> ``echo 'syslog_enable="NO"' >> /etc/rc.conf``
 - enable sshd -> ``echo 'newsyslog_enable="NO"' >> /etc/rc.conf``
 - ``sed -i .bak -e '/^0.*newsyslog/ s/^0/#0/' /etc/crontab``
 - pkg (to initialize)
 - pkg update && pkg upgrade
 - pkg install sudo git curl vim rsync bash pkgconf tig lua54
 - pkg clean -a
 - adduser -s /usr/local/bin/bash dev
 - adduser -f dev:::::::/home/dev:/usr/local/bin/bash:dev
 - echo 'dev ALL=(ALL) NOPASSWD: ALL' > /usr/local/etc/sudoers.d/dev

ArchLinux
---------

 - Download qcow2 image https://mirror.pkgbuild.com/images/
 - export VMS_FOLDER=/mnt/ext001/vms
 - export IMG_FOLDER=/mnt/ext001/vm_images
 - export IMG_NAME=Arch-Linux-x86_64-basic.qcow2
 - cp ${IMG_FOLDER}/${IMG_NAME} ${VMS_FOLDER}/${IMG_NAME}
 - ``qemu-img resize "${VMS_FOLDER}/{IMG_NAME}" +6G``
 - ``virt-install --name archlinux --memory 4096 --vcpus 2 \
     --disk ${VMS_FOLDER}/${IMG_NAME},bus=sata --import \
     --os-variant archlinux --network default``

Inside Archlinux
----------------

 - first login as `arch:arch`
 - sudo su
 - systemctl enable sshd
 - passwd
 - reboot   (into root)

rm /etc/resolv.conf
ln -s /run/systemd/resolve/resolv.conf  /etc/resolv.conf

for IFCFILE in /etc/systemd/network/\*.network; do
  echo -e "[DHCPv4]\n UseDomains=true\n\n[IPv6AcceptRA]\nUseDomains=yes"
done

# systemd logging becomes volatile
``sed -i 's:^#\(Storage\)=.*:\1=volatile:' /etc/systemd/journald.conf``
``sed -i 's:^#\(LogFile *\).*:\1 = /root/pacman.log:' /etc/pacman.conf``


 - usermod -d /home/dev -m arch
 - usermod -l dev arch
 - passwd dev
 - mv /etc/sudoers.d/arch /etc/sudoers.d/dev
 - sed -i 's/arch/dev/' /etc/sudoers.d/dev
 - ``sudo pacman -S which sudo git curl vim rsync bash pkgconf tig lua gcc gdb rxvt-unicode-terminfo make clang lldb net-tools``
 - vim -p /etc/resolve.conf /etc/nsswitch.conf

Getting the project
-------------------

git clone https://github.com/tobbik/lua-t.git

Network to the virtual machine
------------------------------

Using a bridge for this is over the top.  So a simple NAT should do. Two
options:

 1. setup portforwarding on the fly for an already ruinning virtual machine.
    Use virsh:
   ``virsh qemu-monitor-command --hmp freebsd14.0 'hostfwd_add ::55555-:22'``
   ``virsh qemu-monitor-command --hmp archlinux   'hostfwd_add ::55556-:22'``
 2. edit the virsh edit freebsd13.1

.. code:: xml
   <domain type='kvm' xmlns:qemu='http://libvirt.org/schemas/domain/qemu/1.0'>
   ...
     </devices>
     <qemu:commandline>
       <qemu:arg value='-netdev'/>
       <qemu:arg value='user,id=mynet.0,net=10.0.10.0/24,hostfwd=tcp::55555-:22,hostfwd=tcp::8000-:8000'/>
       <qemu:arg value='-device'/>
       <qemu:arg value='e1000,netdev=mynet.0'/>
     </qemu:commandline>
   </domain>



