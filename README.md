# Mazda RX-8 Head Unit Controller

## About The Project

This project is designed to control the motorized hood on the Mazda RX-8 SatNav screen as well as provide auxillary (aux) input and custom LCD text to the existing RX-8 head unit. This project is designed to replace the SatNav hood controller as well as provide an add on control unit for the auxillary input and LCD text display.

The reason this project exist is simple, the RX-8 is a classic, but it's SatNav and Head Unit is showing it's age. This project allows you to remove the original components of the Sat Nav, add a tablet (or Raspberry Pi running OpenAuto Pro) and still keep the functionality of the motorized tilt. The project also allows you to add a much needed (and cheap) aux input to head units (primarally with firmware version 10.1).

You can pick and choose components out of this project. The SatNav screen controller can work as a standalone unit, so can the aux input system. You don't need to incorporate the LCD control either if you don't want.

The Arduino's do communicate with eachother, this allows for some cool LCD display functions as well as future functionality.

<!-- GETTING STARTED -->
## Getting Started

To get this project up and running, not a lot is needed! It's basically an Arduino connected to the motor with a L293D for the Hood control and another arduino for the aux input and LCD display.

### Prerequisites

Here is a list of both software and hardware you need to get the project running.
1. Arduino IDE (I use VS Code, however it's not nessisary)
2. Arduino Uno x2 (You can use any Arduino, or even something like an ESP32, however, you may need to modify the code) 
3. L293D (This is just a cheap motor driver chip)
4. 5v DC regulator (Not always required, but this is for ignition sense, you could also get this from something like a Raspberry Pi)


### Running the Project

To compile and deploy the code, you will need the Arduino IDE or the Arduino Compiler. As this code is designed to be run on an Arduino Uno, all the code is set up to compile directly to the Uno.

This project uses the AVR_SLEEP library to enable the Arduino to enter deep sleep to conserve your car's battery power when you turn your car off. When the Arduino enters deep sleep, your car battery will provide approximately 2 months of power. **I AM NOT RESPONSIBLE FOR YOUR BATTERY DYING!** If you plan to not use your car for a few months, it's recommended you disconnect the battery anyway, but I would at least recommend you disconnect the Arduino to conserve even more battery power. Your car will use power anyway, but the Arduino only consumes approximately 0.5mA. 

## Contributing

Contributions are what make the open source community such an amazing place to learn, inspire, and create. Any contributions you make are **greatly appreciated**.

If you have a suggestion that would make this better, please fork the repo and create a pull request. You can also simply open an issue with the tag "enhancement".
Don't forget to give the project a star! Thanks again!

1. Fork the Project
2. Create your Feature Branch
3. Commit your Changes
4. Push to the Branch
5. Open a Pull Request

<!-- LICENSE -->
## License

Distributed under the MIT License. See `LICENSE.txt` for more information.
