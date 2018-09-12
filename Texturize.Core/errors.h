 /////////////////////////////////////////////////////////////////////////////////////////////////
 // Error definitions.
 // Compile this using command line "mc.exe errors.mc" (requires the Windows Platform SDK).
 /////////////////////////////////////////////////////////////////////////////////////////////////
 /////////////////////////////////////////////////////////////////////////////////////////////////
//
//  Values are 32 bit values laid out as follows:
//
//   3 3 2 2 2 2 2 2 2 2 2 2 1 1 1 1 1 1 1 1 1 1
//   1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0 9 8 7 6 5 4 3 2 1 0
//  +---+-+-+-----------------------+-------------------------------+
//  |Sev|C|R|     Facility          |               Code            |
//  +---+-+-+-----------------------+-------------------------------+
//
//  where
//
//      Sev - is the severity code
//
//          00 - Success
//          01 - Informational
//          10 - Warning
//          11 - Error
//
//      C - is the Customer code flag
//
//      R - is a reserved bit
//
//      Facility - is the facility code
//
//      Code - is the facility's status code
//
//
// Define the facility codes
//
#define FACILITY_SYSTEM                  0xFF
#define FACILITY_TEXTURIZE               0xFFF


//
// Define the severity codes
//
#define STATUS_SEVERITY_SUCCESS          0x0
#define STATUS_SEVERITY_INFORMATIONAL    0x1
#define STATUS_SEVERITY_WARNING          0x2
#define STATUS_SEVERITY_ERROR            0x3


//
// MessageId: TEXTURIZE_ERROR_ASSERT
//
// MessageText:
//
// An assertation condition has not been met.
//
#define TEXTURIZE_ERROR_ASSERT           ((DWORD)0xCFFF0001L)

//
// MessageId: TEXTURIZE_ERROR_IO
//
// MessageText:
//
// An error occured while reading or writing a file.
//
#define TEXTURIZE_ERROR_IO               ((DWORD)0xCFFF0002L)

 /////////////////////////////////////////////////////////////////////////////////////////////////
 // This is the end of the file. The comment here helps to prevent a common pitfall, where the  //
 // last line of the file (".") needs to be terminated with a newline.                          //
 /////////////////////////////////////////////////////////////////////////////////////////////////