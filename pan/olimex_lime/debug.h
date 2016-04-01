#ifndef DEBUG_H
#define DEBUG_H

#define DEBUG_NET
#define DEBUG_LINK
#define DEBUG_PHY
#define DEBUG_LOG
#define DEBUG_GLOBAL

//debug pre vsetko ostatne 
#ifdef DEBUG_GLOBAL
 #define D_G if(1) 
#else
 #define D_G if(0) 
#endif

//debug pre logovanie
#ifdef DEBUG_LOG
 #define D_LOG if(1) 
#else
 #define D_LOG if(0) 
#endif

//debug pre sietovu vrstvu
#ifdef DEBUG_NET
 #define D_NET if(1) 
#else
 #define D_NET if(0) 
#endif

//debug pre linkovu vrstvu
#ifdef DEBUG_LINK
 #define D_LINK if(1) 
#else
 #define D_LINK if(0) 
#endif

//debug pre fyzicku vrstvu 
#ifdef DEBUG_PHY
 #define D_PHY if(1) 
#else
 #define D_PHY if(0) 
#endif

#endif  /* DEBUG_H */

