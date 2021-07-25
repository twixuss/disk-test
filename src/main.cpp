#include <Windows.h>
#include <stdio.h>

#define BLOCK_SIZE 1024

#define ALL_ERRORS(E) \
E(Error_written_byte_count_does_not_match)

enum Error {
	Error_none,
#define E(x) x,
	ALL_ERRORS(E)
#undef E
};

bool volatile running = true;

unsigned volatile error_id;

unsigned volatile written_block_count;

char const buffer[BLOCK_SIZE] = {};

char const *unit_strings[] = {
	"bytes",
	"kilobytes",
	"megabytes",
	"gigabytes",
};

BOOL WINAPI control_handler(DWORD type) {
	switch (type) {
		case CTRL_C_EVENT: {
			running = false;
			return TRUE;
		}
	}
	return FALSE;
}

int main() {
	SetConsoleCtrlHandler(control_handler, TRUE);

	HANDLE thread = CreateThread(0, 0, [](void *) -> DWORD {
		char const *const filename = "disk_test";
		HANDLE file = CreateFileA(filename, GENERIC_WRITE, 0, 0, CREATE_ALWAYS, 0, 0);
		while (running) {
			SetFilePointer(file, 0, 0, SEEK_SET);
			DWORD bytes_written;
			WriteFile(file, buffer, sizeof(buffer), &bytes_written, 0);
			if (bytes_written != sizeof(buffer)) {
				error_id = Error_written_byte_count_does_not_match;
			}
			InterlockedIncrement(&written_block_count);
		}
		CloseHandle(file);
		DeleteFileA(filename);
		return 0;
	}, 0, 0, 0);

	while (running) {
		float const update_delta = 1.0f;
		Sleep((DWORD)(update_delta * 1000));
		unsigned blocks = InterlockedExchange(&written_block_count, 0);
		float unit_count = (float)(blocks * BLOCK_SIZE) / update_delta;
		char const **unit_string = unit_strings;
		while (unit_count >= 1000) {
			unit_count /= 1000;
			unit_string += 1;
		}
		printf("\r%.3f %s per second", unit_count, *unit_string);
	}

	WaitForSingleObject(thread, INFINITE);

	switch (error_id) {
#define E(x) case x: printf(#x); break;
		ALL_ERRORS(E)
#undef E
	}
}