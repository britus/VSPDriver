//
//  VSPDataModel.h
//  VSPClient
//
//  Created by Björn Eschrich on 27.10.25.
//

#ifndef VSPDataModel_h
#define VSPDataModel_h

#define MAGIC_CONTROL 0xBE6605250000L
#define MAX_SERIAL_PORTS 16
#define MAX_PORT_LINKS 16
#define MAX_PORT_NAME 64

#ifndef BIT
#define BIT(x) (1 << x)
#endif

#define TRACE_PORT_RX BIT(16)
#define TRACE_PORT_TX BIT(17)
#define TRACE_PORT_IO BIT(18)
#define CHECK_BAUD BIT(19)
#define CHECK_DATA_SIZE BIT(20)
#define CHECK_STOP_BITS BIT(21)
#define CHECK_PARITY BIT(22)
#define CHECK_FLOWCTRL BIT(23)

typedef enum {
    vspContextPing = 0x01,
    vspContextPort = 0x02,
    vspContextResult = 0x03,
    vspContextError = 0x04,
} CVSPUserContext;

typedef enum {
    vspControlPingPong = 0,
    vspControlGetStatus = 1,
    vspControlCreatePort = 2,
    vspControlRemovePort = 3,
    vspControlLinkPorts = 4,
    vspControlUnlinkPorts = 5,
    vspControlGetPortList = 6,
    vspControlGetLinkList = 7,
    vspControlEnableChecks = 8,
    vspControlEnableTrace = 9,
    
    // Has to be last
    vspLastCommand = 10,
} CVSPControlCommand;

typedef struct
{
    uint8_t id;
    uint64_t flags;
    char name[MAX_PORT_NAME];
} CVSPPortListItem;

typedef struct
{
    uint32_t baudRate;
    uint8_t dataBits;
    uint8_t stopBits;
    uint8_t parity;
    uint8_t flowCtrl;
} CVSPPortParameters;

typedef struct
{
    /* In whitch context calld (enum CVSPUserContext) */
    uint8_t context;

    /* User client command (enum CVSPControlCommand) */
    uint8_t command;

    /* Command status response */
    struct Status
    {
        uint32_t code;
        uint64_t flags;
    } status;

    /* Command parameters */
    struct Parameter
    {
        /* parameter flags */
        uint64_t flags;

        /* port link parameters */
        struct PortLink
        {
            uint8_t source;
            uint8_t target;
        } link;
    } parameter;

    /* Available serial ports */
    struct PortList
    {
        uint8_t count;
        CVSPPortListItem list[MAX_SERIAL_PORTS];
    } ports;

    /* Available serial port links */
    struct LinkList
    {
        uint8_t count;
        uint64_t list[MAX_PORT_LINKS];
    } links;

} CVSPDriverData;

#ifndef VSP_UCD_SIZE
#define VSP_UCD_SIZE sizeof(CVSPDriverData)
#endif

typedef struct
{
    int oscode;
    int system;
    int sub;
    int code;
} CVSPSystemError;

#endif /* VSPDataModel_h */
