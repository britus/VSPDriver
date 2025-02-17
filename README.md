## VSPDriver - Virtual Serial Port Driver

This is a MacOS DriverKit driver implementation including
a installation App suggested by Apple.
 
### Current development status

- Without a port link, the data is sent back to the sender
- With a port link, the data is written from sender 1 to receiver 2 and vice versa
- The installation app has a simple and generic function test for the IOUserClient (controller)
- The project includes a test app to test the serial interfaces. The app is written using the QT framework. Runs with QT 5 and 6..

### ToDo:
- Write a library that handles communication with the IOUserClient and can be used in other app projects.
- Write a controller app that handles the complete management of the port links and provides a TCP server to redirect the data from the serial interface to TCP/IP.

### Client application
The Project [VSPClient](https://github.com/britus/VSPClient) contains the user interface for managing and two frameworks in QT for the virtual serial port driver

## Log Files and Tester

A look at this [log](https://github.com/britus/VSPDriver/blob/master/VSPDriver-Full.log) shows how the driver ticks with the IOUserClient :)
This [picture](https://github.com/britus/VSPDriver/blob/master/VSPDriver-Tester.jpg) shows the VSP driver in action :)

### Donation

*Request to viewers: Need help to continue (> /pm me please ;-)*

If you want to [donate my hard work](https://www.paypal.com/donate/?hosted_button_id=4QZT5YLGGW7S4), please feel free and do it :-)
.. or you can use the [QR-Code](https://github.com/britus/VSPDriver/blob/master/VSPDriver-Donate_Please.png) too.

Thank you very much.
