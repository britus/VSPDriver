// ================================================= //
// VSP Test Responder JavaScript
// ================================================= //

// For macOS JavaScriptCore compatibility:
// Use the following functions:
// - addFunction(name, func) - Adds a function
// - getFunction(name) - Retrieves a function
// - removeFunction(name) - Removes a function
// - hasFunction(name) - Checks if function exists
// - getFunctionNames() - Returns all function names
// - listAllFunctions() - Lists all functions

// Create a global Map object for functions
var functionMap = new Map()

// ================================================= //


/**
 * Parse command parameters.
 * @param {string} command
 *      ^^- The command string to parse
 * @returns {Array<string>}
 *      ^^- Array of parameter strings, First entry = Command
 */
function parseCommand(command) {
    // Check if input is a valid string
    if (typeof command !== 'string') {
        return [command]
    }

    // Handle empty or whitespace-only strings
    if (command.trim() === '') {
        return [command]
    }

    // Find the first '=' character
    var equalsIndex = command.indexOf('=')

    // If no '=' found, return empty array
    if (equalsIndex === -1) {
        return [command]
    }

    // Extract the part after '='
    var paramPart = command.substring(equalsIndex + 1)
    var cmd_only = command.substring(0, equalsIndex)

    // If no parameters found, return empty array
    if (paramPart.trim() === '') {
        return [cmd_only]
    }

    var paramArray = []

    // first command
    paramArray.push(cmd_only)

    // Split by comma and trim whitespace
    var list = paramPart.split(',')
    for (i = 0; i < list.length; i++) {
        paramArray.push(list[i])
    }

    // return command and parameters
    return paramArray
}


/**
 * Convert byte array with CR/LF seperation
 * into string list
 *
 * @param {ArrayBuffer|Uint8Array} byteArray
 *          ^^- The input byte array to convert
 * @returns {Array<string>}
 *          ^^- List of strings split by \r\n line endings
 */
function toStringList(byteArray) {
    // Convert byte array to string (assuming UTF-8 encoding)
    var text = ''

    // Handle different array types
    if (byteArray instanceof ArrayBuffer) {
        var uint8Array = new Uint8Array(byteArray)
        for (var i = 0; i < uint8Array.length; i++) {
            text += String.fromCharCode(uint8Array[i])
        }
    } else if (byteArray instanceof Uint8Array) {
        for (i = 0; i < byteArray.length; i++) {
            text += String.fromCharCode(byteArray[i])
        }
    } else {
        for (i = 0; i < byteArray.length; i++) {
            text += String.fromCharCode(byteArray[i])
        }
    }

    // Split by \r\n and filter out empty lines
    var lines = text.split('\r\n')

    // Filter out any empty strings
    return lines.filter(function (line) {
        return line.length > 0
    })
}

// Function to add a function to the Map object
function addFunction(name, func) {
    if (typeof name !== 'string') {
        throw new Error('Name must be a String')
    }
    if (typeof func !== 'function') {
        throw new Error('Function must be of type Function')
    }
    functionMap.set(name, func)
}

// Function to retrieve a function from the Map object
function getFunction(name) {
    return functionMap.get(name)
}

// Function to remove a function from the Map object
function removeFunction(name) {
    return functionMap.delete(name)
}

// Function to check if a function exists
function hasFunction(name) {
    return functionMap.has(name)
}

// Function to get all function names
function getFunctionNames() {
    return Array.from(functionMap.keys())
}

// Function to list all functions (for debugging purposes)
function listAllFunctions() {
    var names = getFunctionNames()
    print('Available functions:')
    names.forEach(function (name) {
        print('- ' + name)
    })
}

var cmd_at = function (pfx, sfx) {
    return 'AT\n\rOK\n\r'
}

var cmd_atz = function (pfx, sfx) {
    return 'ATZ\n\rOK\n\r'
}

var cmd_atq_0 = function (pfx, sfx) {
    return 'ATQ\n\rOK\n\r'
}

var cmd_atv_0 = function (pfx, sfx) {
    return 'ATV\n\rOK\n\r'
}

var cmd_atv_1 = function (pfx, sfx) {
    return 'ATV\n\rOK\n\r'
}

var cmd_at0 = function (pfx, sfx) {
    return 'AT0\n\rOK\n\r'
}

var cmd_at1 = function (pfx, sfx) {
    return 'AT1\n\rOK\n\r'
}

var cmd_ati = function (pfx, sfx) {
    return 'ATI\r\nOK\r\n'
}

var cmd_ate_1 = function (pfx, sfx) {
    return 'ATE\r\nOK\r\n'
}

var cmd_ats_0_1 = function (pfx, sfx) {
    return 'ATS0=' + sfx + '\r\nOK\r\n'
}

var cmd_atw_0 = function (pfx, sfx) {
    return 'AT&W0\r\nOK\r\n'
}

var cmd_atx4 = function (pfx, sfx) {
    return 'ATX4\r\nOK\r\n'
}

var cmd_cpin_q = function (pfx, sfx) {
    //return '+CPIN: SIM PIN\r\nOK\r\n'
    return '+CPIN: READY\r\nOK\r\n'
}

var cmd_cpin_s = function (pfx, sfx) {
    return 'AT+CPIN=' + sfx + '\r\nOK\r\n'
}

var cmd_cmee_s = function (pfx, sfx) {
    return 'AT+CMEE=' + sfx + '\r\nOK\r\n'
}

var cmd_ctzu_s = function (pfx, sfx) {
    return 'AT+CTZU=' + sfx + '\r\nOK\r\n'
}

var cmd_cclk_s = function (pfx, sfx) {
    return 'AT+CCLK=' + sfx + '\r\nOK\r\n'
}

var cmd_qgpsend_q = function (pfx, sfx) {
    return 'AT+QGPSEND\r\nOK\r\n'
}

var cmd_cfun_q = function (pfx, sfx) {
    return 'AT+CFUN?\r\n+CFUN: 1\r\nOK\r\n'
}

var cmd_qsclk_q = function (pfx, sfx) {
    return 'AT+QSCLK=' + sfx + '\r\nOK\r\n'
}

var cmd_qindcfg_s = function (pfx, sfx) {
    return 'AT+QINDCFG=' + sfx + '\r\nOK\r\n'
}

var cmd_qsimstat_q = function (pfx, sfx) {
    return 'AT+QSIMSTAT\r\n+QSIMSTAT: 1\r\nOK\r\n'
}

var cmd_qsimstat_s = function (pfx, sfx) {
    return 'AT+QSIMSTAT=' + sfx + '\r\nOK\r\n'
}

var cmd_qinistat_q = function (pfx, sfx) {
    return 'AT+QINISTAT\r\n+QINISTAT: 7\r\nOK\r\n'
}

var cmd_qinistat_s = function (pfx, sfx) {
    return 'AT+QINISTAT=' + sfx + '\r\nOK\r\n'
}

var cmd_qsimdet_q = function (pfx, sfx) {
    return 'AT+QSIMDET\r\n+QSIMDET: 3\r\nOK\r\n'
}

var cmd_qpinc_q = function (pfx, sfx) {
    return 'AT+QPINC\r\n+QPINC: 99\r\nOK\r\n'
}

var cmd_gmi_q = function (pfx, sfx) {
    return 'AT+GMI\r\nQUECTEL SIMULATOR\r\nOK\r\n'
}

var cmd_cgmi_q = function (pfx, sfx) {
    return 'AT+CGMI\r\nQUECTEL SIM\r\nOK\r\n'
}

var cmd_gmm_q = function (pfx, sfx) {
    return 'AT+GMM\r\nEG25\r\nOK\r\n'
}

var cmd_cgmm_q = function (pfx, sfx) {
    return 'AT+CGMM\r\nEG25\r\nOK\r\n'
}

var cmd_gmr_q = function (pfx, sfx) {
    return 'AT+GMR\r\nEG25GGBR000000\r\nOK\r\n'
}

var cmd_cgmr_q = function (pfx, sfx) {
    return 'AT+CGMR\r\nEG25GGBR000000\r\nOK\r\n'
}

var cmd_csub_q = function (pfx, sfx) {
    return 'AT+CSUB\r\nSubEdition: V01\r\nOK\r\n'
}

var cmd_gsn_q = function (pfx, sfx) {
    return 'AT+GSN\r\n123456789112233\r\nOK\r\n'
}

var cmd_cgsn_q = function (pfx, sfx) {
    return 'AT+CGSN\r\n123456789112233\r\nOK\r\n'
}

var cmd_cimi_q = function (pfx, sfx) {
    return 'AT+CIMI\r\n098765432199999\r\nOK\r\n'
}

var cmd_qccuid_q = function (pfx, sfx) {
    return 'AT+QCCID\r\n+QCCID: 11223344556677\r\nOK\r\n'
}

var cmd_cmee_q = function (pfx, sfx) {
    return 'AT+CMEE?\r\n+CMEE: 2\r\nOK\r\n'
}

var cmd_cscs_q = function (pfx, sfx) {
    return 'AT+CSCS?\r\n+CSCS: "GSM"\r\nOK\r\n'
}

var cmd_csca_q = function (pfx, sfx) {
    return 'AT+CSCA?\r\n+CSCA: +232123456789,145\r\nOK\r\n'
}

var cmd_csca_s = function (pfx, sfx) {
    return 'AT+CSCA\r\n+CSCA: ' + sfx + '\r\nOK\r\n'
}

var cmd_cnum_q = function (pfx, sfx) {
    return 'AT+CNUM\r\n+CNUM: +2329087651234,145\r\nOK\r\n'
}

var cmd_gcap_q = function (pfx, sfx) {
    return 'AT+GCAP\r\n+GCAP: +CGSM\r\nOK\r\n'
}

var cmd_copn_q = function (pfx, sfx) {
    return 'AT+COPN\r\n+COPN: "AA101","Test PLMN 1-1"\r\nOK\r\n'
}

var cmd_qadc_q = function (pfx, sfx) {
    return 'AT+QADC\r\n+QADC: 1,34\r\nOK\r\n'
}

var cmd_csq_q = function (pfx, sfx) {
    return 'AT+CSQ\r\n+CSQ: 20,99\r\nOK\r\n'
}

var cmd_creg_q = function (pfx, sfx) {
    return 'AT+CREG?\r\n+CREG: 2,1,"AA101","AA101",7\r\nOK\r\n'
}

var cmd_creg_s = function (pfx, sfx) {
    return 'AT+CREG\r\n+CREG: ' + sfx + '\r\nOK\r\n'
}

var cmd_cgreg_q = function (pfx, sfx) {
    return 'AT+CGREG?\r\n+CGREG: 2,1,"AA101","AA101",7\r\nOK\r\n'
}

var cmd_cgreg_s = function (pfx, sfx) {
    return 'AT+CGREG\r\n+CGREG: ' + sfx + '\r\nOK\r\n'
}

var cmd_cereg_q = function (pfx, sfx) {
    return 'AT+CEREG?\r\n+CEREG: 2,1,"AA101","AA101",7\r\nOK\r\n'
}

var cmd_cereg_s = function (pfx, sfx) {
    return 'AT+CEREG\r\n+CEREG: ' + sfx + '\r\nOK\r\n'
}

var cmd_cgerep_s = function (pfx, sfx) {
    return 'AT+CGEREP=' + sfx + '\r\nOK\r\n'
}

var cmd_cgdcont_q = function (pfx, sfx) {
    return 'AT+CGDCONT?\r\n+CGDCONT: 1,"IP","internet","0.0.0.0",0,0,0,0\r\nOK\r\n'
}

var cmd_cgatt_q = function (pfx, sfx) {
    return 'AT+CGATT?\r\n+CGATT: 1\r\nOK\r\n'
}

var cmd_cgact_q = function (pfx, sfx) {
    return 'AT+CGACT?\r\n+CGACT: 1\r\nOK\r\n'
}

var cmd_cgact_s = function (pfx, sfx) {
    return 'AT+CGACT\r\n+CGACT: ' + sfx + '\r\nOK\r\n'
}

var cmd_qopscfg_q = function (pfx, sfx) {
    return 'AT+QOPSCFG\r\n+QOPSCFG: ' + sfx + ',1\r\nOK\r\n'
}

var cmd_qopscfg_s = function (pfx, sfx) {
    return 'AT+QOPSCFG\r\n+QOPSCFG: ' + sfx + '\r\nOK\r\n'
}

var cmd_qopscfg_q = function (pfx, sfx) {
    return 'AT+QCFG\r\n+QCFG: ' + sfx + ',1\r\nOK\r\n'
}

var cmd_qopscfg_s = function (pfx, sfx) {
    return 'AT+QCFG\r\n+QCFG: ' + sfx + '\r\nOK\r\n'
}

var cmd_qlts_s = function (pfx, sfx) {
    return 'AT+QLTS=' + sfx + '\r\n+QLTS: "2025/11/11,11:45:02+04,0"\r\nOK\r\n'
}

var cmd_qnetdevstatus_q = function (pfx, sfx) {
    return 'AT+QNETDEVSTATUS\r\n+QNETDEVSTATUS: 1,2,4,1\r\nOK\r\n'
}

var cmd_qnetdevstatus_s = function (pfx, sfx) {
    return 'AT+QNETDEVSTATUS=' + sfx + '\r\nOK\r\n'
}

var cmd_cgcontrdp_q = function (pfx, sfx) {
    return 'AT+CGCONTRDP\r\n+CGCONTRDP: 1,5,internet,127.0.0.10,,127.0.0.1,127.0.0.1\r\nOK\r\n'
}

var cmd_qspn_q = function (pfx, sfx) {
    return 'AT+QSPN\r\n+QSPN: "moonsky","moonsky","moonsky",0,"AA101"\r\nOK\r\n'
}

var cmd_csdh_s = function (pfx, sfx) {
    return 'AT+CSDH=' + sfx + '\r\nOK\r\n'
}

var cmd_cmgf_s = function (pfx, sfx) {
    return 'AT+CMGF=' + sfx + '\r\nOK\r\n'
}

var cmd_cpms_s = function (pfx, sfx) {
    return 'AT+CPMS=' + sfx + '\r\nOK\r\n'
}

var cmd_cnmi_s = function (pfx, sfx) {
    return 'AT+CNMI=' + sfx + '\r\nOK\r\n'
}

var cmd_clvl_s = function (pfx, sfx) {
    return 'AT+CLVL=' + sfx + '\r\nOK\r\n'
}

var cmd_crc_s = function (pfx, sfx) {
    return 'AT+CRC=' + sfx + '\r\nOK\r\n'
}

var cmd_cvhu_s = function (pfx, sfx) {
    return 'AT+CVHU=' + sfx + '\r\nOK\r\n'
}

var cmd_csta_s = function (pfx, sfx) {
    return 'AT+CSTA=' + sfx + '\r\nOK\r\n'
}

var cmd_cscs_s = function (pfx, sfx) {
    return 'AT+CSCS=' + sfx + '\r\nOK\r\n'
}

var cmd_qeng_q = function (pfx, sfx) {
    return 'AT+QENG=' + sfx + '\r\n+QENG: "servingcell","NOCONN","LTE","FDD",262,03,18E6D17,62,6200,20,3,3,C974,-98,-10,-73,15,-\r\nOK\r\n'
}

var cmd_cops_q = function (pfx, sfx) {
    return 'AT+COPS?\r\n+COPS: 0,0,"sky sky",7\r\nOK\r\n'
}

var cmd_qnwinfo_q = function (pfx, sfx) {
    return 'AT+QNWINFO\r\n+QNWINFO: "FDD LTE","AA101","LTE BAND 20",9900\r\nOK\r\n'
}

var cmd_cgml_q = function (pfx, sfx) {
    return 'AT+CMGL\r\n+CMGL: 0\r\nOK\r\n'
}

var cmd_qpowd_s = function (pfx, sfx) {
    return 'RDY\r\nDONE\r\n+CPIN: SIM PIN\r\n'
}

// Dummy
var cmd__q = function (pfx, sfx) {
    return 'OK\r\n'
}

// Use the function through the Map object
function executeCommand(command) {
    var pfx = ""
    var sfx = ""
    var params = parseCommand(command)
    var cmd = params[0]
        
    onMessage("RCV: " + command)

    if (params.length < 1 || cmd.length === 0) {
        onComplete("ERROR: Command not set.")
        return
    } 
    
    if (params.length === 2) {
        sfx = params[1]
    } 
    else if (params.length > 2) {
        for (i = 1; i < params.length; i++) {
            sfx = sfx + ((sfx.length === 0) ? "" : ",") + params[i]
        }
    }

    // --
    //var func
    if ((func = getFunction(cmd))) {
        onSendText(func(pfx, sfx))
    } else {
        onComplete("ERROR: Cannot find entry point: " + cmd)
    }
}

function main() {
    if (dataAvailable === false) {
        onStart("QUECTEL: No data available, skip.")
        return
    }

    // notify VSP App
    //onStart("QUECTEL LTE Auto Responder");

    // Add handler functions to the Map
    addFunction('AT', cmd_at)
    addFunction('ATQ0', cmd_atq_0)
    addFunction('ATZ', cmd_atz)
    addFunction('ATV', cmd_atv_0)
    addFunction('ATV1', cmd_atv_1)
    addFunction('AT0', cmd_at0)
    addFunction('AT1', cmd_at1)
    addFunction('ATI', cmd_ati)
    addFunction('ATE1', cmd_ate_1)
    addFunction('ATX4', cmd_atx4)
    addFunction('ATS0', cmd_ats_0_1)
    addFunction('AT&W0', cmd_atw_0)
    addFunction('AT+CPIN?', cmd_cpin_q)
    addFunction('AT+CPIN', cmd_cpin_s)
    addFunction('AT+CMEE', cmd_cmee_s)
    addFunction('AT+CTZU', cmd_ctzu_s)
    addFunction('AT+CCLK', cmd_cclk_s)
    addFunction('AT+QGPSEND', cmd_qgpsend_q)
    addFunction('AT+CFUN?', cmd_cfun_q)
    addFunction('AT+CFUN', cmd_cfun_q)
    addFunction('AT+QSCLK', cmd_qsclk_q)
    addFunction('AT+QINDCFG', cmd_qindcfg_s)
    addFunction('AT+QSIMSTAT', cmd_qsimstat_s)
    addFunction('AT+QINISTAT', cmd_qinistat_q)
    addFunction('AT+QSIMDET?', cmd_qsimdet_q)
    addFunction('AT+QSIMSTAT?', cmd_qsimstat_q)
    addFunction('AT+QPINC?', cmd_qpinc_q)
    addFunction('AT+QOPSCFG?', cmd_qopscfg_q)
    addFunction('AT+QOPSCFG', cmd_qopscfg_s)
    addFunction('AT+QCFG?', cmd_qopscfg_q)
    addFunction('AT+QCFG', cmd_qopscfg_s)
    addFunction('AT+GMI', cmd_gmi_q)
    addFunction('AT+CGMI', cmd_cgmi_q)
    addFunction('AT+GMM', cmd_gmm_q)
    addFunction('AT+CGMM', cmd_cgmm_q)
    addFunction('AT+GMR', cmd_gmr_q)
    addFunction('AT+CGMR', cmd_cgmr_q)
    addFunction('AT+CSUB', cmd_csub_q)
    addFunction('AT+GSN', cmd_gsn_q)
    addFunction('AT+CGSN', cmd_cgsn_q)
    addFunction('AT+CIMI', cmd_cimi_q)
    addFunction('AT+QCCID', cmd_qccuid_q)
    addFunction('AT+CMEE?', cmd_cmee_q)
    addFunction('AT+CSCS?', cmd_cscs_q)
    addFunction('AT+CSCA?', cmd_csca_q)
    addFunction('AT+CSCA', cmd_csca_s)
    addFunction('AT+CNUM', cmd_cnum_q)
    addFunction('AT+GCAP', cmd_gcap_q)
    addFunction('AT+COPN', cmd_copn_q)
    addFunction('AT+QADC', cmd_qadc_q)
    addFunction('AT+CSQ', cmd_csq_q)
    addFunction('AT+CREG?', cmd_creg_q)
    addFunction('AT+CREG', cmd_creg_s)
    addFunction('AT+CGREG?', cmd_cgreg_q)
    addFunction('AT+CGREG', cmd_cgreg_s)
    addFunction('AT+CEREG?', cmd_cereg_q)
    addFunction('AT+CEREG', cmd_cereg_s)
    addFunction('AT+CGEREP', cmd_cgerep_s)
    addFunction('AT+CGDCONT?', cmd_cgdcont_q)
    addFunction('AT+CGATT?', cmd_cgatt_q)
    addFunction('AT+CGACT?', cmd_cgact_q)
    addFunction('AT+CGACT', cmd_cgact_s)
    addFunction('AT+QLTS', cmd_qlts_s)
    addFunction('AT+QNETDEVSTATUS?', cmd_qnetdevstatus_q)
    addFunction('AT+QNETDEVSTATUS', cmd_qnetdevstatus_s)
    addFunction('AT+CGCONTRDP', cmd_cgcontrdp_q)
    addFunction('AT+QSPN', cmd_qspn_q)
    addFunction('AT+CSDH', cmd_csdh_s)
    addFunction('AT+CMGF', cmd_cmgf_s)
    addFunction('AT+CPMS', cmd_cpms_s)
    addFunction('AT+CNMI', cmd_cnmi_s)
    addFunction('AT+CLVL', cmd_clvl_s)
    addFunction('AT+CRC', cmd_crc_s)
    addFunction('AT+CVHU', cmd_cvhu_s)
    addFunction('AT+CSTA', cmd_csta_s)
    addFunction('AT+CSCS', cmd_cscs_s)
    addFunction('AT+QENG', cmd_qeng_q)
    addFunction('AT+COPS?', cmd_cops_q)
    addFunction('AT+QNWINFO', cmd_qnwinfo_q)
    addFunction('AT+CMGL', cmd_cgml_q)
    addFunction('AT+QPOWD', cmd_qpowd_s)

    /*
    addFunction('AT+', cmd__q)
    addFunction('AT+', cmd__q)
    addFunction('AT+', cmd__q)
    addFunction('AT+', cmd__q)
    addFunction('AT+', cmd__q)
    addFunction('AT+', cmd__q)
    addFunction('AT+', cmd__q)
    addFunction('AT+', cmd__q)
    addFunction('AT+', cmd__q)
    */

    // Using the value of the global property
    // named "receivedData". The value type
    // is a byte array. [UInt8]
    var lines = toStringList(receivedData)

    // Check if lines is an array
    if (!Array.isArray(lines)) {
        throw new Error('E:Input must be an array of strings')
    }
    if (lines.length === 0) {
        throw new Error('I:Empty resource, skip')
    }

    // Loop through each line in the array
    for (var i = 0; i < lines.length; i++) {

        // Check if current item is a string
        if (typeof lines[i] !== 'string') {
            console.warn('W:Skipping non-string line at index ' + i)
            continue
        }

        // Get the current line
        var currentLine = lines[i]

        // Execute the command function with the
        // current line as parameter
        if (currentLine.length <= 0) {
            throw new Error('W:Empty command line, skip')
        }

        executeCommand(currentLine)
    }

    // notify VSP App
    //onComplete("Done.");
}

// --[Main]--
try {
    main()
} catch (error) {
    onComplete(error)
}
