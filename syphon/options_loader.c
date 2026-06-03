#include "options_loader.h"
#include <CoreFoundation/CoreFoundation.h>
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

FangsOptions fangs_load_options(void) {
    FangsOptions opts = {false, false, false};
    const char* path = "/opt/pluginplayground/current.options";

    FILE* f = fopen(path, "rb");
    if (!f) return opts;

    fseek(f, 0, SEEK_END);
    long len = ftell(f);
    fseek(f, 0, SEEK_SET);

    char* buf = malloc((size_t)len);
    if (!buf) { fclose(f); return opts; }
    fread(buf, 1, (size_t)len, f);
    fclose(f);

    CFDataRef cfData = CFDataCreateWithBytesNoCopy(kCFAllocatorDefault, (const UInt8*)buf, (CFIndex)len, kCFAllocatorNull);
    if (!cfData) { free(buf); return opts; }

    CFPropertyListRef plist = CFPropertyListCreateWithData(kCFAllocatorDefault, cfData, kCFPropertyListImmutable, NULL, NULL);
    CFRelease(cfData);
    free(buf);

    if (!plist || CFGetTypeID(plist) != CFDictionaryGetTypeID()) {
        if (plist) CFRelease(plist);
        return opts;
    }

    CFDictionaryRef dict = (CFDictionaryRef)plist;

    CFBooleanRef val;
    val = (CFBooleanRef)CFDictionaryGetValue(dict, CFSTR("disablePAC"));
    if (val && CFGetTypeID(val) == CFBooleanGetTypeID())
        opts.disablePAC = (bool)CFBooleanGetValue(val);

    val = (CFBooleanRef)CFDictionaryGetValue(dict, CFSTR("useLegacyAmmonia"));
    if (val && CFGetTypeID(val) == CFBooleanGetTypeID())
        opts.useLegacyAmmonia = (bool)CFBooleanGetValue(val);

    val = (CFBooleanRef)CFDictionaryGetValue(dict, CFSTR("pauseInjection"));
    if (val && CFGetTypeID(val) == CFBooleanGetTypeID())
        opts.pauseInjection = (bool)CFBooleanGetValue(val);

    CFRelease(dict);
    return opts;
}

char* fangs_build_dyld_insert_libraries(bool useLegacyAmmonia) {
    if (useLegacyAmmonia)
        return strdup("/private/var/ammonia/core/libopener.dylib");

    const char* dir = "/opt/pluginplayground/tweaks";
    DIR* d = opendir(dir);
    if (!d) return NULL;

    size_t total = 0;
    int count = 0;
    char** paths = NULL;

    struct dirent* entry;
    while ((entry = readdir(d)) != NULL) {
        const char* name = entry->d_name;
        size_t nlen = strlen(name);
        if (nlen > 6 && strcmp(name + nlen - 6, ".dylib") == 0) {
            size_t plen = strlen(dir) + 1 + nlen + 1;
            char* full = malloc(plen);
            snprintf(full, plen, "%s/%s", dir, name);
            total += plen;
            count++;
            paths = realloc(paths, sizeof(char*) * (size_t)count);
            paths[count - 1] = full;
        }
    }
    closedir(d);

    if (count == 0) {
        free(paths);
        return NULL;
    }

    // colon-separated: path1:path2:...  (no trailing colon)
    size_t needed = total + 1; // null terminator
    char* result = malloc(needed);
    if (!result) {
        for (int i = 0; i < count; i++) free(paths[i]);
        free(paths);
        return NULL;
    }
    result[0] = '\0';
    for (int i = 0; i < count; i++) {
        if (i > 0) strcat(result, ":");
        strcat(result, paths[i]);
        free(paths[i]);
    }
    free(paths);
    return result;
}
