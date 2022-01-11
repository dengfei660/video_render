/*
 * Copyright (C) 2007 The Android Open Source Project
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef _TOOLS_ERRORS_CODE_H_
#define _TOOLS_ERRORS_CODE_H_

#include <sys/types.h>
#include <errno.h>

#ifdef  __cplusplus
extern "C" {
#endif


enum ErrorCode {
    OK                = 0,    // Everything's swell.
    NO_ERROR          = 0,    // No errors.

    ERROR_UNKNOWN             = (-2147483647-1), // INT32_MIN value

    ERROR_NO_MEMORY           = -ENOMEM,
    ERROR_INVALID_OPERATION   = -ENOSYS,
    ERROR_BAD_VALUE           = -EINVAL,
    ERROR_BAD_TYPE            = (ERROR_UNKNOWN + 1),
    ERROR_NOT_FOUND           = -ENOENT,
    ERROR_PERMISSION_DENIED   = -EPERM,
    ERROR_NO_INIT             = -ENODEV,
    ERROR_ALREADY_EXISTS      = -EEXIST,
    ERROR_DEAD_OBJECT         = -EPIPE,
    ERROR_FAILED_TRANSACTION  = (ERROR_UNKNOWN + 2),
#if !defined(_WIN32)
    ERROR_BAD_INDEX           = -EOVERFLOW,
    ERROR_NOT_ENOUGH_DATA     = -ENODATA,
    ERROR_WOULD_BLOCK         = -EWOULDBLOCK,
    ERROR_TIMED_OUT           = -ETIMEDOUT,
    ERROR_UNKNOWN_TRANSACTION = -EBADMSG,
#else
    ERROR_BAD_INDEX           = -E2BIG,
    ERROR_NOT_ENOUGH_DATA     = (ERROR_UNKNOWN + 3),
    ERROR_WOULD_BLOCK         = (ERROR_UNKNOWN + 4),
    ERROR_TIMED_OUT           = (ERROR_UNKNOWN + 5),
    ERROR_UNKNOWN_TRANSACTION = (ERROR_UNKNOWN + 6),
#endif
    ERROR_FDS_NOT_ALLOWED     = (ERROR_UNKNOWN + 7),
    ERROR_UNEXPECTED_NULL     = (ERROR_UNKNOWN + 8),
    ERROR_OPEN_FAIL           = (ERROR_UNKNOWN + 9),
    ERROR_PARAM_NULL          = (ERROR_UNKNOWN + 10),
    ERROR_DESTROY_FAIL        = (ERROR_UNKNOWN + 11),
    ERROR_IOCTL_FAIL          = (ERROR_UNKNOWN + 12),
};

#ifdef  __cplusplus
}
#endif

#endif // _TOOLS_ERRORS_CODE_H_
