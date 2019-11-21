# Azure Sphere Cifar-10 NN demo - HLcore

This project is the partner of [azure-sphere-combo-cifar10-rtcore](https://github.com/xiongyu0523/azure-sphere-combo-cifar10-rtcore). The HL app capture image from [ArduCAM mini 2MP PLUS](https://www.arducam.com/docs/spi-cameras-for-arduino/hardware/arducam-shield-mini-2mp-plus/) and preview on 2.8 TFT [ER-TFTM028-4](https://www.buydisplay.com/default/2-8-inch-tft-touch-shield-for-arduino-w-capacitive-touch-screen-module). It then resizes the image to 32x32x3 tensor and send to RT app for NN inference. A result will be displayed on the screen. 

## Note

1. The demo support capturing 320x240 bitmap or 160x120 jpeg as source. Under JPEG mode, the best performance is 9fps. 

2. The RT app must be download before this HL app.

## To build and run the sample

### Prep your device

1. Ensure that your Azure Sphere device is connected to your PC, and your PC is connected to the internet.
2. Even if you've performed this set up previously, ensure that you have Azure Sphere SDK version 19.10. In an Azure Sphere Developer Command Prompt, run **azsphere show-version** to check. Download and install the [latest SDK](https://aka.ms/AzureSphereSDKDownload) as needed.
3. Right-click the Azure Sphere Developer Command Prompt shortcut and issue the following command:

   ```
   azsphere dev edv
   ```

4. ArduCam connection:
   
    |  ArduCAM mini 2MP Plus | RDB  | MT3620 |
    |  ----  | ----  | ---- | 
    | SCL  | H2-7 | GPIO27_MOSI0_RTS0_CLK0 |
    | SDA  | H2-1 | GPIO28_MISO0_RXD0_DATA0 |
    | VCC  | H3-3 | 3V3 | 
    | GND  | H3-2 | GND |
    | SCK  | H4-7 | GPIO31_SCLK1_TXD1 |
    | MISO  | H4-5 | GPIO33_MISO1_RXD1_DATA1 |
    | MOSI  | H4-11 | GPIO32_MOSI1_RTS1_CLK1 |
    | CS  | H1-10 | GPIO3 |

5. TFT connection:

    |  ER-TFTM028-4 Pin | Name | RDB  | MT3620 |
    |  ----  | ----  | ---- | ---- | 
    | 1 | GND  | H4-2 | GND |
    | 2 | VCC |  J3-3 | 3V3 (**Jumper on 1-2 with CR2032 battery**) |
    | 21 | RST  | H1-4 | GPIO0 |
    | 23 | CS | H3-13 | GPIO69_CSA3_CTS3 |
    | 24 | SCK | H3-5 | GPIO66_SCLK3_TXD3 |
    | 25 | DC | H1-6 | GPIO1 |
    | 27 | SDI | H3-7 | GPIO67_MOSI3_RTS3_CLK3 |
    | 28 | SDO | H3-9 | GPIO68_MISO3_RXD3_DATA3 |
    | 29 | BL | H1-8 | GPIO2 |

### Build and deploy the application

1. Start Visual Studio.
2. From the **File** menu, select **Open > CMake...** and navigate to the folder that contains the sample.
3. Select the file CMakeLists.txt and then click **Open**. 
4. In Solution Explorer, right-click the CMakeLists.txt file, and select **Generate Cache for azure-sphere-combo-cifar10-hlcore**. This step performs the cmake build process to generate the native ninja build files. 
5. In Solution Explorer, right-click the *CMakeLists.txt* file, and select **Build** to build the project and generate .imagepackage target.
6. Double click *CMakeLists.txt* file and press F5 to start the application with debugging. 
7. You will start to see image from camera preview and object name recognized on TFT.
