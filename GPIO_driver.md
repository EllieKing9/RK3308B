
GPIO
```
5 groups of GPIO bank(GPIO0 ~ GPIO4)
each group is divided by A0~A7, B0~B7, C0~C7, D0~D7 as the number. //A= 0, B= 1, C= 2, D= 3

pin = bank * 32 + number(= group * 8 + extra )
\_ GPIO0_B5 : 0 * 32 + ((1 * 8) + 5) = 13
```

RockChip RK3399(RK3308) GPIO Driver
```
pinctrl 파일에서 구현
  kernel/drivers/pinctrl/pinctrl-rockchip.c
  chip에 맞는
  GPIO Bank를 구성하고
  kernel에 등록(register)하는 역할
  
static struct platform_driver rockchip_pinctrl_driver = {
	.probe		= rockchip_pinctrl_probe,
	.driver = {
		.name	= "rockchip-pinctrl",
		.pm = &rockchip_pinctrl_dev_pm_ops,
		.of_match_table = rockchip_pinctrl_dt_match,
	},
};

static int __init rockchip_pinctrl_drv_register(void)
{
	return platform_driver_register(&rockchip_pinctrl_driver);
}
postcore_initcall(rockchip_pinctrl_drv_register);
```

gpio 제어

https://blog.naver.com/PostView.nhn?blogId=r2adne&logNo=220862931975

User mode
```
/*
user {
status = "disabled"; // add this line
label = "firefly:yellow:user";
linux,default-trigger = "ir-user-click";
default-state = "off";
gpios = <&gpio0 13 GPIO_ACTIVE_HIGH>;
pinctrl-names = "default";
pinctrl-0 = <&led_user>;
};
*/

$cd /sys/class/gpio

GPIO2_A6 : 2 * 32 + ((0 * 8) + 6 = 70
$echo 70 > /sys/class/gpio/export
$echo out > /sys/class/gpio/gpio70/direction
$echo 1 > /sys/class/gpio/gpio70/value

GPIO4_A6 : 4 * 32 + ((0 * 8) + 6 = 134
$echo 134 > /sys/class/gpio/export
$echo out > /sys/class/gpio/gpio70/direction
$echo 1 > /sys/class/gpio/gpio70/value

$cat /sys/kernel/gpio
```

Kernel Driver
```
kernel/drivers/gpio/gpio-firefly.c ??

==> device/rockchip/BoardConfig.mk 확인
# Kernel dts
export RK_KERNEL_DTS=rk3308b-roc-cc-plus-amic_emmc

kernel/arch/arm64/boot/dts/rockchip/rk3308b-roc-cc-plus-amic_emmc.dts
|_ #include "rk3308b-firefly.dtsi"
  |_ #include "rk3308k.dtsi"
     |_ #include "rk3308.dtsi"
        |_ #include <dt-bindings/gpio/gpio.h>
	   #include <dt-bindings/pinctrl/rockchip.h>

1. device tree script 에 구성 추가








```

Device Tree
```
시스템의 장치를 설명하는  노드가 있는 트리 구조의 데이터
부트로더 > DTS를 메모리에 로드 및 포인터를 커널에 전달 > CPU, Memory, Bus 및 주변 장치(Peripheral)를 관리
	프로빙으로 감지 할 수 없는 경우 PCI 호스트 브리지 장치를 설명한다. 그 외에는 PCI 장치를 설명하는 노드가 필요하지 않다.
노드의 이름은 일반적으로 정의된 리스트가 있다.
디바이스 정보를 여기서 찾음

Representation of the Device Tree contents : /sys/firmware/devicetree/base
If dtc is available on the target, possible to ”unpack” the Device Tree : $dtc -I fs /sys/firmware/devicetree/base

.dtsi(SoC level) files are included files, while .dts(Board level) files are final Device Trees
```

*.dts 내용

http://shukra.cedt.iisc.ernet.in/edwiki/Device_Tree_and_Boot_Flow
```
"/" 는 전체 장치의 최상위 루트 노드라는 의미

node_label: node_name@unit_address {
	a-string-property = "A string";
	a-string-list-property = "first string", "second string";
	a-byte-data-property = [0x01 0x23 0x34 0x56];
	
	a-reference-to-something = <&node_label>
	a-cell-property = < 1 2 3 4>;
	
	child-node_name@unit_address {
		an-empty-property;
	}
}

/ {
	compatible = "제조사,모델"; 
	compatible ="fsl,mpc8349-uart", "nsl16550" //첫번째 문자열 이외에는 형식이 자유롭다
	//"fsl,mpc8349-uart" 문자열을 인식할 수 있는 디바이스 드라이버를 찾을 것이고
	//만약 발견에 실패한다면 "ns16550"의 문자열을 인식할 수 있는 디바이스 드라이버를 찾는다.

	#address-cells = <2>; //32bit: <1>, 64bit: <2>인 경우도 있으며 2개의 주소를 사용(i2c)하는 경우도 있다.
	#size-cells = <1>;
	node {
		compatible = "식별자"; 
		child-node {
			reg = <0xD0000000 0x0000 1024 0xE0000000 0x0000 2048>;
			//#address-cells가 2이고 #size-cells가 1인 경우 
			//=> 0xD0000000 0x0000  : 시작 주소 2개, 1024  : 주소 범위 크기 | 0xE0000000 0x0000 : 시작 주소 2개 , 2048 : 주소 범위 크기
			//reg = <0x0C00 0x0 0xFFFF02 0x3333> 
			//#address-cells가 1이고 #size-cells도 1인 경우 
			//=> 0x0C00  : 시작 주소 , 0x0  : 주소 범위 크기 | 0xFFFF02 : 시작 주소 , 0x3333 : 주소 범위 크기 
		};
	};
};

- 속성 표현
	key = value // 값은 "문자열", <32bit 부호없는 정수형>, [2진 데이터]
- 노드 표현
	이름(31개의 문자 길이를 갖는 아스키 문자열)@장치주소(동일한 디바이스를 나타내거나 동일한 내용을 나타내는 노드가 없다면 생략)
- reg = <주소1 길이1 [주소2 길이2] [주소3 길이3] ... >
	"#address-cells" 속성과 "#size-cells" 은 부모 노드에서 지정하고  reg 속성 은 자식 노드에 지정

* 인터럽트 표현
	디바이스 노드간에 링크 구조로 표현(디바이스 노드의 속성의 형태)
	- interrupt-parent 속성 : 디폴트 인터럽트 컨트롤러를 지정, 트리 계층의 가장 상위에 정의하며 상속됨
	- interrupt-controller; 속성 : 해당 노드의 디바이스가 인터럽트 신호를 수신하는 인터럽트 컨트롤러 디바이스라는 것을 표현
	- #interrupt-cells 속성 : interrupt-controller 속성 선언시 같이 선언
	- interrupts : 디바이스가 발생하는 인터럽트 출력 신호에 대한 정보의 리스트를 값으로 표현


aliases {
	/external-bus/ethernet@0,0 = &eth0;
	..
}

All Device Tree bindings recognized by the kernel are documented in Documentation/devicetree/bindings
PWM
/mnt/blank/rk-3308-linux-oneunit-dispenser/kernel/Documentation/devicetree/bindings/pwm/pwm-gpio.txt 확인

$cd /proc/device-tree
$ls


```

pinctrl

https://docs.zephyrproject.org/3.1.0/hardware/pinctrl/index.html
```

```


