/*
 * Copyright © 2026 Intel Corporation
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

/*
 * crtclist - inspect CRTC usage on a DRM device via drmModeGetCrtcList().
 *
 * Prints the total number of CRTCs, how many are free vs in use, and a
 * per-CRTC line showing state, scanout framebuffer and active mode. Useful
 * for compositor bring-up to see which CRTCs are available.
 *
 * Usage: crtclist [/dev/dri/cardN]   (defaults to the first KMS node)
 */

#include <fcntl.h>
#include <stdio.h>
#include <unistd.h>

#include <xf86drm.h>
#include <xf86drmMode.h>

static int open_kms_node(void)
{
	char path[64];
	int i;

	for (i = 0; i < 16; i++) {
		int fd;

		snprintf(path, sizeof(path), "/dev/dri/card%d", i);
		fd = open(path, O_RDWR | O_CLOEXEC);
		if (fd < 0)
			continue;
		if (drmIsKMS(fd)) {
			printf("Using %s\n", path);
			return fd;
		}
		close(fd);
	}
	return -1;
}

int main(int argc, char **argv)
{
	drmModeCrtcListPtr list;
	int fd, i;

	if (argc > 1)
		fd = open(argv[1], O_RDWR | O_CLOEXEC);
	else
		fd = open_kms_node();

	if (fd < 0) {
		fprintf(stderr, "No accessible KMS DRM device (try running as root).\n");
		return 1;
	}

	list = drmModeGetCrtcList(fd);
	if (!list) {
		fprintf(stderr, "drmModeGetCrtcList() failed\n");
		close(fd);
		return 1;
	}

	printf("Total CRTCs : %d\n", list->count);
	printf("Available   : %d\n", list->count_available);
	printf("In use      : %d\n\n", list->count - list->count_available);

	printf("%-5s %-8s %-13s %-8s %s\n",
	       "idx", "crtc_id", "state", "fb_id", "mode");
	printf("-------------------------------------------------------\n");
	for (i = 0; i < list->count; i++) {
		drmModeCrtcInfoPtr c = &list->crtcs[i];
		const char *state;

		if (c->in_use)
			state = "IN USE";
		else if (c->driver_active)
			state = "ACTIVE";
		else
			state = "FREE";

		printf("%-5d %-8u %-13s %-8u ",
		       c->index, c->crtc_id, state, c->buffer_id);
		if (c->in_use)
			printf("%ux%u @ %uHz (%s)\n",
			       c->width, c->height, c->mode.vrefresh,
			       c->mode.name);
		else if (c->driver_active)
			printf("(active in hardware, not enabled via KMS)\n");
		else
			printf("-\n");
	}

	drmModeFreeCrtcList(list);
	close(fd);
	return 0;
}
