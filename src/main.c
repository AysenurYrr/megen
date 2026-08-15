#include <stdio.h>

#include "profile.h"

int main(void)
{
	struct profile profile;

	if (profile_load(&profile, "profile.toml"))
		return 1;

	printf("name:     %s\n", profile.personal.name);
	printf("email:    %s\n", profile.personal.email);
	printf("phone:    %s\n", profile.personal.phone);
	printf("linkedin: %s\n", profile.personal.linkedin);
	printf("github:   %s\n", profile.personal.github);
	printf("website:  %s\n", profile.personal.website);

	profile_free(&profile);

	return 0;
}