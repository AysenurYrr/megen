#include <stdio.h>

#include "profile.h"

int main(void)
{
	struct profile profile;
	size_t i;

	if (profile_load(&profile, "profile.toml"))
		return 1;

	printf("name:     %s\n", profile.personal.name);
	printf("email:    %s\n", profile.personal.email);
	printf("phone:    %s\n", profile.personal.phone);
	printf("linkedin: %s\n", profile.personal.linkedin);
	printf("github:   %s\n", profile.personal.github);
	printf("website:  %s\n", profile.personal.website);

	for (i = 0; i < profile.education_count; i++) {
		const struct education *education = &profile.education[i];

		printf("education: %s, %s (%04u-%02u - ",
		       education->institution, education->department,
		       education->period.start.year, education->period.start.month);
		if (education->period.ongoing)
			printf("present)\n");
		else
			printf("%04u-%02u)\n", education->period.end.year,
			       education->period.end.month);
	}

	for (i = 0; i < profile.award_count; i++)
		printf("award: %s, %s (%04u-%02u)\n",
		       profile.awards[i].title, profile.awards[i].issuer,
		       profile.awards[i].date.year, profile.awards[i].date.month);

	for (i = 0; i < profile.certificate_count; i++)
		printf("certificate: %s, %s (%04u-%02u)\n",
		       profile.certificates[i].title,
		       profile.certificates[i].issuer,
		       profile.certificates[i].date.year,
		       profile.certificates[i].date.month);

	profile_free(&profile);

	return 0;
}
