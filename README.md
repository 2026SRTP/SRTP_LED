# SRTP智能感应灯项目总装仓库

### 硬件选用：
- 总控：STM32F103C8T6
  - https://detail.tmall.com/item.htm?id=863604223100&mi_id=0000ZMT5hna8DhHnKwTmAYcOpRTrWa4pUUOu2JjihHxhtTo
- 旋转编码器：KY-040
  - https://detail.tmall.com/item.htm?id=821776919954&mi_id=0000gZgsm-nx3M-mHFwmc6QE-6BmhhqV-7sbRQ0l8-fMDoY
- 电源：电源适配器 + DC-DC转接模块（可输出12v/5v/3.3v）
  - https://detail.tmall.com/item.htm?id=867404375743&mi_id=000004FzzEeam1rxFbC7dvWZ3UYFVIOFI6ney2y3zN50fQk
- 场效应管：MOSFET
  - https://item.taobao.com/item.htm?id=836902261308&mi_id=0000wi1myFKNgTFqwjIAot9fEJTIzO_BVE61nehogWYC3FM
- LED灯条：12v灯条
  - https://item.taobao.com/item.htm?id=747644329421&mi_id=0000EgNx85LSJh7EU1hhozFyAGioy8HOVdWnOskD-klaOgg&sku_properties=122276201%3A10122
- 光敏电阻：LDR模块
  - https://e.tb.cn/h.RdH5vJoKmUwEpDR?tk=CRmw5u7choQ
- PIR：HC-SR501模块
  - https://e.tb.cn/h.RVeZWvKvHjrsj2I?tk=hoqW5wNxnyj
- 声音传感器：ky-037模块
  - https://e.tb.cn/h.RUBJbwjYmVPOiKL?tk=5iHU5x5JIib


### 引脚配置：
- ky-040：
  - VCC：3.3V
  - CLK：PA9
  - DT：PA8
  - SW：PB15
- Mosfet：
  - VIN+：12V
  - VIN-：GND
  - IO：PA0
- LDR：
  - VCC：3.3V
  - ADC：PA1
- PIR：
  - VCC：5V
  - OUT：PA2
- ky-037：
  - VCC：5V
  - DO：PA3
  
### 软件部分：
- ky040.c/h：完成
- mosfet.c/h：完成
- ldr.c/h：待完善
- pir.c/h：待完善
- sound.c/h：待测试