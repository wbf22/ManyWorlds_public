#!/usr/bin/env python
import os
import shutil
import sys

SRC='src'

env = SConscript("godot-cpp/SConstruct")
env['OBJPREFIX'] = 'build/'
env.Append(CCFLAGS=['-O0', '-g', '-gdwarf-4', '-w', '-fno-eliminate-unused-debug-types']) # debug symbols I think and also no warnings with -w
# force the GNU bfd linker on Linux (gold/lld choke on the big dwarf-4 debug link);
# a no-op everywhere else, so only apply where it matters
if env['PLATFORM'] == 'linux':
    env.Append(LINKFLAGS=['-fuse-ld=bfd'])
env['CXXFLAGS'] += ['-std=c++20']
env['CXXFLAGS'] += ['-fexceptions']
# For reference:
# - CCFLAGS are compilation flags shared between C and C++
# - CFLAGS are for C-specific compilation flags
# - CXXFLAGS are for C++-specific compilation flags
# - CPPFLAGS are for pre-processor flags
# - CPPDEFINES are for pre-processor defines
# - LINKFLAGS are for linking flags

env.Append(CPPPATH=[f"{SRC}/"])

# Mark the extension build so headers can conditionally include Godot types.
# The test build clones this env and strips the define (see below).
env.Append(CPPDEFINES=['GODOT_ENABLED'])

# ─── Godot extension sources (everything in src/ except TEST/) ───────────────
sources = []
for root, dirs, files in os.walk(SRC):
    # skip the TEST folder so test.cpp doesn't end up in the extension
    dirs[:] = [d for d in dirs if d != 'TEST']
    for file in files:
        if file.endswith('.cpp') or file.endswith('.c'):
            sources.append(os.path.join(root, file))



# ─── Godot extension library ─────────────────────────────────────────────────

if env["platform"] == "macos":
    library = env.SharedLibrary(
        "demo/bin/libgdexample.{}.{}.framework/libgdexample.{}.{}".format(
            env["platform"], env["target"], env["platform"], env["target"]
        ),
        source=sources,
    )
elif env["platform"] == "ios":
    if env["ios_simulator"]:
        library = env.StaticLibrary(
            "demo/bin/libgdexample.{}.{}.simulator.a".format(env["platform"], env["target"]),
            source=sources,
        )
    else:
        library = env.StaticLibrary(
            "demo/bin/libgdexample.{}.{}.a".format(env["platform"], env["target"]),
            source=sources,
        )
else:
    library = env.SharedLibrary(
        "bin/libgdexample{}{}".format(env["suffix"], env["SHLIBSUFFIX"]),
        source=sources,
    )

Default(library) # by defulat tells scons to only build the godot extension



# ─── Standalone test executable ──────────────────────────────────────────────

test_env = env.Clone() # Clone the env so we can safely change settings (e.g. no shared-lib flags)
# Remove linker stripping so debug symbols survive in the test binary.
test_env['LINKFLAGS'] = [f for f in test_env.get('LINKFLAGS', []) if f not in ('-s', '-Wl,-S', '-Wl,-x')]
# Remove the Godot-specific define so the test binary can run without the engine.
test_env['CPPDEFINES'] = [d for d in test_env.get('CPPDEFINES', []) if d != 'GODOT_ENABLED']
# Add a positive define for code that needs to behave differently outside the engine.
test_env.Append(CPPDEFINES=['STANDALONE'])
test_sources = [f"{SRC}/TEST/test.cpp"]
# Exclude Godot-entry-point files from the standalone test build.
for s in sources:
    if not s.endswith('register_types.cpp'):
        test_sources.append(s)


test_program = test_env.Program(
    target="bin/no_godot_test",
    source=test_sources,
)
test_run = test_env.Command(
    target="bin/.test_ran",               # a small stamp file so scons knows it ran
    source=test_program,
    action=["$SOURCE", Touch("$TARGET")]  # run it, then touch the stamp
)
env.Alias("test_build", test_program) # tells scons to build the test program if test_build is included
