#ifndef MRF_PHY_LAYER_H 
#define MRF_PHY_LAYER_H 

#include <stdint.h>
#include <stdbool.h>

#include "constants.h"

#define FXTAL 12800


/**
 * @def MAX_PHY_PAYLOAD_SIZE
 * @brief maximal packet size on phy layer, it is radio dependent for mrf89 it 63
 */
#define MAX_PHY_PAYLOAD_SIZE 63

/**
 * @def bands
 * @brief enum with valid bands
 */
enum bands { 
             BAND_863 = 0,
             BAND_863_C950 = 1, 
             BAND_902 = 2, 
             BAND_915 = 3 
           };

           
/**
 * @def PHY_init_t
 * @brief init params structure
 */
typedef struct{
  uint8_t channel;           
  uint8_t band;              
  uint8_t bitrate;
  uint8_t power;
  uint8_t cca_noise_treshold; //!< maximal value of noise when device trying to send data
}PHY_init_t;


/**
 * @def PHY_init
 * @brief initialize physical layer
 * set band, channel, bitrate, power to radio
 * set maximal treshold of noise(cca) when radio can send data
 * @param params PHY_init_t values whitch be set on radio
 * @return void
 */
void PHY_init (PHY_init_t params);

/**
 * @def PHY_stop
 * @brief deactivate radio 
 * not implemented on devices without OS (protocol uses threads on linux)
 * @see implementation for pan coordinator
 * @return void
 */
void PHY_stop ();

/**
 * @def PHY_set_freq
 * @brief set frequenci used by radio 
 * @param band uint8_t band to set, value must be from enum bands
 * @see bands
 * @return boolean true if band is valid
 */
bool PHY_set_freq (uint8_t band);

/**
 * @def PHY_set_channel
 * @brief set channel used by radio 
 * @param channel uint8_t channel to set, value may be between 0 and 32 (for some bands and speeds only 25 allowed) 
 * @return boolean true if channel is valid
 */
bool PHY_set_channel (uint8_t channel);

/**
 * @def PHY_set_bitrate
 * @brief set bitrate used by radio 
 * @param bitrate uint8_t bitrate to set
 * @return boolean true if bitrate is valid
 */
bool PHY_set_bitrate (uint8_t bitrate);

/**
 * @def PHY_set_power
 * @brief set power used by radio 
 * @param power uint8_t power to set, aviable values from 0 to 8
 * 
 * can be used as macro from constants.h format TX_POWER_${value}_DB
 * power      hex   macro
 * 13 db      0x00  TX_POWER_13_DB
 * 10 db      0x01  TX_POWER_10_DB
 *  7 db      0x02  TX_POWER_7_DB
 *  4 db      0x03  TX_POWER_4_DB
 *  1 db      0x04  TX_POWER_1_DB
 * -2 db      0x05  TX_POWER_N_2_DB
 * -5 db      0x06  TX_POWER_N_5_DB
 * -8 db      0x07  TX_POWER_N_8_DB
 * @return true if power is valid
 */
bool    PHY_set_power  (uint8_t power);

/**
 * @def PHY_get_noise
 * @brief read actual value of noise from radio
 * @return uint8_t noise value
 */
uint8_t PHY_get_noise ();


/**
 * @def PHY_send
 * @brief send raw data to air
 * @param data uint8_t* data to be send
 * @param len  uint8_t data lenght
 * @return void
 */
void PHY_send (const char* data, uint8_t len);

/**
 * @def PHY_send_with_cca
 * @brief send raw data to air wait to cca noise under specified level
 * @param data uint8_t* data to be send
 * @param len  uint8_t data lenght
 * @see PHY_init_t.cca_noise_treshold
 * @see PHY_send
 * @return void
 */
void PHY_send_with_cca (uint8_t* data,uint8_t len);

/**
 * @def PHY_process_packet
 * @brief extern function called after data recived on radio
 * @warning function is runing inside interupt(in this microcontroler implementation) 
 * @param data uint8_t* recived data
 * @param len  uint8_t data lenght
 * @return void
 */
extern void PHY_process_packet(uint8_t* data, uint8_t len);

/**
 * @def PHY_timer_interupt
 * @brief extern function called after timer interupt
 * @return void
 */
extern void PHY_timer_interupt (void);

void    set_rf_mode (uint8_t mode);

#endif
