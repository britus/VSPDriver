## VSPDriver - Virtual Serial Port Driver

This is a MacOS DriverKit driver implementation including
a installation App suggested by Apple.

- The project is still work in progress 
- Request to viewers: Need help to continue (> /pm me please ;-)
 
### Current development status

- Without a port connection, the data is sent back to the sender
- With a port connection, the data is written from sender 1 and receiver 2 and vice versa
- The installation app has a generic function test for the IOUserClient (controller)
- The project includes a test app to test the serial interfaces. The app is written using the QT framework. Runs with QT 5 and 6..

### ToDo:
- Write a library that handles communication with the IOUserClient and can be used in other app projects.
- Write a controller app that handles the complete management of the port links and provides a TCP server to redirect the data from the serial interface to TCP/IP.
