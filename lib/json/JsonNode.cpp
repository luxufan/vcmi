/*
 * JsonNode.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"
#include "JsonNode.h"

#include "filesystem/Filesystem.h"
#include "JsonParser.h"
#include "JsonWriter.h"

namespace
{
// to avoid duplicating const and non-const code
template<typename Node>
Node & resolvePointer(Node & in, const std::string & pointer)
{
	if(pointer.empty())
		return in;
	assert(pointer[0] == '/');

	size_t splitPos = pointer.find('/', 1);

	std::string entry = pointer.substr(1, splitPos - 1);
	std::string remainer = splitPos == std::string::npos ? "" : pointer.substr(splitPos);

	if(in.getType() == VCMI_LIB_WRAP_NAMESPACE(JsonNode)::JsonType::DATA_VECTOR)
	{
		if(entry.find_first_not_of("0123456789") != std::string::npos) // non-numbers in string
			throw std::runtime_error("Invalid Json pointer");

		if(entry.size() > 1 && entry[0] == '0') // leading zeros are not allowed
			throw std::runtime_error("Invalid Json pointer");

		auto index = boost::lexical_cast<size_t>(entry);

		if (in.Vector().size() > index)
			return in.Vector()[index].resolvePointer(remainer);
	}
	return in[entry].resolvePointer(remainer);
}
}

VCMI_LIB_NAMESPACE_BEGIN

using namespace JsonDetail;

class LibClasses;
class CModHandler;

static const JsonNode nullNode;

JsonNode::JsonNode(JsonType Type)
{
	setType(Type);
}

JsonNode::JsonNode(const std::byte *data, size_t datasize)
	:JsonNode(reinterpret_cast<const char*>(data), datasize)
{}

JsonNode::JsonNode(const char *data, size_t datasize)
{
	JsonParser parser(data, datasize);
	*this = parser.parse("<unknown>");
}

JsonNode::JsonNode(const JsonPath & fileURI)
{
	auto file = CResourceHandler::get()->load(fileURI)->readAll();

	JsonParser parser(reinterpret_cast<char*>(file.first.get()), file.second);
	*this = parser.parse(fileURI.getName());
}

JsonNode::JsonNode(const std::string & idx, const JsonPath & fileURI)
{
	auto file = CResourceHandler::get(idx)->load(fileURI)->readAll();
	
	JsonParser parser(reinterpret_cast<char*>(file.first.get()), file.second);
	*this = parser.parse(fileURI.getName());
}

JsonNode::JsonNode(const JsonPath & fileURI, bool &isValidSyntax)
{
	auto file = CResourceHandler::get()->load(fileURI)->readAll();

	JsonParser parser(reinterpret_cast<char*>(file.first.get()), file.second);
	*this = parser.parse(fileURI.getName());
	isValidSyntax = parser.isValid();
}

bool JsonNode::operator == (const JsonNode &other) const
{
	return data == other.data;
}

bool JsonNode::operator != (const JsonNode &other) const
{
	return !(*this == other);
}

JsonNode::JsonType JsonNode::getType() const
{
	return static_cast<JsonType>(data.index());
}

void JsonNode::setMeta(const std::string & metadata, bool recursive)
{
	meta = metadata;
	if (recursive)
	{
		switch (getType())
		{
			break; case JsonType::DATA_VECTOR:
			{
				for(auto & node : Vector())
				{
					node.setMeta(metadata);
				}
			}
			break; case JsonType::DATA_STRUCT:
			{
				for(auto & node : Struct())
				{
					node.second.setMeta(metadata);
				}
			}
		}
	}
}

void JsonNode::setType(JsonType Type)
{
	if (getType() == Type)
		return;

	//float<->int conversion
	if(getType() == JsonType::DATA_FLOAT && Type == JsonType::DATA_INTEGER)
	{
		si64 converted = static_cast<si64>(std::get<double>(data));
		data = JsonData(converted);
		return;
	}
	else if(getType() == JsonType::DATA_INTEGER && Type == JsonType::DATA_FLOAT)
	{
		double converted = static_cast<double>(std::get<si64>(data));
		data = JsonData(converted);
		return;
	}

	//Set new node type
	switch(Type)
	{
		break; case JsonType::DATA_NULL:    data = JsonData();
		break; case JsonType::DATA_BOOL:    data = JsonData(false);
		break; case JsonType::DATA_FLOAT:   data = JsonData(static_cast<double>(0.0));
		break; case JsonType::DATA_STRING:  data = JsonData(std::string());
		break; case JsonType::DATA_VECTOR:  data = JsonData(JsonVector());
		break; case JsonType::DATA_STRUCT:  data = JsonData(JsonMap());
		break; case JsonType::DATA_INTEGER: data = JsonData(static_cast<si64>(0));
	}
}

bool JsonNode::isNull() const
{
	return getType() == JsonType::DATA_NULL;
}

bool JsonNode::isNumber() const
{
	return getType() == JsonType::DATA_INTEGER || getType() == JsonType::DATA_FLOAT;
}

bool JsonNode::isString() const
{
	return getType() == JsonType::DATA_STRING;
}

bool JsonNode::isVector() const
{
	return getType() == JsonType::DATA_VECTOR;
}

bool JsonNode::isStruct() const
{
	return getType() == JsonType::DATA_STRUCT;
}

bool JsonNode::containsBaseData() const
{
	switch(getType())
	{
	case JsonType::DATA_NULL:
		return false;
	case JsonType::DATA_STRUCT:
		for(const auto & elem : Struct())
		{
			if(elem.second.containsBaseData())
				return true;
		}
		return false;
	default:
		//other types (including vector) cannot be extended via merge
		return true;
	}
}

bool JsonNode::isCompact() const
{
	switch(getType())
	{
	case JsonType::DATA_VECTOR:
		for(const JsonNode & elem : Vector())
		{
			if(!elem.isCompact())
				return false;
		}
		return true;
	case JsonType::DATA_STRUCT:
		{
			auto propertyCount = Struct().size();
			if(propertyCount == 0)
				return true;
			else if(propertyCount == 1)
				return Struct().begin()->second.isCompact();
		}
		return false;
	default:
		return true;
	}
}

bool JsonNode::TryBoolFromString(bool & success) const
{
	success = true;
	if(getType() == JsonNode::JsonType::DATA_BOOL)
		return Bool();

	success = getType() == JsonNode::JsonType::DATA_STRING;
	if(success)
	{
		auto boolParamStr = String();
		boost::algorithm::trim(boolParamStr);
		boost::algorithm::to_lower(boolParamStr);
		success = boolParamStr == "true";

		if(success)
			return true;
		
		success = boolParamStr == "false";
	}
	return false;
}

void JsonNode::clear()
{
	setType(JsonType::DATA_NULL);
}

bool & JsonNode::Bool()
{
	setType(JsonType::DATA_BOOL);
	return std::get<bool>(data);
}

double & JsonNode::Float()
{
	setType(JsonType::DATA_FLOAT);
	return std::get<double>(data);
}

si64 & JsonNode::Integer()
{
	setType(JsonType::DATA_INTEGER);
	return std::get<si64>(data);
}

std::string & JsonNode::String()
{
	setType(JsonType::DATA_STRING);
	return std::get<std::string>(data);
}

JsonVector & JsonNode::Vector()
{
	setType(JsonType::DATA_VECTOR);
	return std::get<JsonVector>(data);
}

JsonMap & JsonNode::Struct()
{
	setType(JsonType::DATA_STRUCT);
	return std::get<JsonMap>(data);
}

const bool boolDefault = false;
bool JsonNode::Bool() const
{
	assert(getType() == JsonType::DATA_NULL || getType() == JsonType::DATA_BOOL);

	if (getType() == JsonType::DATA_BOOL)
		return std::get<bool>(data);

	return boolDefault;
}

const double floatDefault = 0;
double JsonNode::Float() const
{
	assert(getType() == JsonType::DATA_NULL || getType() == JsonType::DATA_INTEGER || getType() == JsonType::DATA_FLOAT);

	if(getType() == JsonType::DATA_FLOAT)
		return std::get<double>(data);

	if(getType() == JsonType::DATA_INTEGER)
		return static_cast<double>(std::get<si64>(data));

	return floatDefault;
}

const si64 integerDefault = 0;
si64 JsonNode::Integer() const
{
	assert(getType() == JsonType::DATA_NULL || getType() == JsonType::DATA_INTEGER || getType() == JsonType::DATA_FLOAT);

	if(getType() == JsonType::DATA_INTEGER)
		return std::get<si64>(data);

	if(getType() == JsonType::DATA_FLOAT)
		return static_cast<si64>(std::get<double>(data));

	return integerDefault;
}

const std::string stringDefault = std::string();
const std::string & JsonNode::String() const
{
	assert(getType() == JsonType::DATA_NULL || getType() == JsonType::DATA_STRING);

	if (getType() == JsonType::DATA_STRING)
		return std::get<std::string>(data);

	return stringDefault;
}

const JsonVector vectorDefault = JsonVector();
const JsonVector & JsonNode::Vector() const
{
	assert(getType() == JsonType::DATA_NULL || getType() == JsonType::DATA_VECTOR);

	if (getType() == JsonType::DATA_VECTOR)
		return std::get<JsonVector>(data);

	return vectorDefault;
}

const JsonMap mapDefault = JsonMap();
const JsonMap & JsonNode::Struct() const
{
	assert(getType() == JsonType::DATA_NULL || getType() == JsonType::DATA_STRUCT);

	if (getType() == JsonType::DATA_STRUCT)
		return std::get<JsonMap>(data);

	return mapDefault;
}

JsonNode & JsonNode::operator[](const std::string & child)
{
	return Struct()[child];
}

const JsonNode & JsonNode::operator[](const std::string & child) const
{
	auto it = Struct().find(child);
	if (it != Struct().end())
		return it->second;
	return nullNode;
}

JsonNode & JsonNode::operator[](size_t child)
{
	if (child >= Vector().size() )
		Vector().resize(child + 1);
	return Vector()[child];
}

const JsonNode & JsonNode::operator[](size_t child) const
{
	if (child < Vector().size() )
		return Vector()[child];

	return nullNode;
}

const JsonNode & JsonNode::resolvePointer(const std::string &jsonPointer) const
{
	return ::resolvePointer(*this, jsonPointer);
}

JsonNode & JsonNode::resolvePointer(const std::string &jsonPointer)
{
	return ::resolvePointer(*this, jsonPointer);
}

std::vector<std::byte> JsonNode::toBytes(bool compact) const
{
	std::string jsonString = toJson(compact);
	auto dataBegin = reinterpret_cast<const std::byte*>(jsonString.data());
	auto dataEnd = dataBegin + jsonString.size();
	std::vector<std::byte> result(dataBegin, dataEnd);
	return result;
}

std::string JsonNode::toJson(bool compact) const
{
	std::ostringstream out;
	JsonWriter writer(out, compact);
	writer.writeNode(*this);
	return out.str();
}

VCMI_LIB_NAMESPACE_END
