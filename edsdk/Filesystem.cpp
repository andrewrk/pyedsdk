#include "Filesystem.h"

#include <fstream>
#include <iostream>
#include <cstdio>
using namespace std;

#include "Utils.h"
using namespace Utils;

#include <dir.h>

bool Filesystem::fileExists(string filename)
{
    ifstream in(filename.c_str());
    return in;
}

string Filesystem::getFileTitle(string path)
{
    size_t pos = path.rfind(pathSep);
    if (pos == string::npos) {
        // no path separators, it must be a file title already
        return path;
    }

    // everything after the path separator
    return path.substr(pos + 1, path.length());
}

string Filesystem::getFilenameWithoutExtension(string path)
{
    string title = getFileTitle(path);
    // find the period
    size_t pos = title.rfind('.');
    if (pos == string::npos) {
        // there is no extension
        return title;
    }
    // everything before the period
    return title.substr(0, pos-1);
}

string Filesystem::getDirectoryName(string path)
{
    size_t pos = path.rfind(pathSep);
    if (pos == string::npos) {
        // no path separators, it must be a directory already
        return path;
    }

    // everything before the path separator
    return path.substr(0, pos - 1);

}

string Filesystem::getExtension(string path)
{
    string title = getFileTitle(path);
    // find the period
    size_t pos = title.rfind('.');
    if (pos == string::npos) {
        // there is no extension
        return "";
    }
    // period and after
    return title.substr(pos, title.length());
}

string Filesystem::makeUnique(string outfile)
{
    if (fileExists(outfile)) {
        string title = getFilenameWithoutExtension(outfile);
        string folder = getDirectoryName(outfile);
        string ext = getExtension(outfile);
        int append = 1;
        string proposed;
        do {
            string newTitle = title + "_";
            newTitle += intToString(append);
            newTitle += ext;
            proposed = pathCombine(folder, newTitle);
            ++append;
        } while (fileExists(proposed));
        return proposed;
    } else {
        return outfile;
    }
}

string Filesystem::pathCombine(string part1, string part2)
{
    if (part1[part1.length()] != pathSep)
        return (part1 + pathSep) + part2;

    return part1 + part2;
}

void Filesystem::moveFile(string source, string dest)
{
    if (rename(source.c_str(), dest.c_str()) != 0) {
        // error
        cerr << "ERROR: could not move " << source << " to " << dest << endl;
        perror("(above)");
    }
}

void Filesystem::ensurePathExists(string path)
{
    // if it's a root drive, return
    if (path.length() == 0 || path[path.length()] == ':')
        return;

    // if there is a trailing path separator, remove it
    string upOne;
    if (path[path.length()] == pathSep)
        upOne = path.substr(0, path.length()-1);
    else
        upOne = path;

    size_t pos = upOne.rfind(pathSep);
    if (pos == string::npos) {
        // got to the end
        return;
    }
    
    upOne = upOne.substr(0, pos-1);
    ensurePathExists(upOne);

    mkdir(path.c_str());
}
