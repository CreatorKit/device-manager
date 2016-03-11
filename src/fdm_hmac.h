/**
 * @file
 * HmacSha256
 *
 * @author Imagination Technologies
 *
 * @copyright Copyright (c) 2016, Imagination Technologies Limited
 *
 * All rights reserved.
 *
 * Redistribution and use of the Software in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *
 * 1. The Software (including after any modifications that you make to it) must support the
 *    FlowCloud Web Service API provided by Licensor and accessible at http://ws-uat.flowworld.com
 *    and/or some other location(s) that we specify.
 *
 * 2. Redistributions of source code must retain the above copyright notice, this list of
 *    conditions and the following disclaimer.
 *
 * 3. Redistributions in binary form must reproduce the above copyright notice, this list of
 *    conditions and the following disclaimer in the documentation and/or other materials
 *    provided with the distribution.
 *
 * 4. Neither the name of the copyright holder nor the names of its contributors may be used to
 *    endorse or promote products derived from this Software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND ANY EXPRESS
 * OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL
 * DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER
 * IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT
 * OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef HMAC_256_H
#define HMAC_256_H

#define SHA256_HASH_LENGTH 32

#include <inttypes.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Compute a Hmac using SHA256 of the data provided and write the result into a buffer
 * @param[out] hash   - buffer to store resulting hash
 * @param[in] data    - pointer to data to hash
 * @param[in] dataLen - length of data in buffer
 * @param[in] key     - pointer to key
 * @param[in] keyLen  - length of key
 */
void HmacSha256_ComputeHash(uint8_t hash[SHA256_HASH_LENGTH], const uint8_t * data, int dataLen, const uint8_t * key, int keyLen);

#ifdef __cplusplus
}
#endif

#endif
