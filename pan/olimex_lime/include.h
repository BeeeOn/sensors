#ifndef INCLUDE_H
#define INCLUDE_H

//for arm
#ifndef X86

#include "mqtt_iface.h"
#include <mosquittopp.h>
#include <cstdio>
#include <cstring>
#include <iostream>
#include <iomanip>
#include <functional>
#include <chrono>
#include <thread>
#include "net.h"


//pre x86
#else
#include <stdlib.h>
#include "simulator.h"
#include <stdint.h>
#include <global.h>
#include <fitp.h>
#include "mqtt_iface.h"
#include <iostream>

#include <chrono>
#include <thread>
#include "net.h"

#endif

#endif  /* INCLUDE_H */
