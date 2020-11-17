# rainbowtype-bootloader

rainbowtype secure IoT prototyping system bootloader.  

# Requirements

  Platformio with VS Code environment.   
  install "Espressif 32" platform definition on Platformio   

# Environment reference
  
  Espressif ESP32-WROOM-32E or ESP32-WROVER-E.   
  this firmware supports 8MB flash size.   
  this project initializes both of I2C 0,1 port, and the device on I2C port 0 is absent.  
  pin assined as below:  


      I2C 0 SDA GPIO_NUM_18
      I2C 0 SCL GPIO_NUM_19

      I2C 1 SDA GPIO_NUM_21
      I2C 1 SCL GPIO_NUM_22
          
  Microchip ATECC608(on I2C port 1)   (not supported TRUST&GO and TRUSTFLEX)

# Usage

"git clone --recursive <this pages URL>" on your target directory.  
you need to change a serial port number which actually connected to ESP32 in platformio.ini.

# Run this project

First, on the host PC, run rainbowtype client to prepare   
the root certificate and signer certificate.  
Then create a device certificate template (cert_chain.c) to allow one firmware to   
create individual device certificates for multiple devices.  

On ESP32 side, add "src/cert_chain.c" above and execute "build" on platformio.   
This completes a binary that can be used as a master.  

When "Deploy rt Bootloader" is executed by rainbowtype client on the host PC side, a series of operations     
to write this firmware with code signing, flash encryption will be executed.  

For the second and subsequent ESP32s, simply copy the binary created above and   
execute "Deploy rt Bootloader" from the host PC again, and rainbowtype bootloader setup will proceed  
automatically.

# License

This software is released under the MIT License, see LICENSE.  
