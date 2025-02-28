## VSPDriver - Virtual Serial Port Driver

This is a macOS DriverKit driver implementation including
a installation App suggested by Apple.
 
### Current development status

- Without a port link, the data is sent back to the sender
- With a port link, the data is written from sender 1 to receiver 2 and vice versa
- The client app provide installation, deinstallation and management

### Client application / referenced frameworks
The Project [VSPClient](https://github.com/britus/VSPClient) contains the user interface for managing the virtual serialports. This project provide the
3 frameworks written in QT for the virtual serial port driver.

### Log Files and Tester

A look at this [log](https://github.com/britus/VSPDriver/blob/master/VSPDriver-Full.log) shows how the driver ticks with the IOUserClient.
This [picture](https://github.com/britus/VSPDriver/blob/master/VSPDriver-Tester.jpg) shows the VSP driver in action.

## Donation

If you want to [donate my work](https://www.paypal.com/donate/?hosted_button_id=4QZT5YLGGW7S4), please feel free and do it.
You can use the [QR-Code](https://github.com/britus/VSPDriver/blob/master/VSPDriver-Donate_Please.png) too.

Thank you very much.
