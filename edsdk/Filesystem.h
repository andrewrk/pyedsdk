#ifndef FILESYSTEM_H
#define FILESYSTEM_H

#include <string>
using namespace std;

namespace Filesystem
{
#ifdef _WIN32
    const char pathSep = '\\';
#else
    const char pathSep = '/';
#endif

    bool fileExists(string filename);
    string getFileTitle(string path);
    string getFilenameWithoutExtension(string path);
    string getDirectoryName(string path);
    string getExtension(string path);

    // return a path as close to outfile as possible which is
    // guaranteed to be unique
    string makeUnique(string outfile);

    // make directory structure such that path exists
    void ensurePathExists(string path);

    string pathCombine(string part1, string part2);

    void moveFile(string source, string dest);
};

#endif
