/*
 * Loader Implementation
 *
 * 2018, Operating Systems
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/mman.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>

#include "exec_parser.h"

static so_exec_t *exec;

// Default handler
static struct sigaction def;

// Page size (saved for less function calls)
static int page_size;

// Descriptor
static int des;

// Returns segment index of the segment containing the address given
static int segment(uintptr_t pf_at)
{
	// Loop through segment array
	for (int i = 0; i < exec->segments_no; i++) {
		if (exec->segments[i].vaddr <= pf_at &&
			pf_at <= exec->segments[i].vaddr + exec->segments[i].mem_size) {
				return i;
			}
	}

	// Returns -1 for no segment matching the given address
	return -1;
}

static void handler(int signum, siginfo_t *siginfo, void *other)
{
	// Page Fault address
	uintptr_t pf_at = (uintptr_t) siginfo->si_addr;

	// Page Fault segment index
	int pf_seg = segment(pf_at);

	// Buffer to read from file
	char *buf = malloc(page_size * sizeof(char));

	// Check for invalid memory access
	if (pf_seg == -1) {
		def.sa_sigaction(signum, siginfo, other);
		return;
	}

	// Page Fault page index
	int pf_page = (pf_at - exec->segments[pf_seg].vaddr) / page_size;

	// Page Fault page offset
	int pf_page_offset = page_size * pf_page;

	/* Use data field to keep track of which pages have been handled in the segment
	 * If data field is empty, we must instantiate the array containing the status of each page
	 * 0 if page has not been handled, 1 otherwise
	 */
	if (exec->segments[pf_seg].data == NULL) {
		int page_count = exec->segments[pf_seg].mem_size / page_size + 1;

		exec->segments[pf_seg].data = calloc(page_count, sizeof(char));
	}

	// Check if current page has been handled, if not, handle it
	if (((char *)(exec->segments[pf_seg].data))[pf_page] != 1) {
		// Mark page as handled
		((char *)(exec->segments[pf_seg].data))[pf_page] = 1;

		// Starting address
		uintptr_t start_addr = exec->segments[pf_seg].vaddr + pf_page_offset;

		// Map the page as needed, save the mapped address
		char *map_addr = mmap((void *) start_addr, page_size, PROT_READ | PROT_WRITE,
										MAP_SHARED | MAP_ANONYMOUS | MAP_FIXED, -1, 0);

		// Check mmap error
		if (map_addr == MAP_FAILED) {
			fprintf(stderr, "mmap error!\n");
			exit(1);
		}

		// How much to read out of the page, if value is negative, read whole page
		int read_size = page_size - exec->segments[pf_seg].file_size + pf_page_offset;

		if (read_size <= 0)
			read_size = page_size;
		else {
			read_size = exec->segments[pf_seg].file_size - pf_page_offset;
			if (read_size < 0)
				read_size = 0;
		}

		// If there is something to be read from the file
		if (read_size > 0) {
			// Move the cursor to where we need to read from
			if (lseek(des, exec->segments[pf_seg].offset + pf_page_offset, SEEK_SET) == -1) {
				fprintf(stderr, "lseek error! errno %d\n", errno);
				exit(1);
			}

			// Read the data into a buffer
			if (read(des, buf, read_size) == -1) {
				fprintf(stderr, "Read error! errno %d\n", errno);
				exit(1);
			}

			// Copy the data in memory
			if (memcpy(map_addr, buf, read_size) == NULL) {
				fprintf(stderr, "memcpy error!");
				exit(1);
			}

			// Set segment permissions
			if (mprotect(map_addr, page_size, exec->segments[pf_seg].perm) == -1) {
				printf("mprotect error! errno %d\n", errno);
				exit(1);
			}
		}
	} else {
		def.sa_sigaction(signum, siginfo, other);
	}

	// Free allocated buffer
	free(buf);
}

int so_init_loader(void)
{
	// Create action
	struct sigaction act;

	// Set Handler
	act.sa_sigaction = handler;

	// Set Mask
	sigemptyset(&act.sa_mask);
	sigaddset(&act.sa_mask, SIGSEGV);

	// Set flag to use sa_sigaction instead of sa_handler as the signal-handling function
	act.sa_flags = SA_SIGINFO;

	// Set page size
	page_size = getpagesize();

	// Check for error
	if (sigaction(SIGSEGV, &act, &def) == -1) {
		fprintf(stderr, "Sigaction error!\n");
		exit(1);
	}

	// Close file
	close(des);

	return 0;
}

int so_execute(char *path, char *argv[])
{
	exec = so_parse_exec(path);

	// Set descriptor
	des = open(path, O_RDONLY);

	if (!exec)
		return -1;

	so_start_exec(exec, argv);

	return -1;
}
