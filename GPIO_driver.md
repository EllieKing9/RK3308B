
GPIO
```
5 groups of GPIO bank(GPIO0 ~ GPIO4)
each group is divided by A0~A7, B0~B7, C0~C7, D0~D7 as the number. //A= 0, B= 1, C= 2, D= 3

pin = bank * 32 + number(= group * 8 + extra )
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

GPIO 드라이버 작성
```
kernel/drivers/gpio/gpio-firefly.c

```
