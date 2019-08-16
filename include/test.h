/**
 * @file test.h
 * @author SWH
 * @brief Test modules
 * @version 0.1
 * @date 2019-08-08
 *
 * @copyright Copyright (c) 2019
 *
 */
#ifndef __TEST_H__
#define __TEST_H__
#include <cstdint>

#include "rtp.h"
#include "udp.h"

void RtpSenderLoop(const char* ip, uint16_t remote_port, uint16_t local_port,
                   int packet_num, int frame_pre_packet);
void RtpRecvLoop(char* ip, uint16_t remote_port, uint16_t local_port,
                 bool& work);

#endif  /// __TEST_H__