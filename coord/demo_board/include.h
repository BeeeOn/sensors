#ifndef INCLUDE_H
#define INCLUDE_H



//pre xc8
#ifndef X86
#include <xc.h>
#include <stdint.h>
#include <stdbool.h>

#include <stdint.h>
#include <stdbool.h>
#include <plib.h>

#include "log.h"
#include "phy.h"
#include "link.h"
#include "net.h"
#include "global.h"
#include "fitp.h"

#include <stdio.h>

//pre x86
#else
#include "simulator.h"
#include <string.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdarg.h>
#include "global.h"

#include "log.h"
#include "fitp.h"
#include "log.h"
#include "phy.h"
#include "link.h"
#include "net.h"
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
