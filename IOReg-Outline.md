## IOReg driver registration tree 
### Four virtual serial ports

    +-o IOUserResources  <class IOUserResources, id 0x100000116, registered, matched, active, busy 0 (1025 ms), retain 8>
    | | {
    | |   "IOKit" = "IOService"
    | |   "IODEXTMatchCount" = 0x1
    | |   "IOResourceMatched" = ("IOKit","IODEXTMatchCount","IOResourceMatched")
    | | }
    | | 
    | +-o VSPDriver  <class IOUserService, id 0x100000a57, registered, matched, active, busy 0 (2 ms), retain 11>
    |   | {
    |   |   "IOClass" = "IOUserService"
    |   |   "CFBundleIdentifier" = "org.eof.tools.VSPDriver"
    |   |   "IOProviderClass" = "IOUserResources"
    |   |   "IOUserServerCDHash" = "5b74015b879b93a48be5497fe518349e2624d948"
    |   |   "IOPowerManagement" = {"CapabilityFlags"=0x0,"MaxPowerState"=0x2,"CurrentPowerState"=0x0}
    |   |   "IOMatchedPersonality" = {"IOUserClass"="VSPDriver","CFBundleIdentifier"="org.eof.tools.VSPDriver","IOMatchCategory"="org.eof.tools.VSPDriver","IOUserServerC$
    |   |   "IOResourceMatch" = "IOKit"
    |   |   "IOProbeScore" = 0x0
    |   |   "IOMatchCategory" = "org.eof.tools.VSPDriver"
    |   |   "IOUserServerName" = "org.eof.tools.VSPDriver"
    |   |   "UserClientProperties" = {"IOUserClass"="VSPSerialPort","HiddenPort"=No,"IOClass"="IOUserSerial","IOProviderClass"="IOSerialStreamSync","CFBundleIdentifierKe$
    |   |   "CFBundleIdentifierKernel" = "com.apple.kpi.iokit"
    |   |   "IOUserClass" = "VSPDriver"
    |   | }
    |   | 
    |   +-o VSPSerialPort  <class IOUserSerial, id 0x100000a59, registered, matched, active, busy 0 (0 ms), retain 10>
    |   | | {
    |   | |   "IOUserClass" = "VSPSerialPort"
    |   | |   "HiddenPort" = No
    |   | |   "IOClass" = "IOUserSerial"
    |   | |   "IOProviderClass" = "IOSerialStreamSync"
    |   | |   "CFBundleIdentifierKernel" = "com.apple.driver.driverkit.serial"
    |   | |   "IOTTYBaseName" = "serial-"
    |   | |   "IOTTYSuffix" = "100000A59"
    |   | |   "IOServiceDEXTEntitlements" = "com.apple.developer.driverkit.family.serial"
    |   | | }
    |   | | 
    |   | +-o IOSerialBSDClient  <class IOSerialBSDClient, id 0x100000a5b, registered, matched, active, busy 0 (0 ms), retain 5>
    |   |     {
    |   |       "IOClass" = "IOSerialBSDClient"
    |   |       "CFBundleIdentifier" = "com.apple.iokit.IOSerialFamily"
    |   |       "IOProviderClass" = "IOSerialStreamSync"
    |   |       "IOTTYBaseName" = "serial-"
    |   |       "IOSerialBSDClientType" = "IOSerialStream"
    |   |       "IOProbeScore" = 0x3e8
    |   |       "IOResourceMatch" = "IOBSD"
    |   |       "IOMatchedAtBoot" = Yes
    |   |       "IOMatchCategory" = "IODefaultMatchCategory"
    |   |       "IOTTYDevice" = "serial-100000A59"
    |   |       "IOCalloutDevice" = "/dev/cu.serial-100000A59"
    |   |       "IODialinDevice" = "/dev/tty.serial-100000A59"
    |   |       "IOGeneralInterest" = "IOCommand is not serializable"
    |   |       "IOPersonalityPublisher" = "com.apple.iokit.IOSerialFamily"
    |   |       "CFBundleIdentifierKernel" = "com.apple.iokit.IOSerialFamily"
    |   |       "IOTTYSuffix" = "100000A59"
    |   |     }
    |   |     
    |   +-o VSPSerialPort  <class IOUserSerial, id 0x100000a5a, registered, matched, active, busy 0 (0 ms), retain 10>
    |   | | {
    |   | |   "IOUserClass" = "VSPSerialPort"
    |   | |   "HiddenPort" = No
    |   | |   "IOClass" = "IOUserSerial"
    |   | |   "IOProviderClass" = "IOSerialStreamSync"
    |   | |   "CFBundleIdentifierKernel" = "com.apple.driver.driverkit.serial"
    |   | |   "IOTTYBaseName" = "serial-"
    |   | |   "IOTTYSuffix" = "100000A5A"
    |   | |   "IOServiceDEXTEntitlements" = "com.apple.developer.driverkit.family.serial"
    |   | | }
    |   | | 
    |   | +-o IOSerialBSDClient  <class IOSerialBSDClient, id 0x100000a5d, registered, matched, active, busy 0 (0 ms), retain 5>
    |   |     {
    |   |       "IOClass" = "IOSerialBSDClient"
    |   |       "CFBundleIdentifier" = "com.apple.iokit.IOSerialFamily"
    |   |       "IOProviderClass" = "IOSerialStreamSync"
    |   |       "IOTTYBaseName" = "serial-"
    |   |       "IOSerialBSDClientType" = "IOSerialStream"
    |   |       "IOProbeScore" = 0x3e8
    |   |       "IOResourceMatch" = "IOBSD"
    |   |       "IOMatchedAtBoot" = Yes
    |   |       "IOMatchCategory" = "IODefaultMatchCategory"
    |   |       "IOTTYDevice" = "serial-100000A5A"
    |   |       "IOCalloutDevice" = "/dev/cu.serial-100000A5A"
    |   |       "IODialinDevice" = "/dev/tty.serial-100000A5A"
    |   |       "IOGeneralInterest" = "IOCommand is not serializable"
    |   |       "IOPersonalityPublisher" = "com.apple.iokit.IOSerialFamily"
    |   |       "CFBundleIdentifierKernel" = "com.apple.iokit.IOSerialFamily"
    |   |       "IOTTYSuffix" = "100000A5A"
    |   |     }
    |   |     
    |   +-o VSPSerialPort  <class IOUserSerial, id 0x100000a5c, registered, matched, active, busy 0 (0 ms), retain 10>
    |   | | {
    |   | |   "IOUserClass" = "VSPSerialPort"
    |   | |   "HiddenPort" = No
    |   | |   "IOClass" = "IOUserSerial"
    |   | |   "IOProviderClass" = "IOSerialStreamSync"
    |   | |   "CFBundleIdentifierKernel" = "com.apple.driver.driverkit.serial"
    |   | |   "IOTTYBaseName" = "serial-"
    |   | |   "IOTTYSuffix" = "100000A5C"
    |   | |   "IOServiceDEXTEntitlements" = "com.apple.developer.driverkit.family.serial"
    |   | | }
    |   | | 
    |   | +-o IOSerialBSDClient  <class IOSerialBSDClient, id 0x100000a5f, registered, matched, active, busy 0 (0 ms), retain 5>
    |   |     {
    |   |       "IOClass" = "IOSerialBSDClient"
    |   |       "CFBundleIdentifier" = "com.apple.iokit.IOSerialFamily"
    |   |       "IOProviderClass" = "IOSerialStreamSync"
    |   |       "IOTTYBaseName" = "serial-"
    |   |       "IOSerialBSDClientType" = "IOSerialStream"
    |   |       "IOProbeScore" = 0x3e8
    |   |       "IOResourceMatch" = "IOBSD"
    |   |       "IOMatchedAtBoot" = Yes
    |   |       "IOMatchCategory" = "IODefaultMatchCategory"
    |   |       "IOTTYDevice" = "serial-100000A5C"
    |   |       "IOCalloutDevice" = "/dev/cu.serial-100000A5C"
    |   |       "IODialinDevice" = "/dev/tty.serial-100000A5C"
    |   |       "IOGeneralInterest" = "IOCommand is not serializable"
    |   |       "IOPersonalityPublisher" = "com.apple.iokit.IOSerialFamily"
    |   |       "CFBundleIdentifierKernel" = "com.apple.iokit.IOSerialFamily"
    |   |       "IOTTYSuffix" = "100000A5C"
    |   |     }
    |   |     
    |   +-o VSPSerialPort  <class IOUserSerial, id 0x100000a5e, registered, matched, active, busy 0 (0 ms), retain 10>
    |     | {
    |     |   "IOUserClass" = "VSPSerialPort"
    |     |   "HiddenPort" = No
    |     |   "IOClass" = "IOUserSerial"
    |     |   "IOProviderClass" = "IOSerialStreamSync"
    |     |   "CFBundleIdentifierKernel" = "com.apple.driver.driverkit.serial"
    |     |   "IOTTYBaseName" = "serial-"
    |     |   "IOTTYSuffix" = "100000A5E"
    |     |   "IOServiceDEXTEntitlements" = "com.apple.developer.driverkit.family.serial"
    |     | }
    |     | 
    |     +-o IOSerialBSDClient  <class IOSerialBSDClient, id 0x100000a60, registered, matched, active, busy 0 (0 ms), retain 5>
    |         {
    |           "IOClass" = "IOSerialBSDClient"
    |           "CFBundleIdentifier" = "com.apple.iokit.IOSerialFamily"
    |           "IOProviderClass" = "IOSerialStreamSync"
    |           "IOTTYBaseName" = "serial-"
    |           "IOSerialBSDClientType" = "IOSerialStream"
    |           "IOProbeScore" = 0x3e8
    |           "IOResourceMatch" = "IOBSD"
    |           "IOMatchedAtBoot" = Yes
    |           "IOMatchCategory" = "IODefaultMatchCategory"
    |           "IOTTYDevice" = "serial-100000A5E"
    |           "IOCalloutDevice" = "/dev/cu.serial-100000A5E"
    |           "IODialinDevice" = "/dev/tty.serial-100000A5E"
    |           "IOGeneralInterest" = "IOCommand is not serializable"
    |           "IOPersonalityPublisher" = "com.apple.iokit.IOSerialFamily"
    |           "CFBundleIdentifierKernel" = "com.apple.iokit.IOSerialFamily"
    |           "IOTTYSuffix" = "100000A5E"
    |         }

That's the default load.
