#!/usr/bin/env bash

set -euo pipefail

source_dir="${1:-web/assets/images/original}"
output_dir="${2:-web/assets/images/converted_webp}"
quality="${3:-82}"

if [[ ! -d "$source_dir" ]]; then
  printf 'error: image directory not found: %s\n' "$source_dir" >&2
  exit 1
fi

mkdir -p "$output_dir"

if [[ ! "$quality" =~ ^[0-9]+$ ]] || ((quality < 0 || quality > 100)); then
  printf 'error: quality must be an integer from 0 to 100\n' >&2
  exit 1
fi

if command -v cwebp >/dev/null 2>&1; then
  converter="cwebp"
elif command -v magick >/dev/null 2>&1; then
  converter="magick"
else
  printf 'error: install cwebp or ImageMagick before running this script\n' >&2
  exit 1
fi

converted=0
skipped=0

while IFS= read -r -d '' source; do
  filename="${source##*/}"
  output="$output_dir/${filename%.*}.webp"

  if [[ -f "$output" && "$output" -nt "$source" ]]; then
    printf 'skip:    %s\n' "$output"
    ((skipped += 1))
    continue
  fi

  printf 'convert: %s -> %s\n' "$source" "$output"
  if [[ "$converter" == "cwebp" ]]; then
    cwebp -quiet -mt -m 6 -q "$quality" "$source" -o "$output"
  else
    magick "$source" -auto-orient -strip -quality "$quality" "$output"
  fi
  ((converted += 1))
done < <(
  find "$source_dir" -maxdepth 1 -type f \
    \( -iname '*.jpg' -o -iname '*.jpeg' -o -iname '*.png' \) \
    -print0
)

printf 'done: %d converted, %d unchanged\n' "$converted" "$skipped"
