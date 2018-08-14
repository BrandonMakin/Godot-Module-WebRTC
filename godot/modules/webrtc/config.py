
def can_build(platform):
    return True


def configure(env):
  libpath = "#thirdparty/webrtc/lib/x64/Release/"
  libname = "libwebrtc_full"
  env.Append(CPPPATH=[libpath])
  if env["platform"]== "x11":
    env.Append(LIBS=["libwebrtc_full"])
    env.Append(LIBPATH=[libpath])
  elif env["platform"] == "windows":
    # mostly VisualStudio
    if env["CC"] == "cl":
      env.Append(LINKFLAGS=[p + env["LIBSUFFIX"] for p in ["secur32", libname]])
      env.Append(LIBPATH=[libpath])
    # mostly "gcc"
    else:
      env.Append(LIBS=["secur32", libname])
      env.Append(LIBPATH=[libpath])
  elif env["platform"] == "osx":
    env.Append(LIBS=[libname])
    env.Append(LIBPATH=[libpath])
