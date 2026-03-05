#pragma once

#if ( defined __APPLE__ ) || ( defined __linux__ ) || ( defined __MING32__ )
#include <unistd.h>
#include <sys/types.h>
#endif

#ifdef _WIN32
#include <windows.h>
#include <io.h>	// _access
#define random rand
#define srandom srand
#endif

// fallback Windows native
#ifndef R_OK
#define R_OK	4
#endif
#define _access access
