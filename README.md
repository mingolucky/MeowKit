### **MeowKit — Versatile Device for Makers**

​This is a **pocket-sized multi-tool** device for AI Chat & MCP Control, hacking protocol exploration, DIY hardware expansion, desktop gadget integration, and more — powered by a fully **open-source SDK** . **Built for makers and hackers**, it's designed to be a powerful yet easy-to-use development **platform**.

![本地图片](2.assets/header.png)

### 收到包裹

当您第一次收到MeowKit团队全员动手寄出的包裹,请先查看有哪些东西以及注意事项.

#### 物品清单

1.  MeowKit
2.  Lanyard
3.  Micro sd card Lexar 32GB
4.  Reader
5.  USB Type-C cable
6.  Leather USB cable tie
7.  Postcard
8.  Bread board line

#### 可能问题&解决

## a. 黑灰粉尘
- 黑灰粉尘为正常现象，无害。
  - 避免触碰纸板边缘。
  - 若手上沾灰，请先清洗再操作主机。

## b. 物流与运输
- 运输过程中可能导致电量耗尽或后壳松动。
  - 使用 USB-C 充电，直至红灯熄灭表示充满电。
  - 使用 T5 螺丝刀拧紧后盖。

## c. A/B 按键手感
- 新机按键初期略紧。
  - 经过多次按压和摩擦后，按下和回弹动作会逐渐顺畅，手感得到优化。


### 快速上手

#### 更新固件

a.下载[MeowKit](https://github.com/happy-mingo/MeowKit)->code->download zip,将固件文件保存至本地电脑.

b.浏览器访问烧录网址：https://espressif.github.io/esp-launchpad/

​c.将MeowKit连接usb到电脑，点击Connect选择 “USB JTAG/serial debug unit（COM xx）”

​d.点击DIY开发板COM端口被浏览器连接后，网页会显示 Connected to device: ESP32-S3 字样，同时，菜单 Connect 会变为 Disconnect ，表示开发板COM端口连接成功。

e.selected file and flash address,添加下面3个文件. 点击program.

​		`bootloader.bin` → **0x0**

​		`partitions.bin` → **0x8000**

​		`	firmware.bin` → **0x10000**

f.开始烧录后，网页会自动跳转到 Console 控制台命令行 菜单页面，显示烧录进程，如下图示：当在控制台黑屏上看到写入完成100%并提示离开Leaving...提示信息之后，点击右侧的重启设备按钮，重新启动开发板，就可以看到程序烧录成功了。

​g.固件烧录完成后，设备不会自动重启，需要动手重启设备，这里请点击网页上的 Reset Device 按钮（下图位置1），并再次确认 点击 Comfirm 按钮（下图位置2），确认重启开发板，以启动运行刚才烧录的固件程序。

## 基本使用说明

1. 长按开机键3秒
2. 进入bsp界面，并提示按压A/B按键（显示“bsp init done”）
3. MeowKit进入操作界面
4. 进入apps菜单页面，单击icon打开应用
5. 长按B键退出应用，退回apps菜单页面

#### 图形界面指南


## 固件架构


## 硬件规格


## 一起加入

At the very beginning of the project, we have chosen the tactics of maximum openness and transparency. We believe in open-source, the power of the community, and that enthusiasts can create cool projects without the involvement of large corporations. Therefore, we invite everyone interested in the project to take part in the development!




