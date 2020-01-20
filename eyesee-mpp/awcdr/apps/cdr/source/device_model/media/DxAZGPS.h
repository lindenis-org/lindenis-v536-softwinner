#ifndef _DXAZGPS_H_
#define _DXAZGPS_H_

#ifdef __cplusplus
extern "C" {
#endif

extern double encryptLatitude(double latitude,int gpsSecond,char gpsID[20]);
extern double encryptLongitude(double longitude,int gpsMinute,char gpsID[20]);
extern char * DecryptGPSID(char gID[20]);

#ifdef __cplusplus
}
#endif

#endif //_DXAZGPS_H_
