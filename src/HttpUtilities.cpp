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

#include "maplang/HttpUtilities.h"

using namespace std;

namespace maplang {
namespace http {

const string kParameter_HttpVersion = "httpVersion";
const string kParameter_HttpPath = "httpPath";
const string kParameter_HttpMethod = "httpMethod";
const string kParameter_HttpRequestId = "httpRequestId";
const string kParameter_HttpHeaders = "httpHeaders";
const string kParameter_HttpStatusCode = "httpStatusCode";
const string kParameter_HttpStatusReason = "httpStatusReason";

const string kHttpHeaderNormalized_ContentLength = "content-length";
const string kHttpHeaderNormalized_ContentType = "content-type";

const int kHttpStatus_Continue = 100;
const int kHttpStatus_Ok = 200;
const int kHttpStatus_BadRequest = 400;

string getDefaultReasonForHttpStatus(int status) {
  switch (status) {
    case kHttpStatus_Continue:
      return "CONTINUE";
    case kHttpStatus_Ok:
      return "OK";
    case kHttpStatus_BadRequest:
      return "BAD REQUEST";
    default:
      return "UNSPECIFIED";
  }
}

}  // namespace http
}  // namespace maplang
