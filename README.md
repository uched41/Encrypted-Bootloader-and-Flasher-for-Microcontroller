# Encrypted-Bootloader-and-Flasher-for-Microcontroller
Encrypted boot-loader and flasher for micro-controllers. Implements Double encryption to protect your proprietary firmware.

### Encrypter 
The encrypter application encrypts the bin file in blocks of 16 bytes, and produces an encrypted bin file. 

### Flasher
The flasher application decrypts the bin file, calculates the checksum and encrypts again with a new key before sending to the microcontroller. The flasher loads code to microcontroller through UART communication.

Both the encrypter and Flasher applications are only compatible with windows platform.

### Bootloader
The bootloader contains a sample of the bootloader for ARM Based NUVOTON microcontroller using UART.

### NOTE: This is a guideline for someone trying to write a bootloader for the first time, you will need to read and understand the code to successfully apply it.
