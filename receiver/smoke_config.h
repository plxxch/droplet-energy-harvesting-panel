/*
 * smoke_config.h
 *
 * Packet-length selection for the smoke/temperature receiver.
 * The CCS build configuration defines RF_PACKET_LEN:
 *   3 -> temperature + smoke alarm
 *   5 -> temperature + raw MAX30105 value
 */

#ifndef SMOKE_CONFIG_H
#define SMOKE_CONFIG_H

#ifndef RF_PACKET_LEN
#define RF_PACKET_LEN 5
#endif

#endif /* SMOKE_CONFIG_H */
