#pragma once

// win_32_console.H
// taken from: http://www.williamwilling.com/blog/?p=74
// ... corrected and modified to my needs.

#include <fstream>

class Console
{
private:
	std::ofstream _out;
	std::ofstream _err;

	std::streambuf *_old_cout;
	std::streambuf *_old_cerr;

	FILE *_stdout;
	FILE *_stderr;

	bool _valid;
public:
	bool valid() const { return _valid; }
	Console( int attach_ = true, bool attach_stdout_ = false );
	~Console();
};

// win32_console.cpp
#include <cstdio>
#include <iostream>
#include <windows.h>


#include <windows.h>
#include <stdio.h>
#include <fcntl.h>
#include <io.h>
//#include <iostream>

Console::Console( int attach_/* = true*/, bool attach_stdout_/* = false*/ ) :
	_stdout( 0 ),
	_stderr( 0 ),
	_valid( false )
{
	// create a console window
	if ( attach_ )
		_valid = AttachConsole( ATTACH_PARENT_PROCESS );
	else
		_valid = AllocConsole();

	if ( !_valid )
		return;

	Sleep(50);

    // 1. Reopen standard streams
    FILE* fp;
    freopen_s(&fp, "CONOUT$", "w", stdout);
    freopen_s(&fp, "CONOUT$", "w", stderr);
    freopen_s(&fp, "CONIN$", "r", stdin);

    // 2. CRITICAL FOR POWERSHELL: Clear the error state flags.
    // Early initialization sets these to a 'bad' state because the handles 
    // were missing at the exact moment of execution startup.

    setvbuf(stdout, NULL, _IONBF, 0);
    setvbuf(stderr, NULL, _IONBF, 0);

    std::cout.clear();
    std::cerr.clear();
    std::cin.clear();

    // 3. Synchronize streams and shut off internal buffering for the first few logs
    std::ios::sync_with_stdio(true);
    
    // Force unbuffered output so logs drop straight through the pipe to PowerShell
    std::setvbuf(stdout, NULL, _IONBF, 0); 
    std::setvbuf(stderr, NULL, _IONBF, 0);



#if 0
    // 2. Clear old handles and open the physical console stream
    // Using CreateFile ensures we get a fresh, raw OS handle to the terminal screen buffer
    HANDLE hConsoleOut = CreateFileW(L"CONOUT$", GENERIC_WRITE, FILE_SHARE_WRITE, 
                                     NULL, OPEN_EXISTING, 0, NULL);
    HANDLE hConsoleIn  = CreateFileW(L"CONIN$", GENERIC_READ, FILE_SHARE_READ, 
                                     NULL, OPEN_EXISTING, 0, NULL);

    if (hConsoleOut != INVALID_HANDLE_VALUE) 
    {
        // 3. Force the OS layer to register the new handles
        SetStdHandle(STD_OUTPUT_HANDLE, hConsoleOut);
        SetStdHandle(STD_ERROR_HANDLE, hConsoleOut);
        
        // 4. Force the C Runtime (CRT) layer to register the new handles
        FILE* fp;
        freopen_s(&fp, "CONOUT$", "w", stdout);
        freopen_s(&fp, "CONOUT$", "w", stderr);
    }

    if (hConsoleIn != INVALID_HANDLE_VALUE) 
    {
        SetStdHandle(STD_INPUT_HANDLE, hConsoleIn);
        FILE* fp;
        freopen_s(&fp, "CONIN$", "r", stdin);
    }

    // 5. Clear error flags on streams and synchronize
    std::cout.clear();
    std::cerr.clear();
    std::cin.clear();
    std::ios::sync_with_stdio(true);



#if 0

#if 1

    // 2. Redirect STDOUT
    if (GetStdHandle(STD_OUTPUT_HANDLE) != INVALID_HANDLE_VALUE)
    {
        // Reopen CONOUT$ to link standard output to the console window
        FILE* fp;
        if (freopen_s(&fp, "CONOUT$", "w", stdout) == 0)
        {
            std::clog.clear();
            std::cout.clear();
        }
    }

    // 3. Redirect STDERR
    if (GetStdHandle(STD_ERROR_HANDLE) != INVALID_HANDLE_VALUE)
    {
        FILE* fp;
        if (freopen_s(&fp, "CONOUT$", "w", stderr) == 0)
        {
            std::cerr.clear();
        }
    }

    // 4. Redirect STDIN
    if (GetStdHandle(STD_INPUT_HANDLE) != INVALID_HANDLE_VALUE)
    {
        FILE* fp;
        if (freopen_s(&fp, "CONIN$", "r", stdin) == 0)
        {
            std::cin.clear();
        }
    }

    // 5. Synchronize the C++ streams with the C standard streams
    std::ios::sync_with_stdio();

#else
	// redirect std::cout to our console window
	_old_cout = std::cout.rdbuf();
	_out.open( "CONOUT$" );
	std::cout.rdbuf( _out.rdbuf() );

	// redirect std::cerr to our console window
	_old_cerr = std::cerr.rdbuf();
	_err.open( "CONOUT$" );
	std::cerr.rdbuf( _err.rdbuf() );

	if ( attach_stdout_ )
	{
		// make stdout/stderr also use console window (how to reset?)
		_stdout = freopen( "CONOUT$", "w", stdout );
		_stderr = freopen( "CONOUT$", "w", stderr );
	}
#endif

#endif

#endif

HANDLE hConsole = GetStdHandle(STD_OUTPUT_HANDLE);
    const char* firstMsg = "Absolute first log visible via WriteConsole!\n";
    DWORD written;
    WriteConsoleA(hConsole, firstMsg, (DWORD)strlen(firstMsg), &written, NULL);


}

Console::~Console()
{
	if ( !_valid )
		return;

	// reset the standard streams
	_out.flush();
	_err.flush();

	std::cout.rdbuf( _old_cout );
	std::cerr.rdbuf( _old_cerr );

	if ( _stdout )
		fclose( _stdout );
	if ( _stderr )
		fclose( _stderr );

	// remove the console window
	FreeConsole();
}
