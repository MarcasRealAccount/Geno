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
#include "GUI/Modals/IModal.h"

#include <filesystem>

class NewItemModal : public IModal
{
	GENO_SINGLETON( NewItemModal ) = default;

public:

	using PathCallback   = void( * )( std::string name, std::filesystem::path location, void* user );
	using StringCallback = void( * )( std::string string, void* user );

public:

	void RequestPath  ( std::string_view title, void* user, PathCallback callback );
	void RequestString( std::string_view title, void* user, StringCallback callback );

private:

	std::string PopupID      ( void ) override { return "NewItem"; };
	std::string Title        ( void ) override { return title_; };
	void        UpdateDerived( void ) override;
	void        OnClose      ( void ) override;

private:

	void UpdateItem  ( void );
	void UpdateString( void );

private:

	std::string title_;
	std::string name_;
	std::string location_;

	void*       callback_     = nullptr;
	void*       user_         = nullptr;
	int         request_type_ = -1;

};
