## VSPDriver - Virtual Serial Port Driver

This is a macOS DriverKit driver implementation including a user interface App.
 
### Current development status

- Without a port link, the data is sent back to the sender
- With a port link, the data is written from sender 1 to receiver 2 and vice versa
- The client app provide installation, deinstallation and management

### Release

- [All releases](https://github.com/britus/VSPClient/releases)

### Log Files and Tester

A look at this [log](https://github.com/britus/VSPDriver/blob/master/VSPDriver-Full.log) shows how the driver ticks with the IOUserClient.
This [picture](https://github.com/britus/VSPDriver/blob/master/VSPDriver-Tester.jpg) shows the VSP driver in action.

## Notes for DIY

To build this project you have to do:

- Install QT5 latest for Intel based macOS from archive
- Install QT6 latest for M chip based macOS

- Install the QT framework like
```
~/Qt
```

### Build the VSPDriver project with all frameworks inside

- Use branch QT_5.15.2_x86_64_macos_12.7.6 if you want to build for older macOS
- Use branch QT_6.8.2_arm64_M3_Pro if you want to build for newer macOS

### Entitlements, signing and security checks

You should use you own bundle IDs. To do this:

Replace bundle ID "org.eof.tools.VSPDriver" in the Xcode VSPDriver target and replace 
bundle ID "org.eof.tools.VSPClient" in the Xcode VSPClient target

After that you must replace the Dext bundle ID "org.eof.tools.VSPDriver" in 
VSPClient.entitlements file.

### Update source code to install the Dext

Change the the bundle ID in [VSPClient/vscmainwindow.cpp](https://github.com/britus/VSPClient/blob/master/VSPClient/ui/vscmainwindow.cpp) in method "createVspController()".

## VSPDriver without any signing and security checks.

### The macOS System Integrity Protection (SIP)

Turning off SIP is only possible in the Recovery OS. To do this for your 
hardware, follow the instructions in [Apple's documentation](https://duckduckgo.com/?q=Turning+off+SIP).

In the Recovery OS, open the terminal window from the main menu and enter 
following command:

### Disable the protection on the machine. 

```
$> csrutil disable
```

### Enable the protection on the machine.

```
$> csrutil disable
```

### Clear the existing configuration.

```
$> csrutil clear
```

### Setup NVRAM to disable signing checks

```
$> sudo nvram boot-args="dk=0x8001"
```

### Delete NVRAM boot-args

```
$> sudo nvram -d boot-args
```

### Enable developer mode for system extensions

```
$> systemextensionsctl developer on
```

### Disable developer mode for system extensions

```
$> systemextensionsctl developer off
```

## Donation

If you want to [donate my work](https://www.paypal.com/donate/?hosted_button_id=4QZT5YLGGW7S4), please feel free.
You can use the [QR-Code](https://github.com/britus/VSPDriver/blob/master/VSPDriver-Donate_Please.png) too.

Thank you very much.
