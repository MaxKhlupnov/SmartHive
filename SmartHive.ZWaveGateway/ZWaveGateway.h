#pragma once
#ifndef ZWAVEDEVICE_DEVICE_CLOUD_UPLOAD_CONFIG_SERVICE_H
#define ZWAVEDEVICE_DEVICE_CLOUD_UPLOAD_CONFIG_SERVICE_H

#ifdef __cplusplus
extern "C"
{
#endif

	extern int configureAsAService(void);

	extern void waitForUserInput(void);

#ifdef __cplusplus
}
#endif

#endif