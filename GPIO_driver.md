
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
