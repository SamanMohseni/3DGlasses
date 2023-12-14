#include <Windows.h>
#include <iostream>
#include <cstdlib>
#include <stdint.h>

#include "ftd2xx.h"

using namespace std;

#define TransmitBufferSize 1
#define ReceiveBufferSize 1

class USB {
public:
	USB(){
		status = FT_OpenEx((PVOID) "Digilent USB Device B", FT_OPEN_BY_DESCRIPTION, &handle);
		if (status != FT_OK) {
			cout << "Failed to open the Device!" << endl;
			exit(1);
		}
		else {
			cout << "Device Opened Successfully" << endl;
		}

		status = FT_SetUSBParameters(handle, ReceiveBufferSize, TransmitBufferSize);
		FT_Purge(handle, FT_PURGE_RX);
		status = FT_SetFlowControl(handle, FT_FLOW_RTS_CTS, 0, 0);
	}

	void sendData(unsigned char byte){
		transmitBuffer[0] = byte;
		status = FT_Write(handle, transmitBuffer, TransmitBufferSize, &bytesWritten);
	}

	~USB(){
		status = FT_Close(handle);
	}

private:
	FT_HANDLE handle;
	FT_STATUS status;

	DWORD bytesWritten;
	DWORD bytesRead;

	unsigned char receiveBuffer[ReceiveBufferSize];
	unsigned char transmitBuffer[TransmitBufferSize];

};