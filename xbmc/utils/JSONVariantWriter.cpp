/*
 *  Copyright (C) 2015-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: GPL-2.0-or-later
 *  See LICENSES/README.md for more information.
 */

#include "JSONVariantWriter.h"

#include "utils/Variant.h"

#include <algorithm>
#include <charconv>

#include <nlohmann/json.hpp>

namespace
{
void WriteString(std::string& output, nlohmann::json& scratch, const char* data, size_t size)
{
  // printable ASCII without quote or backslash is written as is by nlohmann too
  const bool plain = std::all_of(data, data + size, [](unsigned char c)
                                 { return c >= 0x20 && c < 0x7F && c != '"' && c != '\\'; });
  if (plain)
  {
    output += '"';
    output.append(data, size);
    output += '"';
    return;
  }

  if (!scratch.is_string())
    scratch = "";
  scratch.get_ref<std::string&>().assign(data, size);
  output += scratch.dump();
}

void WriteCompact(std::string& output, nlohmann::json& scratch, const CVariant& value)
{
  switch (value.type())
  {
    case CVariant::VariantTypeInteger:
    {
      char buffer[24];
      output.append(buffer, std::to_chars(buffer, buffer + sizeof(buffer), value.asInteger()).ptr);
      break;
    }
    case CVariant::VariantTypeUnsignedInteger:
    {
      char buffer[24];
      output.append(buffer,
                    std::to_chars(buffer, buffer + sizeof(buffer), value.asUnsignedInteger()).ptr);
      break;
    }
    case CVariant::VariantTypeDouble:
      scratch = value.asDouble();
      output += scratch.dump();
      break;
    case CVariant::VariantTypeBoolean:
      output += value.asBoolean() ? "true" : "false";
      break;
    case CVariant::VariantTypeString:
      WriteString(output, scratch, value.c_str(), value.size());
      break;
    case CVariant::VariantTypeArray:
      output += '[';
      for (auto itr = value.begin_array(); itr != value.end_array(); ++itr)
      {
        if (itr != value.begin_array())
          output += ',';
        WriteCompact(output, scratch, *itr);
      }
      output += ']';
      break;
    case CVariant::VariantTypeObject:
      output += '{';
      for (auto itr = value.begin_map(); itr != value.end_map(); ++itr)
      {
        if (itr != value.begin_map())
          output += ',';
        WriteString(output, scratch, itr->first.c_str(), itr->first.size());
        output += ':';
        WriteCompact(output, scratch, itr->second);
      }
      output += '}';
      break;
    case CVariant::VariantTypeConstNull:
    case CVariant::VariantTypeNull:
    default:
      output += "null";
      break;
  }
}
} // namespace

void InternalWrite(nlohmann::json& writer, const CVariant& value)
{
  switch (value.type())
  {
  case CVariant::VariantTypeInteger:
    writer = value.asInteger();
    break;
  case CVariant::VariantTypeUnsignedInteger:
    writer = value.asUnsignedInteger();
    break;
  case CVariant::VariantTypeDouble:
    writer = value.asDouble();
    break;
  case CVariant::VariantTypeBoolean:
    writer = value.asBoolean();
    break;
  case CVariant::VariantTypeString:
    writer = std::string(value.c_str(), value.size());
    break;
  case CVariant::VariantTypeArray:
    writer = nlohmann::json::array();

    for (CVariant::const_iterator_array itr = value.begin_array(); itr != value.end_array(); ++itr)
    {
      nlohmann::json& o = writer.emplace_back();
      InternalWrite(o, *itr);
    }

    break;
  case CVariant::VariantTypeObject:
    writer = nlohmann::json::object();

    for (CVariant::const_iterator_map itr = value.begin_map(); itr != value.end_map(); ++itr)
    {
      nlohmann::json& o = writer[itr->first];
      InternalWrite(o, itr->second);
    }

    break;

  case CVariant::VariantTypeConstNull:
  case CVariant::VariantTypeNull:
  default:
    writer = nullptr;
    break;
  }
}

bool CJSONVariantWriter::Write(const CVariant &value, std::string& output, bool compact)
{
  try
  {
    if (compact)
    {
      std::string written;
      nlohmann::json scratch;
      WriteCompact(written, scratch, value);
      output = std::move(written);
    }
    else
    {
      nlohmann::json json;
      InternalWrite(json, value);
      output = json.dump(1, '\t');
    }
  }
  catch (nlohmann::json::exception&)
  {
    return false;
  }

  return true;
}
