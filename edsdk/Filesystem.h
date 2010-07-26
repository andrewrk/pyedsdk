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

    string getFileTitle(string path);
    string getFilenameWithoutExtension(string path);
    string getExtension(string path);

    // all the path leading up to path, not including the filename.
    string getDirectoryName(string path);

    // return a path as close to outfile as possible which is
    // guaranteed to be unique
    string makeUnique(string outfile);

    string pathCombine(string part1, string part2);

    // make directory structure such that path exists
    void ensurePathExists(string path);

    bool fileExists(string filename);
    void moveFile(string source, string dest);
}

#endif
