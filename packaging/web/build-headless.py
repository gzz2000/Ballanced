#!/usr/bin/env python3
"""Build the Node-hosted WASM qualification runner (no browser I/O yet)."""
import argparse
import pathlib
import subprocess

parser = argparse.ArgumentParser(description=__doc__)
parser.add_argument("--emsdk", type=pathlib.Path, required=True)
parser.add_argument("--sdl-source", type=pathlib.Path, required=True)
parser.add_argument("--build", type=pathlib.Path, required=True)
parser.add_argument("--fex-source", type=pathlib.Path)
parser.add_argument("--bml-source", type=pathlib.Path, required=True)
parser.add_argument("--tas-source", type=pathlib.Path, required=True)
parser.add_argument("--webgpu", action="store_true", help="Also build the interactive WebGPU browser host")
parser.add_argument("--jobs", type=int, default=8)
args = parser.parse_args()
source = pathlib.Path(__file__).resolve().parents[2]
build = args.build.resolve()
build.mkdir(parents=True, exist_ok=True)
emcmake = args.emsdk.resolve() / "upstream/emscripten/emcmake"

def run(*command):
    subprocess.run([str(x) for x in command], check=True)

run(emcmake, "cmake", "-S", args.sdl_source.resolve(), "-B", build / "sdl",
    "-G", "Ninja", "-DCMAKE_BUILD_TYPE=Release", "-DSDL_SHARED=OFF",
    "-DSDL_STATIC=ON", "-DSDL_TEST_LIBRARY=OFF", "-DSDL_TESTS=OFF",
    "-DSDL_EXAMPLES=OFF", f"-DCMAKE_INSTALL_PREFIX={build / 'sdl-install'}")
run("cmake", "--build", build / "sdl", "--parallel", args.jobs)
run("cmake", "--install", build / "sdl")
extra = [f"-DBALLANCE_BML_SOURCE_DIR={args.bml_source.resolve()}",
         f"-DBALLANCE_TAS_SOURCE_DIR={args.tas_source.resolve()}"]
if args.fex_source:
    extra.append(f"-DFETCHCONTENT_SOURCE_DIR_FEX_X87={args.fex_source.resolve()}")
extra.append(f"-DCKRE_BUILD_WEBGPU_RASTERIZER={'ON' if args.webgpu else 'OFF'}")
run(emcmake, "cmake", "-S", source, "-B", build / "engine", "-G", "Ninja",
    "-C", source / "cmake/WebHeadless.cmake",
    f"-DSDL3_DIR={build / 'sdl-install/lib/cmake/SDL3'}", *extra)
run("cmake", "--build", build / "engine", "--parallel", args.jobs)
if args.webgpu:
    run("cmake", "--build", build / "engine", "--target", "PlayerBrowser", "--parallel", args.jobs)
run("ctest", "--test-dir", build / "engine", "-R",
    "^ivp_|^PlayerSdlShortcutsTest$|^NativeHookAbiTest$|^NativeRenderSkipTest$|^StaticModRegistryTest$|^CKStaticPluginTest$",
    "--output-on-failure", "--parallel", args.jobs)
