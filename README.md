# 7zToTar
A Toy program that converts .7z to posix format tar.

This program is used to convert a 7z file to posix format tar by the 7z command utiliy.

This program is only workable under AMD64/Linux.

It uses the 7z command to extract the stream and then it encapsulates it with tar stream out standard output. The original information in the 7z archive is not append to the tar, such as the time information.

The correction of this program is no guarantee.

Thank Gary S. Brown for the crc32 program.
