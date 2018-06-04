/******************************************************************************
 * Icinga 2                                                                   *
 * Copyright (C) 2012-2018 Icinga Development Team (https://www.icinga.com/)  *
 *                                                                            *
 * This program is free software; you can redistribute it and/or              *
 * modify it under the terms of the GNU General Public License                *
 * as published by the Free Software Foundation; either version 2             *
 * of the License, or (at your option) any later version.                     *
 *                                                                            *
 * This program is distributed in the hope that it will be useful,            *
 * but WITHOUT ANY WARRANTY; without even the implied warranty of             *
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the              *
 * GNU General Public License for more details.                               *
 *                                                                            *
 * You should have received a copy of the GNU General Public License          *
 * along with this program; if not, write to the Free Software Foundation     *
 * Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301, USA.             *
 ******************************************************************************/

#ifndef APICLIENT_H
#define APICLIENT_H

#include "base/tlsstream.hpp"
#include "base/value.hpp"
#include "base/exception.hpp"
#include <vector>

namespace icinga
{

class ApiClient : public Object
{
public:
	DECLARE_PTR_TYPEDEFS(ApiClient);

	ApiClient(const String& host, const String& port, const String& user, const String& password);

	Dictionary::Ptr ExecuteScript(const String& session, const String& command, bool sandboxed) const;

	Array::Ptr AutocompleteScript(const String& session, const String& command, bool sandboxed) const;

private:
	static TlsStream::Ptr Connect(const String& host, const String& port);

	String m_Host;
	String m_Port;
	String m_User;
	String m_Password;
};

}

#endif /* APICLIENT_H */
