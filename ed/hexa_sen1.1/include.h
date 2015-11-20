#ifndef INCLUDE_H
#define INCLUDE_H



//pre xc8
#ifndef X86
#include <xc.h>
#include <plib.h>
#include <string.h>
#include <rtcc.h>
#include <stdbool.h>
#include <stdint.h>
#include <delays.h>
#include <stdio.h>
#include <usart.h>
#include <stdarg.h>
#include <pps.h>

#include "global.h"
#include "log.h"
#include "fitp.h"

#include "log.h"
#include "phy.h"
#include "link.h"

#include "ds18b20.h"
#include "hw.h"
#include "senact.h"
#include "sht21lib.h"
#include "spieeprom.h"
#include "vibra.h"
#include "edid_config.h"

//pre x86
#else

#include "simulator.h"
#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdarg.h>
#include "global.h"
#include "hw.h"

#include "log.h"
#include "fitp.h"
#include "log.h"
#include "phy.h"
#include "link.h"
#include <unistd.h>

#include "mqtt_iface.h"
#include <mosquittopp.h>
#include <stdio.h>
#include <cstring>
#include <unistd.h> //get PID
#include <global.h>

#include <thread>
#include <chrono>

#include "phy.h"
#include "simulator.h"

#endif

#endif  /* INCLUDE_H */

