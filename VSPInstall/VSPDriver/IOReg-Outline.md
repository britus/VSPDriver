##  IOReg driver registration tree

    | +-o VSPDriver  <class IOUserSerial, id 0x100000a45, registered, matched, active, busy 0 (0 ms), retain 10>
    |   | {
    |   |   "IOClass" = "IOUserSerial"
    |   |   "CFBundleIdentifier" = "org.eof.tools.VSPDriver"
    |   |   "IOProviderClass" = "IOUserResources"
    |   |   "IOUserServerCDHash" = "d0a7a6ef73682de029dcb05bc78701951c4bc7d7"
    |   |   "IOServiceDEXTEntitlements" = "com.apple.developer.driverkit.family.serial"
    |   |   "IOTTYBaseName" = "serial-"
    |   |   "IOMatchedPersonality" = {"IOUserClass"="VSPDriver","CFBundleIdentifier"="org.eof.tools.VSPDriver","IOMatchCategory"="org.eof.tools.VSPDriver","IOClass"="IOU$
    |   |   "IOResourceMatch" = "IOKit"
    |   |   "IOProbeScore" = 0xffffffffffffffff
    |   |   "HiddenPort" = Yes
    |   |   "IOMatchCategory" = "org.eof.tools.VSPDriver"
    |   |   "IOUserServerName" = "org.eof.tools.VSPDriver"
    |   |   "IOPowerManagement" = {"CapabilityFlags"=0x0,"MaxPowerState"=0x2,"CurrentPowerState"=0x0}
    |   |   "CFBundleIdentifierKernel" = "com.apple.driver.driverkit.serial"
    |   |   "IOTTYSuffix" = "100000A45"
    |   |   "IOUserClass" = "VSPDriver"
    |   | }
    |   | 
    |   +-o IOSerialBSDClient  <class IOSerialBSDClient, id 0x100000a47, registered, matched, active, busy 0 (0 ms), retain 5>
    |       {
    |         "IOClass" = "IOSerialBSDClient"
    |         "CFBundleIdentifier" = "com.apple.iokit.IOSerialFamily"
    |         "IOProviderClass" = "IOSerialStreamSync"
    |         "IOTTYBaseName" = "serial-"
    |         "IOSerialBSDClientType" = "IOSerialStream"
    |         "IOProbeScore" = 0x3e8
    |         "IOResourceMatch" = "IOBSD"
    |         "IOMatchedAtBoot" = Yes
    |         "IOMatchCategory" = "IODefaultMatchCategory"
    |         "IOTTYDevice" = "serial-100000A45"
    |         "IOCalloutDevice" = "/dev/cu.serial-100000A45"
    |         "IODialinDevice" = "/dev/tty.serial-100000A45"
    |         "IOPersonalityPublisher" = "com.apple.iokit.IOSerialFamily"
    |         "CFBundleIdentifierKernel" = "com.apple.iokit.IOSerialFamily"
    |         "IOTTYSuffix" = "100000A45"
    |       }
    |   
    +-o IOUserServer(org.eof.tools.VSPDriver-0x100000a45)  <class IOUserServer, id 0x100000a46, registered, matched, active, busy 0 (0 ms), retain 11>
        {
          "IOUserClientDefaultLocking" = Yes
          "IOPowerManagement" = {"CapabilityFlags"=0x0,"MaxPowerState"=0x2,"CurrentPowerState"=0x0}
          "IOUserClientCreator" = "pid 1227, org.eof.tools.VS"
          "IOUserServerName" = "org.eof.tools.VSPDriver"
          "IOUserServerTag" = 0x100000a45
          "IOAssociatedServices" = (0x100000a45)
        }
        
