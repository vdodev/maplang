/*
 * Copyright 2020 VDO Dev Inc <support@maplang.com>
 *
 *  Licensed under the Apache License, Version 2.0 (the "License");
 *  you may not use this file except in compliance with the License.
 *  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 */

#ifndef MAPLANG_INCLUDE_PRIVATE_HTTPUTILITIES_H_
#define MAPLANG_INCLUDE_PRIVATE_HTTPUTILITIES_H_

#include <string>

namespace maplang {
namespace http {

const extern std::string kParameter_HttpVersion;
const extern std::string kParameter_HttpPath;
const extern std::string kParameter_HttpMethod;
const extern std::string kParameter_HttpRequestId;
const extern std::string kParameter_HttpHeaders;
const extern std::string kParameter_HttpStatusCode;
const extern std::string kParameter_HttpStatusReason;

const extern std::string kHttpHeaderNormalized_ContentLength;
const extern std::string kHttpHeaderNormalized_ContentType;

const extern int kHttpStatus_Continue;
const extern int kHttpStatus_Ok;
const extern int kHttpStatus_BadRequest;

std::string getDefaultReasonForHttpStatus(int status);

}  // namespace http
}  // namespace maplang

#endif  // MAPLANG_INCLUDE_PRIVATE_HTTPUTILITIES_H_
