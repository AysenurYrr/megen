#ifndef MEGEN_HTML_H
#define MEGEN_HTML_H

struct profile;

int static_html_render(const struct profile *profile, const char *path);
int static_html_copy_asset(const char *source_path, const char *output_path);
int static_html_copy_asset_tree(const char *source_path, const char *output_path);

#endif /* MEGEN_HTML_H */
