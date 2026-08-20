#ifndef MEGEN_LATEX_H
#define MEGEN_LATEX_H

struct profile;

int latex_render(const struct profile *profile, const char *path);
int latex_compile_pdf(const char *tex_path);

#endif /* MEGEN_LATEX_H */
