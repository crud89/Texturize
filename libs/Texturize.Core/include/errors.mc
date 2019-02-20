; /////////////////////////////////////////////////////////////////////////////////////////////////
; // Error definitions.
; // Compile this using command line "mc.exe errors.mc" (requires the Windows Platform SDK).
; /////////////////////////////////////////////////////////////////////////////////////////////////
MessageIdTypedef=DWORD

SeverityNames=(
	Success=0x0:STATUS_SEVERITY_SUCCESS
	Informational=0x1:STATUS_SEVERITY_INFORMATIONAL
	Warning=0x2:STATUS_SEVERITY_WARNING
	Error=0x3:STATUS_SEVERITY_ERROR
)

FacilityNames=(
	System=0x0FF:FACILITY_SYSTEM
	Texturize=0xFFF:FACILITY_TEXTURIZE
)

LanguageNames=(English=0x409:MSG00409)

; /////////////////////////////////////////////////////////////////////////////////////////////////

MessageId=0x01
Severity=Error
Facility=Texturize
SymbolicName=TEXTURIZE_ERROR_ASSERT
Language=English
An assertation condition has not been met.
.

MessageId=0x02
Severity=Error
Facility=Texturize
SymbolicName=TEXTURIZE_ERROR_IO
Language=English
An error occured while reading or writing a file.
.

; /////////////////////////////////////////////////////////////////////////////////////////////////
; // This is the end of the file. The comment here helps to prevent a common pitfall, where the  //
; // last line of the file (".") needs to be terminated with a newline.                          //
; /////////////////////////////////////////////////////////////////////////////////////////////////