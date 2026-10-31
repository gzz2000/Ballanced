#!/bin/zsh
set -euo pipefail

project_root="${0:A:h:h:h}"
package_dir="${0:A:h}"
stage="${BALLANCED_STAGE:-$project_root/build/macos-arm64-runtime/stage}"
bml_root="${BML_PLUS_ROOT:-$project_root/../BallanceModLoaderPlus}"
bml_library="${BML_PLUS_LIBRARY:-$bml_root/build-macos-arm64-native/lib/libBMLPlus.dylib}"
bml_defaults="${BML_PLUS_DEFAULTS:-$bml_root/packaging/runtime/ModLoader}"
practice_tools="${BML_PRACTICE_TOOLS:-$bml_root/build-macos-arm64-native/mods/PracticeTools/PracticeTools.bmodp}"
speedrun_shortcuts="${BML_SPEEDRUN_SHORTCUTS:-$bml_root/build-macos-arm64-native/mods/SpeedrunShortcuts/SpeedrunShortcuts.bmodp}"
tas_mod="${BALLANCE_TAS_MOD:-$project_root/../BallanceTAS/build-macos-arm64/src/libBallanceTAS.bmodp}"
legacy_runtime="${BML_LEGACY_TAS_RUNTIME:-$bml_root/build-legacy-native/LegacyTASRuntime.bmodp}"
build_dir="$project_root/build"
dist_dir="$project_root/dist"
output="${1:-$dist_dir/Ballance.app}"

[[ -x "$stage/Bin/Player" ]] || { print -u2 "Missing native Player: $stage/Bin/Player"; exit 1; }
[[ -f "$bml_library" ]] || { print -u2 "Missing native BML+: $bml_library"; exit 1; }
[[ -f "$practice_tools" ]] || { print -u2 "Missing PracticeTools mod: $practice_tools"; exit 1; }
[[ -f "$speedrun_shortcuts" ]] || { print -u2 "Missing SpeedrunShortcuts mod: $speedrun_shortcuts"; exit 1; }
[[ -f "$tas_mod" ]] || { print -u2 "Missing BallanceTAS mod: $tas_mod"; exit 1; }
[[ -f "$legacy_runtime" ]] || { print -u2 "Missing LegacyTASRuntime mod: $legacy_runtime"; exit 1; }
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

# Database.tdb is mutable score data at runtime. Keep only a factory seed in
# Defaults so no code path can accidentally update the signed app resources.
if [[ -f "$game/Database.tdb" ]]; then
  mv "$game/Database.tdb" "$resources/Defaults/Database.tdb"
fi

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
mkdir -p "$resources/Defaults/ModLoader/Mods"
cp "$practice_tools" "$resources/Defaults/ModLoader/Mods/PracticeTools.bmodp"
cp "$speedrun_shortcuts" "$resources/Defaults/ModLoader/Mods/SpeedrunShortcuts.bmodp"
cp "$tas_mod" "$resources/Defaults/ModLoader/Mods/BallanceTAS.bmodp"
cp "$legacy_runtime" "$resources/Defaults/ModLoader/Mods/LegacyTASRuntime.bmodp"

cp "$package_dir/Info.plist" "$contents/Info.plist"
cp "$package_dir/Ballance" "$contents/MacOS/Ballance"
cp "$package_dir/Resources/BallanceNative.icns" "$resources/BallanceNative.icns"
chmod +x "$contents/MacOS/Ballance"
/usr/bin/clang++ -std=c++17 -Os -arch arm64 -mmacosx-version-min=15.0 \
  "$package_dir/MigrateUserData.cpp" -o "$contents/MacOS/MigrateUserData"
codesign --force --sign - "$contents/MacOS/MigrateUserData"

homebrew_sdl='/opt/homebrew/opt/sdl3/lib/libSDL3.0.dylib'
while IFS= read -r -d '' binary; do
  file "$binary" | grep -q 'Mach-O' || continue

  if otool -L "$binary" | awk '{print $1}' | grep -qx "$homebrew_sdl"; then
    install_name_tool -change "$homebrew_sdl" '@rpath/libSDL3.0.dylib' "$binary"
  fi

  while IFS= read -r rpath; do
    [[ -n "$rpath" ]] || continue
    case "$rpath" in
      /*)
        install_name_tool -delete_rpath "$rpath" "$binary"
        ;;
    esac
  done < <(otool -l "$binary" | awk '$1 == "cmd" && $2 == "LC_RPATH" { getline; getline; print $2 }')

  current_rpaths="$(otool -l "$binary" | awk '$1 == "cmd" && $2 == "LC_RPATH" { getline; getline; print $2 }')"
  grep -qx '@loader_path' <<<"$current_rpaths" || install_name_tool -add_rpath '@loader_path' "$binary"
  grep -qx '@loader_path/../Bin' <<<"$current_rpaths" || install_name_tool -add_rpath '@loader_path/../Bin' "$binary"
  # Mods are copied to Application Support at launch. Resolve their native
  # dependencies from the running Player's bundle rather than a build tree.
  for rpath in '@executable_path' '@executable_path/../Plugins' '@executable_path/../BuildingBlocks' '@executable_path/../RenderEngines' '@executable_path/../Managers'; do
    grep -qx "$rpath" <<<"$current_rpaths" || install_name_tool -add_rpath "$rpath" "$binary"
  done
  while IFS= read -r dependency; do
    case "$dependency" in
      @*|/System/Library/*|/usr/lib/*|'') ;;
      /*) print -u2 "Unbundled dependency in $binary: $dependency"; exit 1 ;;
    esac
  done < <(otool -L "$binary" | tail -n +2 | awk '{print $1}')
done < <(find "$resources" -type f -print0)

install_name_tool -id '@rpath/libSDL3.0.dylib' "$game/Bin/libSDL3.0.dylib"

xattr -cr "$app"
while IFS= read -r -d '' binary; do
  file "$binary" | grep -q 'Mach-O' || continue
  codesign --force --sign - "$binary"
done < <(find "$resources" -type f -print0)
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
