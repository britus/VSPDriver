##  IOReg driver registration tree

    | +-o VSPDriver  <class IOUserService, id 0x100000965, registered, matched, active, busy 0 (1 ms), retain 8>
    |   | {
    |   |   "IOClass" = "IOUserService"
    |   |   "CFBundleIdentifier" = "org.eof.tools.VSPDriver"
    |   |   "IOProviderClass" = "IOUserResources"
    |   |   "IOUserServerCDHash" = "f5800d2b1b3bb24dd2fc0c45833d287805d9aabc"
    |   |   "IOPowerManagement" = {"CapabilityFlags"=0x0,"MaxPowerState"=0x2,"CurrentPowerState"=0x0}
    |   |   "IOMatchedPersonality" = {"IOUserClass"="VSPDriver","CFBundleIdentifier"="org.eof.tools.VSPDriver","IOUserServerCDHash"="f5800d2b1b3bb24dd2fc0c45833d287805d9$
    |   |   "IOResourceMatch" = "IOKit"
    |   |   "IOProbeScore" = 0x0
    |   |   "IOUserServerName" = "org.eof.tools.VSPDriver"
    |   |   "IOMatchCategory" = "org.eof.tools.VSPDriver"
    |   |   "UserClientProperties" = {"IOUserClass"="VSPSerialPort","HiddenPort"=No,"IOClass"="IOUserSerial","IOProviderClass"="IOSerialStreamSync","CFBundleIdentifierKe$
    |   |   "CFBundleIdentifierKernel" = "com.apple.kpi.iokit"
    |   |   "IOUserClass" = "VSPDriver"
    |   | }
    |   | 
    |   +-o VSPSerialPort  <class IOUserSerial, id 0x100000967, registered, matched, active, busy 0 (1 ms), retain 10>
    |     | {
    |     |   "IOUserClass" = "VSPSerialPort"
    |     |   "HiddenPort" = No
    |     |   "IOClass" = "IOUserSerial"
    |     |   "IOProviderClass" = "IOSerialStreamSync"
    |     |   "CFBundleIdentifierKernel" = "com.apple.driver.driverkit.serial"
    |     |   "IOTTYBaseName" = "serial-"
    |     |   "IOTTYSuffix" = "100000967"
    |     |   "IOServiceDEXTEntitlements" = "com.apple.developer.driverkit.family.serial"
    |     | }
    |     | 
    |     +-o IOSerialBSDClient  <class IOSerialBSDClient, id 0x100000968, registered, matched, active, busy 0 (0 ms), retain 5>
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
    |           "IOTTYDevice" = "serial-100000967"
    |           "IOCalloutDevice" = "/dev/cu.serial-100000967"
    |           "IODialinDevice" = "/dev/tty.serial-100000967"
    |           "IOGeneralInterest" = "IOCommand is not serializable"
    |           "IOPersonalityPublisher" = "com.apple.iokit.IOSerialFamily"
    |           "CFBundleIdentifierKernel" = "com.apple.iokit.IOSerialFamily"
    |           "IOTTYSuffix" = "100000967"
    |         }
    |         
