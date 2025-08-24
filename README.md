## **MeowKit — Versatile Device for Makers**

![本地图片](2.assets/header.png)

​This is a **pocket-sized multi-tool** device for AI Chat & MCP Control, hacking protocol exploration, DIY hardware expansion, desktop gadget integration, **open-source SDK** and more . **Built for makers and hackers**, it's designed to be a powerful yet easy-to-use development **platform**.

![本地图片](2.assets/apps.png)

### 收到包裹

当您第一次收到MeowKit团队全员动手寄出的包裹,请先查看有哪些东西以及注意事项.

#### 📦 物品清单

| #  | Item                    | 描述/备注            |
|----|--------------------------|----------------------|
| 1  | **MeowKit**              |                      |
| 2  | **Lanyard**              |                      |
| 3  | **Micro SD Card**        | Lexar 32GB           |
| 4  | **Card Reader**          |                      |
| 5  | **USB Type-C Cable**     |                      |
| 6  | **Leather USB Cable Tie**|                      |
| 7  | **Postcard**             |                      |
| 8  | **Breadboard Jumper Wires** |                   |


#### 可能问题&解决

##### a. 黑色粉末
- 黑色粉末为激光切割蜂窝纸板边缘烧结产品，自然无害。
  - 避免触碰纸板边缘。
  - 若手上沾灰，请先清洗再操作主机。

##### b. 物流与运输
- 运输过程中可能导致电量耗尽或后壳松动。
  -  USB-C 线充电，直至红灯熄灭表示充满电。
  -  T5 螺丝刀拧紧后盖。

##### c. A/B 按键手感
- 新机按键初期略紧。
  - 经过多次按压和摩擦后，按下和回弹动作会逐渐顺畅，手感得到优化。


### 快速上手

#### 更新固件

1. **下载固件**
   - 访问 [MeowKit GitHub](https://github.com/happy-mingo/MeowKit) → 点击 **Code** → **Download ZIP**，将固件文件保存到本地电脑。

2. **打开烧录网页**
   - 在浏览器中访问：[ESP Launchpad](https://espressif.github.io/esp-launchpad/)

3. **连接设备**
   - 将 MeowKit 通过 USB 连接到电脑。
   - 点击 **Connect**，选择 `USB JTAG/serial debug unit (COM xx)`。

4. **确认连接**
   - 连接成功后，网页会显示：
     ```
     Connected to device: ESP32-S3
     ```
   - 此时菜单 **Connect** 会变为 **Disconnect**，表示 COM 端口连接成功。

5. **选择固件文件并设置烧录地址**

    Hacker Protocol & Desktop Gadgets 固件
    - 点击 **Selected file and flash address**，添加以下文件：
       - `bootloader.bin` → `0x0`
       - `partitions.bin` → `0x8000`
       - `meowkit.bin` → `0x10000`
    - 点击 **Program** 开始烧录。
    
    AI Chat & MCP 固件
    - 点击 **Selected file and flash address**，添加以下文件：
       - `bootloader.bin` → `0x0`
       - `partition-table.bin` → `0x8000`
       - `ota_data_initial.bin` → `0xd000`
       - `srmodels.bin` → `0x10000`
       - `xiaozhi.bin` → `0x100000`
    - 点击 **Program** 开始烧录。


7. **查看烧录进程**
   - 烧录过程中，网页会自动跳转到 **Console** 控制台，显示烧录进度。
   - 当控制台显示：
     ```
     写入完成 100% 
     Leaving...
     ```
     提示信息后，表示烧录完成。

8. **重启设备**
   - 固件烧录完成后，设备不会自动重启。
   - 请点击网页上的 **Reset Device** 按钮，然后点击 **Confirm** 按钮，确认重启开发板以启动刚烧录的固件程序。


#### 上手操作

![本地图片](2.assets/get_start.png)

1. **开机**：长按开机键 3 秒。  
2. **初始化 BSP**：进入 BSP 界面，屏幕会提示按压 A/B 按键（显示 “bsp init done”）。  
3. **进入操作界面**：MeowKit 自动进入主操作界面。  
4. **打开应用**：进入 **Apps 菜单** 页面，单击图标打开应用。  
5. **退出应用**：长按 **B 键**，退出应用并返回 Apps 菜单页面。


#### 界面指南

#####The graphical interface includes a clock, home, app menu, SD‑card file manager, and control center. With SD‑card file management (littleFS) and the Mooncake multi‑app management and scheduling framework for microcontrollers (by Forairaaaaa), you can easily build and add your own applications.

![本地图片](2.assets/ui.png)


#### 硬件规格


#### 加入我们

At the very beginning of the project, we have chosen the tactics of maximum openness and transparency. We believe in open-source, the power of the community, and that enthusiasts can create cool projects without the involvement of large corporations. Therefore, we invite everyone interested in the project to take part in the development!


## **MeowKit — Versatile Device for Makers**

![Header](2.assets/header.png)

This is a **pocket-sized multi-tool** device for AI Chat & MCP Control, protocol hacking exploration, DIY hardware expansion, desktop gadget integration, **open-source SDK**, and more.  
**Built for makers and hackers**, it is designed to be a powerful yet easy-to-use development **platform**.

![Apps](2.assets/apps.png)

### Unboxing

When you first receive the package carefully prepared and shipped by the MeowKit team, please check the contents and read the important notes.

#### 📦 Package Contents

| #  | Item                      | Description/Notes    |
|----|---------------------------|----------------------|
| 1  | **MeowKit**               | Main device          |
| 2  | **Lanyard**               | Wrist/neck strap     |
| 3  | **Micro SD Card**         | Lexar 32GB           |
| 4  | **Card Reader**           |                      |
| 5  | **USB Type-C Cable**      |                      |
| 6  | **Leather USB Cable Tie** |                      |
| 7  | **Postcard**              |                      |
| 8  | **Breadboard Jumper Wires** | For prototyping    |

#### Possible Issues & Solutions

##### a. Black powder
- The black powder comes from laser-cut honeycomb cardboard edges; it is harmless.  
  - Avoid direct contact with the cardboard edges.  
  - If your hands get dusty, wash them before handling the device.  

##### b. Shipping and handling
- During transport, the battery may be discharged or the back cover may become loose.  
  - Recharge using the USB-C cable until the red LED turns off (indicating full charge).  
  - Tighten the back cover screws with a T5 screwdriver if needed.  

##### c. A/B button stiffness
- The A/B buttons may feel slightly stiff at first.  
  - With repeated use, pressing and rebound will become smoother and the tactile feel will improve.  

---

### Getting Started

#### Firmware Update

1. **Download the firmware**  
   - Visit [MeowKit GitHub](https://github.com/happy-mingo/MeowKit) → click **Code** → **Download ZIP**, and save the firmware files to your computer.  

2. **Open the flashing tool**  
   - In your browser, go to: [ESP Launchpad](https://espressif.github.io/esp-launchpad/)  

3. **Connect the device**  
   - Connect MeowKit to your PC via USB.  
   - Click **Connect**, then select `USB JTAG/serial debug unit (COM xx)`.  

4. **Confirm connection**  
   - Once connected, the webpage will display:  
     ```
     Connected to device: ESP32-S3
     ```
   - The **Connect** menu will switch to **Disconnect**, confirming a successful COM connection.  

5. **Select firmware files and set flash addresses**

   **Hacker Protocol & Desktop Gadgets firmware**  
   - Click **Selected file and flash address**, then add:  
     - `bootloader.bin` → `0x0`  
     - `partitions.bin` → `0x8000`  
     - `meowkit.bin` → `0x10000`  
   - Click **Program** to start flashing.  

   **AI Chat & MCP firmware**  
   - Click **Selected file and flash address**, then add:  
     - `bootloader.bin` → `0x0`  
     - `partition-table.bin` → `0x8000`  
     - `ota_data_initial.bin` → `0xd000`  
     - `srmodels.bin` → `0x10000`  
     - `xiaozhi.bin` → `0x100000`  
   - Click **Program** to start flashing.  

7. **Monitor flashing progress**  
   - During flashing, the browser will switch to the **Console** view and display progress.  
   - Once you see:  
     ```
     Writing complete 100% 
     Leaving...
     ```
     the flashing is successfully finished.  

8. **Restart the device**  
   - The device does not reboot automatically after flashing.  
   - Click the **Reset Device** button on the webpage, then click **Confirm** to reboot and run the newly flashed firmware.  

---

#### Basic Operation

![Getting Started](2.assets/get_start.png)

1. **Power on**: Press and hold the power button for 3 seconds.  
2. **Initialize BSP**: On first boot, follow the on-screen prompt to press the A/B buttons (display shows “bsp init done”).  
3. **Enter main interface**: MeowKit will automatically boot into the main interface.  
4. **Open an app**: Go to the **Apps Menu**, tap an icon to launch an app.  
5. **Exit an app**: Long-press the **B button** to exit and return to the Apps Menu.  

---

#### Interface Guide

The graphical interface includes a clock, home screen, app menu, SD-card file manager, and control center.  
With SD-card file management (littleFS) and the Mooncake multi-app management and scheduling framework for microcontrollers (by Forairaaaaa), you can easily build and add your own applications.

![UI](2.assets/ui.png)

---

#### Hardware Specifications
*(To be added)*

---

### Join Us

From the very beginning of this project, we have embraced **openness and transparency** as our core values.  
We believe in open-source, the strength of the community, and the idea that enthusiasts can create amazing projects without the backing of large corporations.  

We warmly invite anyone interested to **contribute, collaborate, and help build MeowKit together!** 🚀

















