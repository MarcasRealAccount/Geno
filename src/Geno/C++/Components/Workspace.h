/*
 * Copyright (c) 2020 Sebastian Kylander https://gaztin.com/
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

#pragma once
#include "Compilers/ICompiler.h"
#include "Components/BuildMatrix.h"
#include "Components/Project.h"

#include <Common/EventDispatcher.h>
#include <GCL/Deserializer.h>

#include <filesystem>
#include <memory>
#include <string>
#include <vector>

class Workspace;

struct WorkspaceBuildFinished
{
	Workspace*            workspace;
	std::filesystem::path output;
	bool                  success;
};

class Workspace : public EventDispatcher< Workspace, WorkspaceBuildFinished >
{
	GENO_DISABLE_COPY( Workspace );
	GENO_DEFAULT_MOVE( Workspace );

public:

	static constexpr std::string_view ext = ".gwks";

public:

	explicit Workspace( std::filesystem::path location );

public:

	void Build      ( void );
	bool Serialize  ( void );
	bool Deserialize( void );

public:

	Project& NewProject   ( std::filesystem::path location, std::string name );
	Project* ProjectByName( std::string_view name );

public:

	std::filesystem::path        location_;
	std::string                  name_;
	std::vector< Project >       projects_;
	std::unique_ptr< ICompiler > compiler_;

	std::vector< std::string >   projects_left_to_build_;

	BuildMatrix                  build_matrix_;

private:

	static void GCLObjectCallback( GCL::Object object, void* user );

private:

	void BuildNextProject            ( void );
	void OnBuildFinished             ( const std::filesystem::path& output, bool success );
	void SerializeBuildMatrixColumn  ( GCL::Object& object, const BuildMatrix::Column& column );
	void DeserializeBuildMatrixColumn( BuildMatrix::Column& column, const GCL::Object& object );

};
