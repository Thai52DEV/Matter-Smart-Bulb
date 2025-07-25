## III. Chi tiết
### 1. Thread Border Router
#### 1.1. Một số lưu ý
- Phần cứng sử dụng là ESP Thread Border Router Board (**ESP TBR**) (gồm host SoC ESP32-S3 và RCP ESP32-H2)
   <img src="https://github.com/user-attachments/assets/e781b65e-b5c2-4371-bc9a-cd72a6b56dfc" alt="image" width="400"/>
- Phiên bản ESP-IDF được khuyên dùng là v5.2.4

   ```git clone -b v5.2.4 --recursive https://github.com/espressif/esp-idf.git```
- Repository dành cho ESP32-S3

   ```git clone --recursive https://github.com/espressif/esp-thread-br.git```
##
#### 1.2. Cấu hình các thành phần
- Cấu hình RCP: Sử dụng example có sẵn trong ```esp-idf/examples/openthread/ot_rcp```.
- Cấu hình Border Router
   - Sử dụng example có sẵn trong ```esp-thread-br/examples/basic_thread_border_router```.
   - Sử dụng lệnh ```idf.py menuconfig``` để cấu hình sâu:
      - Kích thước ...: 8MB.
      - SSID, Password cho Wifi mặc định.
      - OpenThread:
         - Điều chỉnh thông tin mạng Thread (Tên, PanID, Channel, Mật khẩu mạng Thread, ...).
         - Bật chế độ Commissioner, Joiner.
         - Bật chế độ tự khởi động Thread, giao diện Web Server.
         - Bật chế độ Command Line Interface (CLI) - Giao tiếp qua lệnh trên monitor.
- Build, flash. Border Router sẽ tự động tìm dến thư mục chứa file đã build của RCP để flash cùng.
##
#### 1.3. Kiểm tra hệ thống
##### 1.3.1. ESP TBR - Thread End Device (TED)
- Lấy Thread network dataset trên Border Router ```dataset active -x```.
- Trên monitor hiện
<pre>> dataset active -x
0e080000000000010000000300001335060004001fffe00208dead00beef00cafe0708fdfaeb6813db063b0510112233445566778899aabbccddeeff00030f4f70656e5468726561642d34396436010212340410104810e2315100afd6bc9215a6bfac530c0402a0f7f8
Done</pre>
- Thêm dataset trên cho TED
<pre>> dataset set active 0e080000000000010000000300001335060004001fffe00208dead00beef00cafe0708fdfaeb6813db063b0510112233445566778899aabbccddeeff00030f4f70656e5468726561642d34396436010212340410104810e2315100afd6bc9215a6bfac530c0402a0f7f8
Done
> dataset commit active
Done
> ifconfig up
Done
I (1665530) OPENTHREAD: netif up
> thread start
I(1667730) OPENTHREAD:[N] Mle-----------: Role disabled -> detached
Done
> I(1669240) OPENTHREAD:[N] Mle-----------: RLOC16 5800 -> fffe
I(1669590) OPENTHREAD:[N] Mle-----------: Attempt to attach - attempt 1, AnyPartition
I(1670590) OPENTHREAD:[N] Mle-----------: RLOC16 fffe -> 6c01
I(1670590) OPENTHREAD:[N] Mle-----------: Role detached -> child</pre>
- TED được phân quyền ```Role detached ->``` làm child hoặc router là thành công
##
##### 1.3.2. Linux Host - ESP TBR - TED
   <img src="https://github.com/user-attachments/assets/145c90dd-eceb-4c85-b9f6-5a2d29713d15" alt="image" width="600"/>

**Lưu ý:** Tuỳ thuộc vào Linux Host mà card wifi / ethernet sẽ có tên khác nhau. Trong ví dụ này là wpl5s0.
- Xác thực kết nối IPv6
   - Linux Host cần cấu hình để chấp nhận Router Advertisements (RA) của Border Router
   - Chấp nhận toàn bộ RA:
  
      ```sudo sysctl -w net.ipv6.conf.wlp5s0.accept_ra=2```
   - Chấp nhận toàn bộ các dộ dài của prefix trong RA:

      ```sudo sysctl -w net.ipv6.conf.wlp5s0.accept_ra_rt_info_max_plen=128```

   - Nhập lệnh ```ipaddr``` trên monitor của TED để lấy địa chỉ Global. Chẳng hạn
     <pre>> ipaddr
       fd66:afad:575f:1:744d:573e:6e60:188a
       fd87:8205:4651:27c8:0:ff:fe00:0
       fd87:8205:4651:27c8:e65a:3138:745a:df06
       fe80:0:0:0:2433:db2e:62c:b2e4
       Done</pre>
   - Nhập lệnh ```ping``` với địa chỉ Global của TED trên Linux Host
     <pre>
        ping fd66:afad:575f:1:744d:573e:6e60:188a
        PING fd66:afad:575f:1:744d:573e:6e60:188a(fd66:afad:575f:1:744d:573e:6e60:188a) 56 data bytes
        64 bytes from fd66:afad:575f:1:744d:573e:6e60:188a: icmp_seq=1 ttl=254 time=187 ms
        64 bytes from fd66:afad:575f:1:744d:573e:6e60:188a: icmp_seq=2 ttl=254 time=167 ms
     </pre> 
- Multicast forwarding Linux Host -> TED
   - Trên TED, tham gia vào nhóm multicast và tạo UDP socket
      <pre>
      > mcast join ff04::123
      Done
      > udp open
      Done
      > udp bind :: 5083
      Done</pre> 
   - Linux Host sẽ gửi gói tin qua TED bằng code ```multicast_udp_client.py``` trong thư mục ```Linux Host``` của repo này.
   - TED sẽ in ra như sau:
      <pre>5 bytes from fd11:1111:1122:2222:4a9e:272e:6a50:cf61 56024 hello</pre>
- Multicast forwarding TED -> Linux Host
   - Chạy ```multicast_udp_server.py``` trong thư mục ```Linux Host``` của repo này.
   - Build ```Thread End Device``` có trong repo này, sau đó flash vào TED.
   - Thực hiện các bước kết nối với mạng thread như phần **1.3.1. ESP TBR - Thread End Device (TED)**.
   - Dùng lệnh ```sendudp "tin nhan"``` để gửi lệnh cho Linux Host.
   - Trên Linux Host sẽ hiện
      <pre>
      $ python3 multicast_udp_server.py
      b'tin nhan' ('fd66:afad:575f:1:744d:573e:6e60:188a', 49154, 0, 0)</pre>
##
### 2. Home Assistant
#### 2.1. Tải [img](https://github.com/home-assistant/operating-system/releases/download/15.2/haos_ova-15.2.vmdk.zip) và cài đặt VMware Workstation 17
#### 2.2. Cấu hình máy ảo
- Create a New Virtual Machine.
- I will install the operating system later.
- Linux > Other Linux 5.x kernel 64-bit.
- Đặt tên máy chủ, chẳng hạn ```home-assistant```, and define an easy to reach storage location, such as ```C:\home-assistant```.
- Maximum disk size (GB): 32GB, Store virtual disk as a single file.

  <img src="https://github.com/user-attachments/assets/6090ccf6-a6e1-4260-8bcb-232af007a2bf" alt="image" width="200"/>
  <img src="https://github.com/user-attachments/assets/ce4eb669-822c-4b6e-86e5-82361d2e40c1" alt="image" width="200"/>
  <img src="https://github.com/user-attachments/assets/4581634b-903d-4946-8873-51617745453c" alt="image" width="200"/>
  <img src="https://github.com/user-attachments/assets/e9749e6d-a159-4802-82c4-b8c5ea2de02b" alt="image" width="200"/>

- Customize Hardware...
- 2 GB RAM,  2vCPU (minimum)
- Dùng cáp Ethernet (bắt buộc, không dùng wifi), tại **Network adapter**, chọn **Bridged: Connected directly to the physical network**.
- Close > Finish.

  <img src="https://drive.google.com/file/d/1DEpi6VZOBgf6smI4JiaYOwoGQ5LCu-7A/view?usp=drive_link" alt="image" height="350"/>
  <img src="https://github.com/user-attachments/assets/986fa573-8b77-49f2-9861-d8a3643bac87" alt="image" height="350"/>
  
##
#### 2.3. Khởi động máy ảo
- Boot thành công.

  <img src="https://github.com/user-attachments/assets/fc17a997-e939-413d-990d-cb245bbfd07b" alt="image" width="650"/>

- Dùng thiết bị cùng mạng LAN, truy cập đường dẫn http://homeassistant.local:8123 để cấu hình
  <img src="https://github.com/user-attachments/assets/fc7b1ad6-07b2-4c00-8676-e97553ca579d" alt="image" width="650"/>

- Create my smart home > Tạo tài khoản.

  <img src="https://github.com/user-attachments/assets/a33b09f3-35dd-4b0f-9e64-f5bd70432674" alt="image" height="300"/>  
  <img src="https://github.com/user-attachments/assets/2916916d-f902-4143-bb3e-7fa9942965d2" alt="image" height="300"/>

- Chọn vị trí > Next > Finish.

<img src="https://github.com/user-attachments/assets/be88eda3-6d16-47bd-ad08-283a149613f3" alt="image" height="300"/>
<img src="https://github.com/user-attachments/assets/d529893b-c70f-460e-9fe5-bca1bc7541fc" alt="image" height="300"/>
<img src="https://github.com/user-attachments/assets/30956ce6-1191-4691-8b7a-07baade76f59" alt="image" height="300"/  >

##
#### 2.4. Cấu hình Home Assistant
- Thêm Matter Server
  - Truy cập cửa hàng addon
  - Tìm Matter Server
  - Cài đặt

    <img src="https://github.com/user-attachments/assets/712dfeac-51be-4f98-8c24-83a60788a29b" alt="image" width="600"/>

- Thêm OpenThread Border Router
  - Chọn thêm dịch vụ > Tìm ```Open Thread``` > Chọn OpenThread Border Router.
  - Nhập REST API của Border Router, dạng ```http://x.x.x.x:80```, có thể thấy ở monitor của Border Router
    <pre>
      I (5211) obtr_web: <=======================server start========================>
      I (5211) obtr_web: http://192.168.3.112:80/index.html
      I (5211) obtr_web: <===========================================================>
    </pre>
    <img src="https://github.com/user-attachments/assets/65c6ccfe-622b-45a6-8bc5-dbae97741866" alt="image" width="250"/>
    <img src="https://github.com/user-attachments/assets/026faa7f-e149-408a-a259-aca3c3e60d22" alt="image" width="250"/>
    <img src="https://github.com/user-attachments/assets/a9ff3e53-80f2-4ee1-b28d-8be410512135" alt="image" width="250"/>

- Thêm Thread
  - Chọn thêm dịch vụ > Tìm ```Thread``` > Chọn Thread.
  - Cấu hình.
  - Hiển thị ra được Border Router là thành công.

    <img src="https://github.com/user-attachments/assets/adfb0516-6334-4b34-824a-d8e2e576750a" alt="image" width="200"/>
    <img src="https://github.com/user-attachments/assets/c4a7ba12-3f6c-4e38-94f0-f7ae4df88f07" alt="image" width="200"/>
    <img src="https://github.com/user-attachments/assets/10b8405f-6112-4fcf-b7e8-0752b7a4dc01" alt="image" width="200"/>
    <img src="https://github.com/user-attachments/assets/733066b9-c758-4428-b43c-8dd819cbdf64" alt="image" width="200"/>

- Đồng bộ thông tin Thread với điện thoại:
  - Quay về phần **cài đặt**.
  - Ứng dụng đồng hành > Xử lí sự cố > Đồng bộ thông tin đăng nhập Thread
  
    <img src="https://github.com/user-attachments/assets/16b19d23-c99b-405f-9859-3141886bde62" alt="image" width="200"/>
    <img src="https://github.com/user-attachments/assets/635fb945-2acb-4779-a6f5-8d39e24e387b" alt="image" width="200"/>
    <img src="https://github.com/user-attachments/assets/6b6e810a-3d21-4bf1-a14b-65ce59c49316" alt="image" width="200"/>
    <img src="https://github.com/user-attachments/assets/7e6663ac-79c0-4710-9d39-7c55ff21e1da" alt="image" width="200"/>

- Thêm thiết bị matter:
  - Vào **thiết bị** > Thêm thiết bị > Tìm ```Matter``` > Thêm thiết bị Matter.
  - Chọn ```Không, nó mới``` > Quét QR do thiết bị Matter cung cấp.
   
      <img src="https://github.com/user-attachments/assets/039134f9-0400-4409-be72-5bcef2d80b88" alt="image" width="200"/>
      <img src="https://github.com/user-attachments/assets/06249c25-def7-4754-b7c8-ab44c1a0f2f3" alt="image" width="200"/>
      <img src="https://github.com/user-attachments/assets/ec6993e7-e0b4-41ec-a67e-cc26338c82e6" alt="image" width="200"/>

  - Sau khi thêm xong thiết bị, sẽ xuất hiện phần điều khiển ở phần **Tổng quan**
##
### 3. ESP Launchpad
  *Sử dụng tool có sẵn của espressif tạo một thiết bị chuẩn matter cho testing, ví dụ này sử dụng ESP32C6*
- Truy cập tool [ESP Launchpad](https://espressif.github.io/esp-launchpad/?solution=matter).
- Kết nối ESP32C6 qua USB.
- Bấm chọn tab Connect > Chọn cổng ESP32C6 đang chiếm dụng (ở đây là COM19) > Connect.  
  <img src="https://github.com/user-attachments/assets/da0b87e7-b1c2-43ae-bfc1-9b770dfff11a" alt="image" width="650"/>

- Tại Quick Start:  
  - Select Application: thread_matter_light.  
  - ESP Chipset Type: ESP32C6.  
  - Nhấn Flash.  
    <img src="https://github.com/user-attachments/assets/de9b5bc1-dc43-4f82-baa1-799532e840d7" alt="image" width="650"/>

- Flash xong, web sẽ hiện thị console log của ESP32C6  
  <img src="https://github.com/user-attachments/assets/e8060158-6a0a-4e62-83d0-8d1f00398c32" alt="image" width="650"/>

- Chọn Reset > Confirm.  
  <img src="https://github.com/user-attachments/assets/cf2c2cc2-9dff-44e8-8b02-645dbfd5a015" alt="image" width="650"/>

- ESP32C6 đã sẵn sàng kết nối. Mở app quản lí nhà (Home Assistant,...) > quét QR thêm thiết bị này.  
  <img src="https://github.com/user-attachments/assets/202f1d95-9818-4dd5-814f-bc8d49fcddac" alt="image" width="650"/>
##
#### 4. ZeroTier  
*Sử dụng ZeroTier để điều khiển Home Assistant từ xa (khác mạng LAN)*

##### 4.1. Setup Network  
- Tạo tài khoản  
- Truy cập <a href="https://my.zerotier.com/" target="_blank">ZeroTier</a> > Tạo tài khoản  
- Create new network > Cấu hình network vừa tạo  
  <img src="https://github.com/user-attachments/assets/ddfdd154-4433-4f33-a25d-bd318b78c536" alt="image" width="650"/>

- Chỉnh sửa tên  
  <img src="https://github.com/user-attachments/assets/489f7119-29b1-4bb3-82e8-84610aa45f28" alt="image" width="650"/>

- Chỉnh sửa địa chỉ IPv4 mong muốn  
  <img src="https://github.com/user-attachments/assets/6a8e23b8-e058-474f-9a59-d744348e0c14" alt="image" width="650"/>

#### 4.2. Setup trên Home Assistant  
- Truy cập cửa hàng Add-on > Tìm ZeroTier One > Cài đặt.  
- Cấu hình > Thêm Network ID tạo ở bước **4.1** `52b337794f4b624b` vào networks > Lưu.  
- Khởi động lại ZeroTier One.  
  <img src="https://github.com/user-attachments/assets/c14d6d8c-59b9-4549-a8b4-b9a09040450a" alt="image" width="650"/>

#### 4.3. Setup trên Điện Thoại  
- Truy cập Google Play hoặc App Store > Tìm ZeroTier One > Cài đặt.  
- Add Network > Thêm Network ID tạo ở bước **4.1** `52b337794f4b624b` > Add > Bật lên.  
  <img src="https://github.com/user-attachments/assets/52537fcc-b46f-49aa-91ad-19300ab8a113" alt="image" width="300"/>
  <img src="https://github.com/user-attachments/assets/dc0165ed-dcc1-4dc3-a6bb-45a28ca0c05f" alt="image" width="300"/>

#### 4.4. Xác thực các member  
  <img src="https://github.com/user-attachments/assets/e36e869c-5c86-4888-b443-6f61d4f16b68" alt="image" width="650"/>
  <img src="https://github.com/user-attachments/assets/557cfc0a-1e43-4554-81be-b48261c8c4bb" alt="image" width="650"/>
  <img src="https://github.com/user-attachments/assets/f5bc8a44-9be5-44a8-90a6-da24d2ea6396" alt="image" width="650"/>

#### 4.5. Điều khiển Home Assistant từ xa bằng điện thoại  
- Truy cập vào IP của Home Assistant đã được thiết lập theo dạng ```x.x.x.x:8123```, chẳng hạn `172.23.155.40:8123`.  
  <img src="https://github.com/user-attachments/assets/0e1b0958-4233-449f-b447-eb471eab1334" alt="image" width="300"/>

- Đăng nhập, điều khiển như bình thường.


















  




  




    





  



 
