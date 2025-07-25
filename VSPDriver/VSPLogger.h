// ********************************************************************
// VSPDriver - VSPUserClient.cpp
//
// Copyright © 2025 by EoF Software Labs
// Copyright © 2024 Apple Inc. (some copied parts)
// SPDX-License-Identifier: MIT
// ********************************************************************
#ifndef VSPLogger_h
#define VSPLogger_h

#include <os/log.h>
#include <DriverKit/IOLib.h>

#ifndef DEBUG
#define DEBUG
#endif

// This log to makes it easier to parse out individual logs from the driver,
// since all logs will be prefixed with the same word/phrase. DriverKit logging
// has no logging levels; some developers might want to prefix errors differently
// than info messages. Another option is to #define a "DEBUG" case where some
// log messages only exist when doing a debug build.
#ifndef VSPLog
#ifdef DEBUG
#define VSPLog(prefix, fmt, ...) os_log(OS_LOG_DEFAULT, "[" prefix "]: " fmt, ##__VA_ARGS__)
#else
#define VSPLog(prefix, fmt, ...)
#endif
#endif

#define VSPErr(prefix, fmt, ...)   os_log(OS_LOG_DEFAULT, "[" prefix "]: " fmt, ##__VA_ARGS__)
#define VSPTrace(prefix, fmt, ...) os_log(OS_LOG_DEFAULT, "[" prefix "]: " fmt, ##__VA_ARGS__)

#endif /* VSPLogger_h */
