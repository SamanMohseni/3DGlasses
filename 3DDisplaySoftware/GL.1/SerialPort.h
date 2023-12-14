#include <stdio.h>
#include <conio.h>
#include <string.h>

#define STRICT
#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <iostream>
using namespace std;

class SerialPort{
public:
	SerialPort(){
		port_name = L"\\\\.\\COM4";
		init = "";

		file = CreateFile(port_name, GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
		if (INVALID_HANDLE_VALUE == file) {
			cout << "ERROR!" << endl;
		}

		memset(&port, 0, sizeof(port));
		port.DCBlength = sizeof(port);
		if (!GetCommState(file, &port)){
			cout << "ERROR!" << endl;
		}
		if (!BuildCommDCB(L"baud=19200 parity=n data=8 stop=1", &port)){
			cout << "ERROR!" << endl;
		}
		if (!SetCommState(file, &port)){
			cout << "ERROR!" << endl;
		}

		timeouts.ReadIntervalTimeout = 1;
		timeouts.ReadTotalTimeoutMultiplier = 1;
		timeouts.ReadTotalTimeoutConstant = 1;
		timeouts.WriteTotalTimeoutMultiplier = 1;
		timeouts.WriteTotalTimeoutConstant = 1;
		if (!SetCommTimeouts(file, &timeouts)){
			cout << "ERROR!" << endl;
		}

		if (!EscapeCommFunction(file, CLRDTR)){
			cout << "ERROR!" << endl;
		}
		Sleep(200);
		if (!EscapeCommFunction(file, SETDTR)){
			cout << "ERROR!" << endl;
		}

		if (!WriteFile(file, init, sizeof(init), &written, NULL)){
			cout << "ERROR!" << endl;
		}

		if (written != sizeof(init)){
			cout << "ERROR!" << endl;
		}
	}
	~SerialPort(){
		CloseHandle(file);
	}
	void Write(char _write_value){
		write_value = _write_value;
		WriteFile(file, &write_value, 1, &written, NULL);
	}
	char Read(){
		do{
			ReadFile(file, read_value, sizeof(read_value), &read, NULL);
			Sleep(1);
		} while (!read);
		return read_value[0];
	}
	short ReadWord(){
		char msb = Read();
		char lsb = Read();
		short res = msb;
		res = res << 8;
		res += lsb;
		return res;
	}
private:
	HANDLE file;
	COMMTIMEOUTS timeouts;
	DWORD read, written;
	DCB port;
	DWORD mode;
	LPCWSTR port_name;
	char *init;
	char write_value;
	char read_value[1];
	bool dont_care;
};