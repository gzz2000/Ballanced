#!/bin/zsh
set -euo pipefail

project_root="${0:A:h:h:h}"
package_dir="${0:A:h}"
stage="${BALLANCED_STAGE:-$project_root/build/macos-arm64-runtime/stage}"
bml_root="${BML_PLUS_ROOT:-$project_root/../BallanceModLoaderPlus}"
bml_library="${BML_PLUS_LIBRARY:-$bml_root/build-macos-arm64-native/lib/libBMLPlus.dylib}"
bml_defaults="${BML_PLUS_DEFAULTS:-$bml_root/packaging/runtime/ModLoader}"
build_dir="$project_root/build"
dist_dir="$project_root/dist"
output="${1:-$dist_dir/Ballance.app}"

[[ -x "$stage/Bin/Player" ]] || { print -u2 "Missing native Player: $stage/Bin/Player"; exit 1; }
[[ -f "$bml_library" ]] || { print -u2 "Missing native BML+: $bml_library"; exit 1; }
[[ -f "$stage/Bin/libSDL3.0.dylib" ]] || { print -u2 "Missing bundled SDL3"; exit 1; }
[[ -f "$package_dir/Resources/BallanceNative.icns" ]] || { print -u2 "Missing app icon"; exit 1; }

mkdir -p "$build_dir" "$dist_dir" "${output:h}"
work_dir="$(mktemp -d "$build_dir/.ballance-native.XXXXXX")"
app="$work_dir/Ballance.app"
contents="$app/Contents"
resources="$contents/Resources"
game="$resources/Game"

mkdir -p "$contents/MacOS" "$resources/Defaults/ModLoader"
ditto "$stage" "$game"
mkdir -p "$game/Plugins"
cp "$bml_library" "$game/Plugins/libBMLPlus.dylib"

# Diagnostic, SDK, and build-only artifacts are not runtime content.
rm -rf "$game/Plugins.disabled" "$game/include" "$game/lib"
rm -f "$game/Bin/Player.log"
find "$game" -name '.DS_Store' -delete

if [[ -d "$bml_defaults/Fonts" ]]; then
  ditto "$bml_defaults/Fonts" "$resources/Defaults/ModLoader/Fonts"
fi
if [[ -d "$bml_defaults/Themes" ]]; then
  ditto "$bml_defaults/Themes" "$resources/Defaults/ModLoader/Themes"
fi

cp "$package_dir/Info.plist" "$contents/Info.plist"
cp "$package_dir/Ballance" "$contents/MacOS/Ballance"
cp "$package_dir/Resources/BallanceNative.icns" "$resources/BallanceNative.icns"
chmod +x "$contents/MacOS/Ballance"

homebrew_sdl='/opt/homebrew/opt/sdl3/lib/libSDL3.0.dylib'
while IFS= read -r -d '' binary; do
  file "$binary" | grep -q 'Mach-O' || continue

  if otool -L "$binary" | awk '{print $1}' | grep -qx "$homebrew_sdl"; then
    install_name_tool -change "$homebrew_sdl" '@rpath/libSDL3.0.dylib' "$binary"
  fi

  while IFS= read -r rpath; do
    [[ -n "$rpath" ]] || continue
    case "$rpath" in
      /opt/homebrew/*|/usr/local/*|"$project_root"/*|"$bml_root"/*)
        install_name_tool -delete_rpath "$rpath" "$binary"
        ;;
    esac
  done < <(otool -l "$binary" | awk '$1 == "cmd" && $2 == "LC_RPATH" { getline; getline; print $2 }')

  current_rpaths="$(otool -l "$binary" | awk '$1 == "cmd" && $2 == "LC_RPATH" { getline; getline; print $2 }')"
  grep -qx '@loader_path' <<<"$current_rpaths" || install_name_tool -add_rpath '@loader_path' "$binary"
  grep -qx '@loader_path/../Bin' <<<"$current_rpaths" || install_name_tool -add_rpath '@loader_path/../Bin' "$binary"
done < <(find "$game" -type f -print0)

install_name_tool -id '@rpath/libSDL3.0.dylib' "$game/Bin/libSDL3.0.dylib"

xattr -cr "$app"
while IFS= read -r -d '' binary; do
  file "$binary" | grep -q 'Mach-O' || continue
  codesign --force --sign - "$binary"
done < <(find "$game" -type f -print0)
codesign --force --deep --sign - "$app"
codesign --verify --deep --strict --verbose=2 "$app"

if [[ -e "$output" ]]; then
  backup="$build_dir/Ballance-pre-native-$(date +%Y%m%d-%H%M%S).app"
  mv "$output" "$backup"
  print "Previous bundle moved to: $backup"
fi
mv "$app" "$output"

print "Built native Apple Silicon app: $output"
du -sh "$output"
