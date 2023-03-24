# Physical Eetlijst
Physical Eetlijst is a personal project to interface with the popular dutch student houses website [Eetlijst](https://eetlijst.nl/) and create a physical representation of the statuses of the website which shows who is there for dinner.

## Inspiration
This Physical Eetlijst was first inspired by others living on the University of Twente campus, all credits for the idea should be given to them.
The model presented here is a simplified version just to show the statuses and how many people are eating, it is not possible to sign up for dinner through this physical device.

## Hardware
The hardware (and prototype board layout) can be seen in the Eetlijst v5 folder, it is given both as a [Fritzing](Eetlijst_v5/Eetlijst.fzz) and [png](Eetlijst_v5/Eetlijst_Hardware%20Breadboard%20layout.png) and [jpg](Eetlijst_v5/Eetlijst_Hardware%20Breadboard%20layout.jpg).
Pay close attention to the connections as it otherwise may not work.
For the Wemos D1 mini, the MCU is flipped in the image (meaning 5V should be in the upper right corner, and 3.3V should be in the Upper Left.
The necessary items are the following:

**Item**   | **Count** | **Description**
------------- | ------------- | -------------
**Necessities** ||
Prototype board | 1 | This should be approximately (32 by 50) for 7 people, this should be altered to fit
Power connector breakout board | 1 | In the overview a USB micro is chosen, as that was what was available
Power switch | 1 | can be chosen as one sees fit
Wemos D1 mini (clone) | 1 | the brain of the operation is a ESP8266, in this case a clone Wemos D1 mini
TM1637 | 1 | A simple display to show how many people will be eating
Wires |N.A.| Some spare wires to connect everything
Headers |N.A.| Some spare headers to make everything swappable
**Indicators** ||
Blue LED | 1 | Indicator for when information is getting retrieved
RG LED | x | A bicolour led, in my case Red and Green to show the status of the people. Get as many as there are people in Eetlijst, in this example 7
Shift Register (74HC595) | x/4 | Shift register for driving LEDs, get the amount of bicolour LEDs divided by 4
Level Shifter | 1 | To convert signals from 3.3V ESP8266 to 5V shift registers
**OR** | | Instead of the above for indicators, there may be an easier option, this does require modifications for the hardware of the system.
Digital LED Strip (ws281x) | x + 1 | A digital LED strip may be easier than the above to indicate the people and when the system is retrieving information.

## Software
The Arduino IDE can be used to flash the software found in [Eetlijst_final_v5](Eetlijst_v5/Eetlijst_final_v5) to the Wemos D1 mini.
Before flashing, the different login credentials should be filled in [keys.h](Eetlijst_v5/Eetlijst_final_v5/keys.h).
In the place of the xxxxxxxx the Wi-Fi credentials, as well as the Eetlijst credentials should be filled in.

## Limitations
The current limitation of the system is that the ESP8266 does not work with enterprise networks, and will only work with simple home networks.
Furthermore, it is not possible to change your status from the device, this should still be done through the website or phone apps.

## Versions
There are two versions, the older one [Eetlijst_v4](Eetlijst_v4), is non-working, due to the updates given to Eetlijst itself.
The current version, [Eetlijst_v5](Eetlijst_v5), should be working and running.

## Support
This repository is for documentation purposes only, there will be no support for any issues. Questions are welcome, but it is not guaranteed to be fixed.

### License
[MIT](https://choosealicense.com/licenses/mit/)
