# Azure Sphere Cifar-10 NN demo - RTcore

This project is the partner of [azure-sphere-combo-cifar10-hlcore](https://github.com/xiongyu0523/azure-sphere-combo-cifar10-hlcore). The RT app waits for image data from HL app, then run cifar-10 CMSIS-NN inference to recognize an ojbect and report to HL app. The model and code is extract from [ML-examples](https://github.com/ARM-software/ML-examples)

## Note

1. The Cortex-M4F core is running at 26MHz out of reset, the example boost core frequency to 197.6MHz to get high performance
   
   ```
   uint32_t val = ReadReg32(IO_CM4_RGU, 0);
   val &= 0xFFFF00FF;
   val |= 0x00000200;
   WriteReg32(IO_CM4_RGU, 0, val);
   ```
   
2. To get full performance of CM4F core, need modify the gcc compiler flag to use FPU instructions. Just copy the *AzureSphereRTCoreToolchainVFP.cmake* file into the Azure Sphere SDK install folder. (Default path is *C:\Program Files (x86)\Microsoft Azure Sphere SDK\CMakeFiles*)

    The underlayer work is to replace line 45:

    `SET(CMAKE_C_FLAGS_INIT "-mcpu=cortex-m4")`

    with

    `SET(CMAKE_C_FLAGS_INIT "-mthumb -mcpu=cortex-m4 -mfloat-abi=hard -mfpu=fpv4-sp-d16")`

3. To compile an optimized CMSIS-NN library, `ARM_MATH_DSP` and `__FPU_PRESENT=1U` need to be set. (already in CMakeLists.txt)

4. This project must be download before running HL app. Otherwise HL app can't open the socket.

## To build and run the sample

### Prep your device

1. Ensure that your Azure Sphere device is connected to your PC, and your PC is connected to the internet.
2. Even if you've performed this set up previously, ensure that you have Azure Sphere SDK version 19.10. In an Azure Sphere Developer Command Prompt, run **azsphere show-version** to check. Download and install the [latest SDK](https://aka.ms/AzureSphereSDKDownload) as needed.
3. Right-click the Azure Sphere Developer Command Prompt shortcut and select **More > Run as administrator**.
4. At the command prompt issue the following command:

   ```
   azsphere dev edv --enablertcoredebugging
   ```

   This command must be run as administrator when you enable real-time core debugging because it installs USB drivers for the debugger.
5. Close the window after the command completes because administrator privilege is no longer required.  

### Build and deploy the application

1. Start Visual Studio.
2. From the **File** menu, select **Open > CMake...** and navigate to the folder that contains the sample.
3. Select '**ARM-Release**' as project configuration
4. Select the file CMakeLists.txt and then click **Open**. 
5. In Solution Explorer, right-click the CMakeLists.txt file, and select **Generate Cache for azure-sphere-combo-cifar10-rtcore**. This step performs the cmake build process to generate the native ninja build files. 
6. In Solution Explorer, right-click the *CMakeLists.txt* file, and select **Build** to build the project and generate .imagepackage target.
7. Double click *CMakeLists.txt* file and press F5 to start the application with debugging. 
8. The demo will print message "**Exmaple to run NN inference on RTcore for cifar-10 dataset**" after boot via IO0_TXD (Header3-6) and then wait for data from HL app.
