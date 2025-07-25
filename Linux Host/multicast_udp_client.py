#!/usr/bin/env python3
# save as test_multicast.py
import socket
import time

def send_udp_multicast():
    data = b'hello from host via multicast'
    group = 'ff04::123'
    port = 5083
    interface = 'wlp5s0'

    sock = socket.socket(socket.AF_INET6, socket.SOCK_DGRAM)
    
    try:
        # Bind to WiFi interface
        sock.setsockopt(socket.SOL_SOCKET, socket.SO_BINDTODEVICE, 
                       interface.encode())
        
        # Set multicast parameters
        sock.setsockopt(socket.IPPROTO_IPV6, socket.IPV6_MULTICAST_HOPS, 64)
        sock.setsockopt(socket.IPPROTO_IPV6, socket.IPV6_MULTICAST_IF, 
                       socket.if_nametoindex(interface))
        
        # Send UDP to multicast group
        sock.sendto(data, (group, port))
        print(f"Sent UDP multicast: {data} to {group}:{port}")
        
    except Exception as e:
        print(f"Error: {e}")
    finally:
        sock.close()

if __name__ == "__main__":
    send_udp_multicast()
