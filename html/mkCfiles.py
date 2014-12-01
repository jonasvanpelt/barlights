#!/usr/bin/python
import codecs
"""
This file automatically generates the www_data.h and www_data.c
files for use by HTTP(S) servers. To include your file, simply
add it to the list of files.

Alexander Zuliani --- I somehow managed to write unreadable python. Sorry for that
"""

listOfFiles = ["index.html"]

def write_header_top(handle):
	header="""#ifndef WWW_FILES_H_
#define WWW_FILES_H_

// THIS FILE IS AUTOMATICALLY GENERATED.
// MODIFY sources/mkCfiles.py INSTEAD OF THIS.


"""
	handle.write(header)

def write_header_middle(handle):
	handle.write("""

struct Www_file {
	const char * filename;
	const int * filesize;
	const char * content;
	const int cacheable;
};

extern const struct Www_file www_files[%d];
extern const int num_files;
""" % (len(listOfFiles), ) )

def write_header_end(handle):
	handle.write("\n#endif\n")

def write_c_top(handle):
	handle.write("""#include "www_files.h"

// THIS FILE IS AUTOMATICALLY GENERATED.
// MODIFY sources/mkCfiles.py INSTEAD OF THIS.
""")

def write_c_middle(handle):
	hex_encoder = codecs.getencoder('hex')
	for name in listOfFiles:
		name_as_scores = name.replace(".","_")
		lenname = name_as_scores + "_len"
		count = 0;
		bin_handle = open(name,'rb')
		handle.write("static const unsigned char %s[] = {\n    " % (name_as_scores, ) )
		while(1):
			c = bin_handle.read(1)
			if not c: 
				break
			handle.write("0x%s, " % (hex_encoder(c)[0].decode(), ))
			count+=1
			if (count % 16 == 0):
				handle.write("\n    ")

		handle.write("\n};\nstatic const int %s = %s;\n\n" % (lenname, count))

	handle.write("const struct Www_file www_files[%d] = {\n"%(len(listOfFiles),))
	for name in listOfFiles:
		name_as_scores = name.replace(".","_")
		lenname = name_as_scores + "_len"
		handle.write("    { \"%s\", &%s, %s, 1 },\n"%(name, lenname, name_as_scores))

	handle.write("};\n")
	handle.write("const int num_files = %d;"%(len(listOfFiles),))


def write_c_end(handle):
	pass
	
def do():
	try:
		header_file = open("../www_files.h",'w')
		c_file = open("../www_files.c", 'w')
	except:
		print("Failed to open files for writing!")
		return


	write_header_top(header_file)
	write_header_middle(header_file)
	write_header_end(header_file)
	write_c_top(c_file)
	write_c_middle(c_file)
	write_c_end(c_file)
	print("HERE JONAS")

	header_file.close()
	c_file.close()

if __name__=="__main__":
	do()

