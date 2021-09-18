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
 - pacman -S qemu virt-manager qemu-guest-agent libguestfs
 - systemctl enable libvirtd
 - s usermod -a -G libvirt <username>
 - ip link add name free_br type bridge
 - ip link set free_br up
 - ip address add dev free_br 192.168.17.5/24
 - ip link set eno1 master free_br

If needed:
 - ip link set eno1 nomaster
 - ip link delete free_br type bridge


FreeBSD
-------

 - Download qcow2 image
 - qemu-img resize FreeBSD-13.0-RELEASE-amd64.qcow2 +2GB
 - create virtual machine


Inside the virtual machine
--------------------------

 - gpart show ada0
 - gpart resize -i 4 -s 6000M ada0
 - growfs /dev/ada0p4
   - if that fails:   service growfs onestart
 - pkg (to initialize)
 - pkg update
 - pkg install sudo git curl vim rsync bash pkgconf
 - adduser

Getting the project
-------------------

git clone https://github.com/tobbik/lua-t.git
