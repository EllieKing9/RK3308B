# ROC-RK3308B-CC-PLUS
development board
  
* 펌웨어 업데이트

https://wiki.t-firefly.com/en/Core-3308Y/burning_firmware.html (Burning Firmware manual)

https://en.t-firefly.com/doc/download/84.html#other_296 (Driver Assistant & Android Tool download)

https://en.t-firefly.com/doc/download/page/id/67.html#other_294

```
윈도우즈(Windows)에서 준비
1. 드라이버 어시스턴트(Driver Assistant) 다운로드 및 설치(DriverInstall.exe)
2. Connect to the device
  > 개발 보드에 전원이 연결 안된 상태에서
    [RECOVERY] 버튼을 누르고 USB를 연결하여 2초후에 뗀다
  > Windows _ 검색(Search) _ 장치 관리자(Device Manager)에서 
    "Rockusb Device"가 보인다면 제대로 연결이 된 것
3. 안드로이드 툴(Android Tool) 다운로드 및 실행(AndroidTool.exe)
  > 한글 이름이 없는 경로에 복사하여 사용
4. 펌웨어 다운로드(ROC-3308B-CC-PLUS-Ubuntu18-Minimal-20220106.img)
5. AndroidTool.exe _ [Firmware] 탭(Tab) 클릭 _ 펌웨어 선택
  _ [Upgrade] 클릭
```
  
* 시리얼 디버깅 (Serial Debugging)

https://wiki.t-firefly.com/en/Core-3308Y/serial_debug.html (Serial Debug manual)

https://www.silabs.com/developers/usb-to-uart-bridge-vcp-drivers (CP2104 driver download)

https://www.chiark.greenend.org.uk/~sgtatham/putty/latest.html (PuTTY download)

```
윈도우즈에서 준비
1. USB to serial port 드라이버 다운로드 및 설치(CP210xVCPInstaller_x64.exe)
2. 개발보드랑 USB-to-UART serial port converter 쪽보드랑 연결
  > GND, TXD, RXD 확인
3. Windows _ 검색(Search) _ 장치 관리자(Device Manager)에서
   USB Serial Port(COM숫자) 확인
4. PuTTY 다운로드 및 설치
5. PuTTY 실행 및 세팅
  > Connection type : Serial 로 선택
  > Serial line에 [COM숫자] 세팅  
  > Baud rate(Speed): 1500000 세팅
  
  > Data bits: 8
  > Stop bit: 1
  > Parity: None
  > Flow control: None
5. 연결
  > 터미널 창에 아무것도 안뜨면..
    Baud rate 재확인 및 쪽보드랑 개발보드 연결 라인 변경(TXD, RXD 바꾸어서)
```
