#include "[require].h"

bool Reflex::System::IsDebuggerPresent()
{
	FILE * file = fopen("/proc/self/status", "r");

	if (!file) return false;

	char line[256];
	bool attached = false;

	while (fgets(line, sizeof(line), file))
	{
		if (strncmp(line, "TracerPid:", 10) == 0)
		{
			attached = atoi(line + 10) != 0;
			break;
		}
	}

	fclose(file);

	return attached;
}

void Reflex::System::DisableCrashReporter()
{
}