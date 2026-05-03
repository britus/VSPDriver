# VSPClient User Documentation

## Overview

VSPClient is the graphical control center for **Virtual Serial Port Controller** on macOS. The app creates virtual serial ports, connects them to port links, records driver activities, and opens Terminal windows for testing data streams.

The application is aimed at users who want to simulate, test, or logically interconnect multiple tools and devices via serial interfaces.

## Main Areas of the App

The sidebar divides the app into three work areas:

### 1. Serial Ports

In the **Serial Ports** area, you manage existing virtual interfaces.

- The list on the left shows all created virtual ports.
- On the right, you see details of the currently selected port.
- Use `+` to create a new port.
- Use the trash can icon to delete the selected port.

Typical port parameters:

- Baud rate
- Data bits
- Stop bits
- Parity
- Flow Control

A new port is created via the **Create Serial Port** dialog. There, you select the desired communication parameters.

## 2. Port Links

In the **Port Links** area, you connect virtual ports to logical cables.

A link consists of:

- a **Source Port**
- a **Target Port**
- optionally a **Target 2**

### Link Types

#### Point-to-Point

A simple link connects exactly two ports together.

Example:

- `vsp1 <-> vsp2`

#### Branched Link with Two Targets

A link can also include a second target.

Two routing modes are available:

- **Answer Source only**  
  Responses from target ports go back only to the source port.
- **Answer Source and peer target**  
  Responses are forwarded to both the source port and the other target port.

### Creating a Link

1. Open **Port Links**.
2. Click on `+`.
3. Select **Source port** and **Target port**.
4. Optionally select **Target 2**.
5. Enable **Answer source only** if needed.
6. Click **Create**.

Note: A new link can only be created if at least two free, unlinked ports are available.

## 3. Driver Traces

In the **Driver Traces** area, you specify which driver checks and data flows are logged.

Activatable checks:

- Baud rate
- Data bits
- Stop bits
- Parity
- Flow control

Activatable traces:

- Trace RX
- Trace TX
- Trace control

Available actions:

- **Save To Driver** saves the current trace configuration.
- **Refresh** reloads the current driver status.
- **Clear Log** clears the log output.

The lower area shows the driver log in real time.

## Opening Terminal Windows

Using the Terminal icon in the top toolbar, you open a new **Serial Terminal**.

A terminal is used for direct testing of traffic on a virtual port.

## Working with Serial Terminal

In the terminal, you first select:

- the serial port
- Baud rate
- Data bits
- Stop bits
- Parity
- Flow Control

Then you connect using the Connect icon.

### Sending Text

- Enter outgoing text in **Text to send**.
- Enable **Add CR** and **Add LF** if needed.
- Send the content via the Send function.

### Sending File Contents

Using the **Send File** icon, you can transfer file contents in blocks to the connected port.

### Loop Test

The Loop function automatically generates continuous test strings.

- **Loop length** determines the length of the generated text.
- Start and stop the test via the Loop icon.

### Log Display

The lower area of the window shows:

- sent data
- received data
- status messages

## Typical Test Scenarios

### Testing Two Ports Directly Against Each Other

1. Create two virtual ports.
2. Create a link between both ports.
3. Open two Terminal windows.
4. Connect each terminal to a different port.
5. Send test data in one window and check the output in the other.

## Driver Installation and Activation

If the driver is not fully activated yet, the app displays a hint page.

Possible states:

- Waiting for Driver Extension activation in macOS
- Restart required

In this case, follow the app's instructions, activate the Driver Extension in System Settings, and restart your Mac if necessary.

## Tips for Daily Use

- Delete a port only when you no longer need the associated link.
- Use **Driver Traces** when a connection reacts unexpectedly or parameters are not correctly applied.
- Use the Terminal for functional checks before connecting external software or hardware.
- For more complex routing scenarios, a link with a second target port is worthwhile.
