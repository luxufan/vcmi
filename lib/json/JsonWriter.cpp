/*
 * JsonWriter.cpp, part of VCMI engine
 *
 * Authors: listed in file AUTHORS in main folder
 *
 * License: GNU General Public License v2.0 or later
 * Full text of license available in license.txt file, in main folder
 *
 */

#include "StdInc.h"
#include "JsonWriter.h"

VCMI_LIB_NAMESPACE_BEGIN

template<typename Iterator>
void JsonWriter::writeContainer(Iterator begin, Iterator end)
{
	if (begin == end)
		return;

	prefix += '\t';

	writeEntry(begin++);

	while (begin != end)
	{
		out << (compactMode ? ", " : ",\n");
		writeEntry(begin++);
	}

	out << (compactMode ? "" : "\n");
	prefix.resize(prefix.size()-1);
}

void JsonWriter::writeEntry(JsonMap::const_iterator entry)
{
	if(!compactMode)
	{
		if (!entry->second.meta.empty())
			out << prefix << " // " << entry->second.meta << "\n";
		if(!entry->second.flags.empty())
			out << prefix << " // flags: " << boost::algorithm::join(entry->second.flags, ", ") << "\n";
		out << prefix;
	}
	writeString(entry->first);
	out << " : ";
	writeNode(entry->second);
}

void JsonWriter::writeEntry(JsonVector::const_iterator entry)
{
	if(!compactMode)
	{
		if (!entry->meta.empty())
			out << prefix << " // " << entry->meta << "\n";
		if(!entry->flags.empty())
			out << prefix << " // flags: " << boost::algorithm::join(entry->flags, ", ") << "\n";
		out << prefix;
	}
	writeNode(*entry);
}

void JsonWriter::writeString(const std::string &string)
{
	static const std::string escaped = "\"\\\b\f\n\r\t/";

	static const std::array<char, 8> escaped_code = {'\"', '\\', 'b', 'f', 'n', 'r', 't', '/'};

	out <<'\"';
	size_t pos = 0;
	size_t start = 0;
	for (; pos<string.size(); pos++)
	{
		//we need to check if special character was been already escaped
		if((string[pos] == '\\')
			&& (pos+1 < string.size())
			&& (std::find(escaped_code.begin(), escaped_code.end(), string[pos+1]) != escaped_code.end()) )
		{
			pos++; //write unchanged, next simbol also checked
		}
		else
		{
			size_t escapedPos = escaped.find(string[pos]);

			if (escapedPos != std::string::npos)
			{
				out.write(string.data()+start, pos - start);
				out << '\\' << escaped_code[escapedPos];
				start = pos+1;
			}
		}

	}
	out.write(string.data()+start, pos - start);
	out <<'\"';
}

void JsonWriter::writeNode(const JsonNode &node)
{
	bool originalMode = compactMode;
	if(compact && !compactMode && node.isCompact())
		compactMode = true;

	switch(node.getType())
	{
		break; case JsonNode::JsonType::DATA_NULL:
			out << "null";

		break; case JsonNode::JsonType::DATA_BOOL:
			if (node.Bool())
				out << "true";
			else
				out << "false";

		break; case JsonNode::JsonType::DATA_FLOAT:
			out << node.Float();

		break; case JsonNode::JsonType::DATA_STRING:
			writeString(node.String());

		break; case JsonNode::JsonType::DATA_VECTOR:
			out << "[" << (compactMode ? " " : "\n");
			writeContainer(node.Vector().begin(), node.Vector().end());
			out << (compactMode ? " " : prefix) << "]";

		break; case JsonNode::JsonType::DATA_STRUCT:
			out << "{" << (compactMode ? " " : "\n");
			writeContainer(node.Struct().begin(), node.Struct().end());
			out << (compactMode ? " " : prefix) << "}";

		break; case JsonNode::JsonType::DATA_INTEGER:
			out << node.Integer();
	}

	compactMode = originalMode;
}

JsonWriter::JsonWriter(std::ostream & output, bool compact)
	: out(output), compact(compact)
{
}

VCMI_LIB_NAMESPACE_END
