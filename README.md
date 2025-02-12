## VSPDriver - Virtual Serial Port Driver

This is a MacOS DriverKit driver implementation including
a installation App suggested by Apple.

- The project is still work in progress 
- Request to viewers: Need help to continue (> /pm me please ;-)
 
### Current development status

- Without a port link, the data is sent back to the sender
- With a port link, the data is written from sender 1 to receiver 2 and vice versa
- The installation app has a simple and generic function test for the IOUserClient (controller)
- The project includes a test app to test the serial interfaces. The app is written using the QT framework. Runs with QT 5 and 6..

### ToDo:
- Write a library that handles communication with the IOUserClient and can be used in other app projects.
- Write a controller app that handles the complete management of the port links and provides a TCP server to redirect the data from the serial interface to TCP/IP.

### Donation
If you want to [donate my hard work](https://www.paypal.com/donate/?hosted_button_id=4QZT5YLGGW7S4), please feel free and do it :-)

You can use the [QR-Code](https://github.com/britus/VSPDriver/blob/master/VSPDriver-Donate_Please.png) too.

Thank you very much.
