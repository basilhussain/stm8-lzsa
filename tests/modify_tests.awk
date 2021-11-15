#!/usr/bin/awk -f

BEGIN {
	print "#include <stddef.h>";
	print "#include <stdint.h>";
}

/^unsigned char/ {
	sub(/^unsigned char/, "static const uint8_t");
}

/^unsigned int/ {
	sub(/^unsigned int/, "// static const size_t");
}

{
	print $0;
}
