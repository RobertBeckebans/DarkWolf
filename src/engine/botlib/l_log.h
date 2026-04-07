/*
===========================================================================

Return to Castle Wolfenstein single player GPL Source Code
Copyright (C) 1999-2010 id Software LLC, a ZeniMax Media company.

This file is part of the Return to Castle Wolfenstein single player GPL Source Code (RTCW SP Source Code).

RTCW SP Source Code is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

RTCW SP Source Code is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with RTCW SP Source Code.  If not, see <http://www.gnu.org/licenses/>.

In addition, the RTCW SP Source Code is also subject to certain additional terms. You should have received a copy of these additional terms immediately following the terms and conditions of the GNU
General Public License which accompanied the RTCW SP Source Code.  If not, please request a copy in writing from id Software at the address below.

If you have questions concerning this license or the applicable additional terms, you may contact in writing id Software LLC, c/o ZeniMax Media Inc., Suite 120, Rockville, Maryland 20850 USA.

===========================================================================
*/

/*****************************************************************************
 * name:		l_log.h
 *
 * desc:		log file
 *
 *
 *****************************************************************************/

/*!
	\brief Opens a log file when logging is enabled.

	The function first checks if logging is activated by querying the configuration variable "log"; if logging is disabled it exits immediately. When logging is enabled it delegates to Log_AlwaysOpen
   to actually open the file specified by the filename argument.

	\param filename the full path to the log file to be opened
*/
void	   Log_Open( char* filename );

/*!
	\brief Opens a log file for writing if one is not already open

	If the supplied filename is null or empty, the function prints a usage message and returns.  If a log file is already open, an error message is printed and the function exits.  The function
   attempts to open the file in binary write mode; failure to open results in an error message.  On success, the global logfile structure is populated with the file pointer and the filename (truncated
   to MAX_LOGFILENAMESIZE characters).  Informational output confirms the file that was opened.

	\param filename Path of the file to open
*/
void	   Log_AlwaysOpen( char* filename );

/*!
	\brief Closes the current log file, if one is open.

	When called, the function first checks whether a file pointer is active. If not, it simply exits. Otherwise it attempts to close the file. If the close operation fails, an error message is printed
   via botimport.Print, and the function returns immediately. If the close succeeds, the file pointer is cleared and a message confirming the closure is output.

*/
void	   Log_Close();

/*!
	\brief Shuts down the logging system by closing the log file if it is open.

	The function checks whether the global log file pointer has been initialized and, if so, calls Log_Close to close the file. If the file was never opened, no action is taken.

*/
void	   Log_Shutdown();

/*!
	\brief Writes a formatted message to the current log file if it is open.

	The function accepts a printf‑style format string followed by a variable number of arguments. It first checks whether the global logfile handle is valid; if not, it simply returns. When the file
   is open, the function uses vfprintf to output the formatted text to the file and then flushes the stream to ensure the data is written immediately. The implementation deliberately omits an
   automatic newline, leaving any necessary characters to be provided explicitly by the caller.

	\param fmt Printf‑style format string that specifies how the remaining arguments should be formatted.
*/
void QDECL Log_Write( char* fmt, ... );

/*!
	\brief Writes a formatted message to the current log file, prefixed with a write counter and timestamp.

	If the log file is not open, the function returns without doing anything.
	The timestamp is printed as <write count>   HH:MM:SS:CC   where HH is hours since start, MM minutes, SS seconds, and CC hundredths of a second derived from botlibglobals.time.
	After the timestamp, the supplied format string is printed with the variable arguments.
	A carriage return and line feed are appended, the counter is incremented, and the output is flushed to ensure durability.

	\param fmt Format string used with vfprintf; it may contain format specifiers supported by printf.
*/
void QDECL Log_WriteTimeStamped( char* fmt, ... );

/*!
	\brief Returns a pointer to the log file used by the logging system.

	The function retrieves the FILE* stored in the global logfile structure. The returned pointer is used throughout the codebase for writing log entries. It may be NULL if the log file has not yet
   been initialized or if it was closed.

	\return a FILE* pointing to the log file or NULL if unavailable
*/
FILE*	   Log_FilePointer();

/*!
	\brief Flushes the opened log file to disk.

	If the log file has been successfully opened, the function calls fflush on its file pointer to ensure all buffered output is written to the underlying file. No operation is performed when the file
   pointer is null.

*/
void	   Log_Flush();
