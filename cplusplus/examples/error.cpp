/*
 * compile with:
 *
 *      g++ -g -Wall error.cpp `pkg-config vips-cpp --cflags --libs`
 *
 */

#include <vips/vips8>
#include <iostream>

int
main(int argc, char **argv)
{
	if (VIPS_INIT(argv[0]))
		vips_error_exit(nullptr);

	try {
		vips::VImage::thumbnail("/dev/null", 800);
	}
	catch (const vips::VError &e) {
		std::cerr << "Unable to thumbnail: " << e.what() << std::endl;
	}

	return 0;
}
