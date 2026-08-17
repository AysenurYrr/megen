#include "profile.h"

int main(void)
{
	struct profile profile;

	if (profile_load(&profile, "profile.toml"))
		return 1;

	profile_dump(&profile);
	profile_free(&profile);

	return 0;
}
