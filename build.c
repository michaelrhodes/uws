#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

#ifdef _WIN32
#define OS "win32"
#define IS_WINDOWS
#endif
#ifdef __linux
#define OS "linux"
#define IS_LINUX
#endif
#ifdef __APPLE__
#define OS "darwin"
#define IS_MACOS
#endif

struct node_version {
  char *name;
  char *abi;
} versions[] = {
  {"v10.22.0", "64"},
  {"v12.18.3", "72"},
  {"v14.8.0", "83"}
};

int exec (const char *cmd, ...) {
  char buf[512];
  va_list args;
  va_start(args, cmd);
  vsprintf(buf, cmd, args);
  va_end(args);
  printf("--> %s\n", buf);
  return system(buf);
}

void download () {
  for (unsigned int i = 0; i < sizeof(versions) / sizeof(struct node_version); i++) {
    exec("curl -OJ https://nodejs.org/dist/%s/node-%s-headers.tar.gz", versions[i].name, versions[i].name);
    exec("tar xzf node-%s-headers.tar.gz -C targets", versions[i].name);
#ifdef IS_WINDOWS
    exec("curl https://nodejs.org/dist/%s/win-x64/node.lib > targets/node-%s/node.lib", versions[i].name, versions[i].name);
#endif
  }
}

void build_windows (char *os, char *arch) {
  for (unsigned int i = 0; i < sizeof(versions) / sizeof(struct node_version); i++) {
    exec("cl /W3 /D \"UWS_HTTPRESPONSE_NO_WRITEMARK\" /D \"UWS_WITH_PROXY\" /D \"LIBUS_USE_LIBUV\" /D \"LIBUS_USE_OPENSSL\" /std:c++17 /I uWebSockets/uSockets/src uWebSockets/uSockets/src/*.c "
      "uWebSockets/uSockets/src/eventing/*.c uWebSockets/uSockets/src/crypto/*.c /I targets/node-%s/include/node /I uWebSockets/src /EHsc "
      "/Ox /LD /Fedist/uws_%s_%s_%s.node src/addon.cpp targets/node-%s/node.lib",
      versions[i].name, os, arch, versions[i].abi, versions[i].name);
  }
}

void build_unix (char *compiler, char *cpp_compiler, char *cpp_linker, char *os, char *arch) {
  char *c_shared = "-DLIBUS_USE_LIBUV -DLIBUS_USE_OPENSSL -flto -O3 -c -fPIC -I uWebSockets/uSockets/src uWebSockets/uSockets/src/*.c uWebSockets/uSockets/src/eventing/*.c uWebSockets/uSockets/src/crypto/*.c";
  char *cpp_shared = "-DUWS_HTTPRESPONSE_NO_WRITEMARK -DUWS_WITH_PROXY -DLIBUS_USE_LIBUV -DLIBUS_USE_OPENSSL -flto -O3 -c -fPIC -std=c++17 -I uWebSockets/uSockets/src -I uWebSockets/src src/addon.cpp";

  for (unsigned int i = 0; i < sizeof(versions) / sizeof(struct node_version); i++) {
    exec("%s %s -I targets/node-%s/include/node", compiler, c_shared, versions[i].name);
    exec("%s %s -I targets/node-%s/include/node", cpp_compiler, cpp_shared, versions[i].name);
    exec("%s %s %s -o dist/uws_%s_%s_%s.node", cpp_compiler, "-flto -O3 *.o -std=c++17 -shared", cpp_linker, os, arch, versions[i].abi);
  }
}

int main () {
  if (!exec("mkdir dist") && !exec("mkdir targets")) download();

#ifdef IS_WINDOWS
  build_windows(OS, "x64");
#endif

#ifdef IS_MACOS
  build_unix("clang -mmacosx-version-min=10.7",
    "clang++ -stdlib=libc++ -mmacosx-version-min=10.7",
    "-undefined dynamic_lookup", OS, "x64");
#endif

#ifdef IS_LINUX
  build_unix("clang", "clang++", "-static-libstdc++ -static-libgcc -s", OS, "x64");
  build_unix("aarch64-linux-gnu-gcc", "aarch64-linux-gnu-g++", "-static-libstdc++ -static-libgcc -s", OS, "arm64");
#endif
}
