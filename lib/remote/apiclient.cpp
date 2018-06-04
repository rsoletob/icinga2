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

#include "remote/apiclient.hpp"
#include "remote/url.hpp"
#include "remote/httpclientconnection.hpp"
#include "base/base64.hpp"
#include "base/json.hpp"
#include "base/logger.hpp"
#include "base/exception.hpp"
#include "base/convert.hpp"
#include "base/tcpsocket.hpp"
#include <boost/scoped_array.hpp>

using namespace icinga;

ApiClient::ApiClient(const String& host, const String& port, const String& user, const String& password)
	: m_Host(host), m_Port(port), m_User(user), m_Password(password)
{ }

TlsStream::Ptr ApiClient::Connect(const String& host, const String& port) {
	TcpSocket::Ptr socket = new TcpSocket();
	TlsStream::Ptr tlsStream;

	try {
		socket->Connect(host, port);
		tlsStream = new TlsStream(socket, host, RoleClient);
		tlsStream->Handshake();
	} catch (const std::exception& ex) {
		Log(LogWarning, "ApiClient")
			<< "Can't connect to Api on host '" << host << "' port '" << port << "'.";
		throw ex;
	}

	return tlsStream;
}

Dictionary::Ptr ApiClient::ExecuteScript(const String& session, const String& command, bool sandboxed) const
{
	TlsStream::Ptr stream = Connect(m_Host, m_Port);

	if (!stream) {
		Log(LogCritical, "ApiClient", "Failed to open TLS stream.");
		return nullptr;
	}

	Url::Ptr url = new Url();
	url->SetScheme("https");
	url->SetHost(m_Host);
	url->SetPort(m_Port);

	url->SetPath({ "v1", "console", "execute-script" });

	std::map<String, std::vector<String> > params;
	params["session"].push_back(session);
	params["command"].push_back(command);
	params["sandboxed"].emplace_back(sandboxed ? "1" : "0");
	url->SetQuery(params);

	HttpRequest req(stream);
	req.RequestMethod = "POST";
	req.RequestUrl = url;
	req.AddHeader("Authorization", "Basic " + Base64::Encode(m_User + ":" + m_Password));
	req.AddHeader("Accept", "application/json");

	try {
		//req.WriteBody(body.CStr(), body.GetLength());
		req.WriteBody("", 0);
		req.Finish();
	} catch (const std::exception& ex) {
		Log(LogWarning, "ApiClient")
			<< "Cannot write to TCP socket on host '" << m_Host << "' port '" << m_Port << "'.";
		throw ex;
	}

	HttpResponse resp(stream, req);
	StreamReadContext context;

	try {
		while (resp.Parse(context, true) && !resp.Complete)
			; /* Do nothing */
	} catch (const std::exception& ex) {
		Log(LogWarning, "ApiClient")
		    << "Failed to parse HTTP response from host '" << m_Host << "' port '" << m_Port << "': "
		    << DiagnosticInformation(ex);
		throw ex;
	}

	if (!resp.Complete) {
		Log(LogWarning, "ApiClient")
			<< "Failed to read a complete HTTP response from the server.";
		return nullptr;
	}

	if (resp.StatusCode < 200 || resp.StatusCode > 299) {
		Log(LogCritical, "ApiClient")
		    << "Unexpected status code: " << resp.StatusCode;
		return nullptr;

		//BOOST_THROW_EXCEPTION(ScriptError(message));
	}


	size_t responseSize = resp.GetBodySize();
	boost::scoped_array<char> buffer(new char[responseSize + 1]);
	resp.ReadBody(buffer.get(), responseSize);
	buffer.get()[responseSize] = '\0';

	Dictionary::Ptr answer;

	try {
		answer = JsonDecode(buffer.get());
	} catch (...) {
		Log(LogWarning, "ApiClient")
			<< "Unable to parse JSON response:\n" << buffer.get();
		return nullptr;
	}

	Array::Ptr results = answer->Get("results");
	String errorMessage = "Unexpected result from API.";

	Value result;

	if (results && results->GetLength() > 0) {
		Dictionary::Ptr resultInfo = results->Get(0);
		errorMessage = resultInfo->Get("status");

		if (resultInfo->Get("code") >= 200 && resultInfo->Get("code") <= 299) {
			result = resultInfo->Get("result");
		} else {
			DebugInfo di;
			Dictionary::Ptr debugInfo = resultInfo->Get("debug_info");
			if (debugInfo) {
				di.Path = debugInfo->Get("path");
				di.FirstLine = debugInfo->Get("first_line");
				di.FirstColumn = debugInfo->Get("first_column");
				di.LastLine = debugInfo->Get("last_line");
				di.LastColumn = debugInfo->Get("last_column");
			}
			bool incompleteExpression = resultInfo->Get("incomplete_expression");
			BOOST_THROW_EXCEPTION(ScriptError(errorMessage, di, incompleteExpression));
		}
	}

	return result;
}

Array::Ptr ApiClient::AutocompleteScript(const String& session, const String& command, bool sandboxed) const
{
	TlsStream::Ptr stream = Connect(m_Host, m_Port);

	Url::Ptr url = new Url();
	url->SetScheme("https");
	url->SetHost(m_Host);
	url->SetPort(m_Port);
	url->SetPath({ "v1", "console", "auto-complete-script" });

	std::map<String, std::vector<String> > params;
	params["session"].push_back(session);
	params["command"].push_back(command);
	params["sandboxed"].emplace_back(sandboxed ? "1" : "0");
	url->SetQuery(params);

	HttpRequest req(stream);

	req.RequestMethod = "POST";
	req.RequestUrl = url;
	req.AddHeader("Authorization", "Basic " + Base64::Encode(m_User + ":" + m_Password));
	req.AddHeader("Accept", "application/json");

	try {
		req.Finish();
	} catch (const std::exception& ex) {
		Log(LogWarning, "ApiClient")
			<< "Cannot write to TCP socket on host '" << m_Host << "' port '" << m_Port << "'.";
		throw ex;
	}

	HttpResponse resp(stream, req);
	StreamReadContext context;

	try {
		while (resp.Parse(context, true) && !resp.Complete)
			; /* Do nothing */
	} catch (const std::exception& ex) {
		Log(LogWarning, "InfluxdbWriter")
			<< "Failed to parse HTTP response from host '" << m_Host << "' port '" << m_Port << "': " << DiagnosticInformation(ex);
		throw ex;
	}

	if (!resp.Complete) {
		Log(LogWarning, "ApiClient")
			<< "Failed to read a complete HTTP response from the server.";
		return nullptr;
	}

	size_t responseSize = resp.GetBodySize();
	boost::scoped_array<char> buffer(new char[responseSize + 1]);
	resp.ReadBody(buffer.get(), responseSize);
	buffer.get()[responseSize] = '\0';

	if (resp.StatusCode < 200 || resp.StatusCode > 299) {
		std::string message = "HTTP request failed; Code: " + Convert::ToString(resp.StatusCode) + "; Body: " + buffer.get();

		BOOST_THROW_EXCEPTION(ScriptError(message));
	}

	Dictionary::Ptr answer;

	try {
		answer = JsonDecode(buffer.get());
	} catch (...) {
		Log(LogWarning, "ApiClient")
			<< "Unable to parse JSON response:\n" << buffer.get();
		return nullptr;
	}

	Array::Ptr results = answer->Get("results");
	Array::Ptr suggestions;
	String errorMessage = "Unexpected result from API.";

	if (results && results->GetLength() > 0) {
		Dictionary::Ptr resultInfo = results->Get(0);
		errorMessage = resultInfo->Get("status");

		if (resultInfo->Get("code") >= 200 && resultInfo->Get("code") <= 299)
			suggestions = resultInfo->Get("suggestions");
		else
			BOOST_THROW_EXCEPTION(ScriptError(errorMessage));
	}

	return suggestions;
}
