/*
 * Copyright (c) 2021 Sebastian Kylander https://gaztin.com/
 *
 * This software is provided 'as-is', without any express or implied warranty. In no event will
 * the authors be held liable for any damages arising from the use of this software.
 *
 * Permission is granted to anyone to use this software for any purpose, including commercial
 * applications, and to alter it and redistribute it freely, subject to the following restrictions:
 *
 * 1. The origin of this software must not be misrepresented; you must not claim that you wrote the
 *    original software. If you use this software in a product, an acknowledgment in the product
 *    documentation would be appreciated but is not required.
 * 2. Altered source versions must be plainly marked as such, and must not be misrepresented as
 *    being the original software.
 * 3. This notice may not be removed or altered from any source distribution.
 */

#if defined( _WIN32 )

#include "CompilerMSVC.h"

#include "Components/Project.h"

#include <Common/Process.h>

#include <Windows.h>

//////////////////////////////////////////////////////////////////////////

static std::wstring GetHostString( void )
{
	switch( Configuration::HostArchitecture() )
	{
		case Configuration::Architecture::x86:    return L"Hostx86";
		case Configuration::Architecture::x86_64: return L"Hostx64";
		default:                                  return L"Hostx64";
	}

} // GetHostString

//////////////////////////////////////////////////////////////////////////

static std::wstring GetTargetString( const Configuration::Architecture Architecture )
{
	switch( Architecture )
	{
		case Configuration::Architecture::x86:    return L"x86";
		case Configuration::Architecture::x86_64: return L"x64";
		default:                                  return GetTargetString( Configuration::HostArchitecture() );
	}

} // GetTargetString

//////////////////////////////////////////////////////////////////////////

static std::filesystem::path FindProgramFilesX86Dir( void )
{
	if( DWORD ProgramFilesLength = GetEnvironmentVariableW( L"ProgramFiles(x86)", nullptr, 0 ) )
	{
		std::wstring ProgramFilesBuffer;
		ProgramFilesBuffer.resize( ProgramFilesLength );

		GetEnvironmentVariableW( L"ProgramFiles(x86)", ProgramFilesBuffer.data(), ProgramFilesLength );

		// Remove extra null-terminator
		ProgramFilesBuffer.pop_back();

		return ProgramFilesBuffer;
	}

	return std::filesystem::path();

} // FindProgramFilesX86Dir

//////////////////////////////////////////////////////////////////////////

static std::filesystem::path FindMSVCDir( const std::filesystem::path& rProgramFilesX86 )
{
	const std::filesystem::path VSWhereLocation = rProgramFilesX86 / "Microsoft Visual Studio" / "Installer" / "vswhere.exe";
	if( std::filesystem::exists( VSWhereLocation ) )
	{
		// Run vswhere.exe to get the installation path of Visual Studio
		int                Result;
		std::wstring       VSWhereLocationFull = VSWhereLocation.wstring() + L" -latest -property installationPath";
		Process            VSWhereProcess = Process( VSWhereLocationFull );
		const std::wstring VSWhereOutput = VSWhereProcess.OutputOf( Result );
		if( Result == 0 )
		{
			// Trim trailing newlines
			const std::filesystem::path VisualStudioLocation( VSWhereOutput.begin(), VSWhereOutput.end() - 2 );
			if( std::filesystem::exists( VisualStudioLocation ) )
			{
				for( const std::filesystem::directory_entry& rMSVCDir : std::filesystem::directory_iterator( VisualStudioLocation / "VC" / "Tools" / "MSVC" ) )
				{
					// Just choose the first best version
					if( true )
						return rMSVCDir;
				}
			}
		}
	}

	return std::filesystem::path();

} // FindMSVCDir

//////////////////////////////////////////////////////////////////////////

static std::wstring FindWindowsSDKVersion( const Configuration& rConfiguration, const std::filesystem::path& rProgramFilesX86 )
{
	const std::wstring Target = GetTargetString( rConfiguration.m_Architecture.value_or( Configuration::HostArchitecture() ) );

	for( const std::filesystem::directory_entry& rDirectory : std::filesystem::directory_iterator( rProgramFilesX86 / "Windows Kits" / "10" / "Lib" ) )
	{
		const std::filesystem::path DirectoryPath = rDirectory.path();

		if( std::filesystem::exists( DirectoryPath / "um" / Target / "kernel32.lib" ) )
			return DirectoryPath.filename();
	}

	return std::wstring();

} // FindWindowsSDKVersion

//////////////////////////////////////////////////////////////////////////

std::wstring CompilerMSVC::MakeCompilerCommandLineString( const Configuration& rConfiguration, const std::filesystem::path& rFilePath )
{
	const std::filesystem::path ProgramFilesX86 = FindProgramFilesX86Dir();
	const std::filesystem::path MSVCDir         = FindMSVCDir( ProgramFilesX86 );
	const std::wstring          Host            = GetHostString();
	const std::wstring          Target          = GetTargetString( rConfiguration.m_Architecture.value_or( Configuration::HostArchitecture() ) );
	UTF8Converter               UTF8;

	std::wstring CommandLine;
	CommandLine += L"\"" + ( MSVCDir / "bin" / Host / Target / "cl.exe" ).wstring() + L"\"";
	CommandLine += L" /nologo";

	// Compile (don't just preprocess)
	CommandLine += L" /c";

	// Language-specific options
	const auto FileExtension = rFilePath.extension();
	if     ( FileExtension == ".c"   ) CommandLine += L" /std:c11";
	else if( FileExtension == ".cpp" ) CommandLine += L" /std:c++latest /D _HAS_EXCEPTIONS=0";
	else if( FileExtension == ".cxx" ) CommandLine += L" /std:c++latest /D _HAS_EXCEPTIONS=0";
	else if( FileExtension == ".cc"  ) CommandLine += L" /std:c++latest /D _HAS_EXCEPTIONS=0";

	// Add user-defined preprocessor defines
	for( const std::string& rDefine : rConfiguration.m_Defines )
	{
		CommandLine += L" /D \"" + UTF8.from_bytes( rDefine ) + L"\"";
	}

	// Set standard include directories
	// TODO: Add these to the "Windows" system default configuration's m_IncludeDirs?
	{
		const std::wstring          WindowsSDKVersion    = FindWindowsSDKVersion( rConfiguration, ProgramFilesX86 );
		const std::filesystem::path WindowsSDKIncludeDir = ProgramFilesX86 / "Windows Kits" / "10" / "Include" / WindowsSDKVersion;

		CommandLine += L" /I\"" + ( MSVCDir / "include"             ).wstring() + L"\"";
		CommandLine += L" /I\"" + ( WindowsSDKIncludeDir / "ucrt"   ).wstring() + L"\"";
		CommandLine += L" /I\"" + ( WindowsSDKIncludeDir / "um"     ).wstring() + L"\"";
		CommandLine += L" /I\"" + ( WindowsSDKIncludeDir / "shared" ).wstring() + L"\"";
	}

	// Add user-defined include directories
	for( const std::filesystem::path& rIncludeDir : rConfiguration.m_IncludeDirs )
	{
		CommandLine += L" /I\"" + rIncludeDir.wstring() + L"\"";
	}

	// Set output file
	CommandLine += L" /Fo\"" + GetCompilerOutputPath( rConfiguration, rFilePath ).wstring() + L"\"";

	// Set input file
	if     ( FileExtension == ".c"   ) CommandLine += L" /Tc \"" + rFilePath.wstring() + L"\"";
	else if( FileExtension == ".cpp" ) CommandLine += L" /Tp \"" + rFilePath.wstring() + L"\"";
	else if( FileExtension == ".cxx" ) CommandLine += L" /Tp \"" + rFilePath.wstring() + L"\"";
	else if( FileExtension == ".cc"  ) CommandLine += L" /Tp \"" + rFilePath.wstring() + L"\"";

	return CommandLine;

} // MakeCompilerCommandLineString

//////////////////////////////////////////////////////////////////////////

std::wstring CompilerMSVC::MakeLinkerCommandLineString( const Configuration& rConfiguration, std::span< std::filesystem::path > InputFiles, const std::wstring& rOutputName, Project::Kind Kind )
{
	const std::filesystem::path ProgramFilesX86   = FindProgramFilesX86Dir();
	const std::filesystem::path MSVCDir           = FindMSVCDir( ProgramFilesX86 );
	const std::filesystem::path OutputPath        = GetLinkerOutputPath( rConfiguration, rOutputName, Kind );
	const std::wstring          WindowsSDKVersion = FindWindowsSDKVersion( rConfiguration, ProgramFilesX86 );
	const std::wstring          Host              = GetHostString();
	const std::wstring          Target            = GetTargetString( rConfiguration.m_Architecture.value_or( Configuration::HostArchitecture() ) );
	std::wstring                CommandLine;

	CommandLine += L"\"" + ( MSVCDir / "bin" / Host / Target / "link.exe" ).wstring() + L"\"";

	switch( Kind )
	{
		case Project::Kind::Application:    { CommandLine += L" /SUBSYSTEM:CONSOLE /OUT:\"" + OutputPath.wstring() + L"\""; } break;
		case Project::Kind::StaticLibrary:  { CommandLine += L" /LIB /OUT:\"" + OutputPath.wstring() + L"\"";               } break;
		case Project::Kind::DynamicLibrary: { CommandLine += L" /DLL /OUT:\"" + OutputPath.wstring() + L"\"";               } break;
	}

	// Add standard library paths
	{
		const std::filesystem::path WindowsSDKLibraryDir = ProgramFilesX86 / "Windows Kits" / "10" / "Lib" / WindowsSDKVersion;

		CommandLine += L" /LIBPATH:\"" + ( MSVCDir / "lib" / Target               ).wstring() + L"\"";
		CommandLine += L" /LIBPATH:\"" + ( WindowsSDKLibraryDir / "um" / Target   ).wstring() + L"\"";
		CommandLine += L" /LIBPATH:\"" + ( WindowsSDKLibraryDir / "ucrt" / Target ).wstring() + L"\"";
	}

	// Add user-defined library paths
	for( const std::filesystem::path& rLibraryDirectory : rConfiguration.m_LibraryDirs )
	{
		// Get rid of trailing slashes. It's not allowed in MSVC
		const std::filesystem::path Path = ( rLibraryDirectory / L"NUL" ).parent_path();

		CommandLine += L" /LIBPATH:\"" + Path.wstring() + L"\"";
	}

	// Add input files
	for( std::filesystem::path Library : rConfiguration.m_Libraries )
	{
		// Assume static library if the extension is missing
		if( !Library.has_extension() )
			Library.replace_extension( ".lib" );

		CommandLine += L" \"" + Library.wstring() + L"\"";
	}

	// Add all object files
	for( const std::filesystem::path& rInputFile : InputFiles )
	{
		CommandLine += L" \"" + rInputFile.wstring() + L".obj\"";
	}

	// Miscellaneous options
	CommandLine += L" /NOLOGO";

	if( rConfiguration.m_Architecture )
	{
		switch( *rConfiguration.m_Architecture )
		{
			case Configuration::Architecture::x86_64:
				CommandLine += L" /MACHINE:x64";
				break;

			default:
				break;
		}
	}

	return CommandLine;

} // MakeLinkerCommandLineString

#endif // _WIN32
