#!/bin/sh

# generate base64.h

awk '
	BEGIN {
		map = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/"
		print "static const char *MINUID_BASE64_I2C = \"" map "\";"
		for(n = 1; n <= length(map); n++)
			MAP[substr(map, n, 1)] = n - 1
		printf "static const int MINUID_BASE64_C2I[256] = {"
		for(n = 0; n < 256; n++) {
			if ((n % 16) == 0)
				print ""
			else
				printf " "
			chr = sprintf("%c", n)
			if (chr in MAP) {
				if (MAP[chr] < 10)
					printf(" %d", MAP[chr])
				else
					printf("%d", MAP[chr])
			}
			else
				printf("-1")
			
			if (n != 255)
				printf(",")
		}
		print ""
		print "};"
	}
' > base64.h

