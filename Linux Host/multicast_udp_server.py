import socket
import struct

if_index = socket.if_nametoindex('wlp5s0')
sock = socket.socket(socket.AF_INET6, socket.SOCK_DGRAM)
sock.bind(('::', 5083))
sock.setsockopt(
        socket.IPPROTO_IPV6, socket.IPV6_JOIN_GROUP,
        struct.pack('16si', socket.inet_pton(socket.AF_INET6, 'ff04::123'),
                                if_index))
while True:
        data, sender = sock.recvfrom(1024)
        print(data, sender)
